#include "afl.hpp"
#include "common.hpp"

#include <optional>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <afl/types.h>

namespace
{

static uint32_t __afl_fuzz_len_dummy;

uint8_t* __afl_area_ptr;

uint8_t* __afl_fuzz_ptr;
uint32_t* __afl_fuzz_len = &__afl_fuzz_len_dummy;

uint32_t __afl_map_size = MAP_SIZE;

} // namespace

namespace
{

enum class ChildMessage : uint32_t
{
    Next,
    Exit,
    Crash,
};

bool forkserverRunning = false;

int childPipe[2];
int parentPipe[2];

uint32_t prevLocation = 0;

template<std::integral T>
bool ForkserverWrite(T value)
{
    return ::write(FORKSRV_FD + 1, &value, sizeof(value)) == sizeof(value);
}

template<std::integral T>
std::optional<T> ForkserverRead()
{
    T value;
    if (::read(FORKSRV_FD, &value, sizeof(value)) == sizeof(value)) {
        return value;
    }

    return std::nullopt;
}

void ForkserverSendError(int error)
{
    uint32_t status = FS_NEW_ERROR | error;
    ForkserverWrite(status);
}

bool MapFuzzShm()
{
    const char* shmidStr = std::getenv(SHM_FUZZ_ENV_VAR);
    if (!shmidStr) {
        EMU_LOG_ERROR("Failed to get fuzz shared memory env var");
        ForkserverSendError(FS_ERROR_SHM_OPEN);
        return false;
    }

    int shmid = std::atoi(shmidStr);
    uint8_t* memory = static_cast<uint8_t*>(::shmat(shmid, nullptr, 0));
    if (!memory || memory == reinterpret_cast<uint8_t*>(-1)) {
        EMU_LOG_ERROR("Cannot access fuzz shared memory");
        ForkserverSendError(FS_ERROR_SHMAT);
        return false;
    }

    __afl_fuzz_len = reinterpret_cast<uint32_t*>(memory);
    __afl_fuzz_ptr = memory + sizeof(uint32_t);
    return true;
}

bool MapShm()
{
    const char* shmidStr = std::getenv(SHM_ENV_VAR);
    if (!shmidStr) {
        // EMU_LOG_ERROR("Failed to get shared memory env var");
        // ForkserverSendError(FS_ERROR_SHM_OPEN);
        return false;
    }

    int shmid = std::atoi(shmidStr);
    uint8_t* memory = static_cast<uint8_t*>(::shmat(shmid, nullptr, 0));
    if (!memory || memory == reinterpret_cast<uint8_t*>(-1)) {
        EMU_LOG_ERROR("Cannot access shared memory");
        ForkserverSendError(FS_ERROR_SHMAT);
        return false;
    }

    __afl_area_ptr = memory;
    return true;
}

// From https://github.com/AFLplusplus/unicornafl/blob/7b79cd88c61efc1cddef89406aee2fe25ff83d54/src/forkserver.rs#L301-L310
// Find a value for which WIFSIGNALED is true
constexpr int GetValidWIFSIGNALED()
{
    int status = 0;
    while (!WIFSIGNALED(status)) {
        status++;
    }

    return status;
}

constexpr uint64_t hash64(uint64_t value)
{
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

} // namespace

bool afl::StartForkserver()
{
    // TODO where to call this?
    if (!MapShm()) {
        return false;
    }

    // Supported forkserver version
    const uint32_t version = 0x41464c00 + 1; // FS_NEW_VERSION_MAX
    if (!ForkserverWrite(version)) {
        EMU_LOG_ERROR("Failed to write version to forkserver controller");
        return false;
    }

    auto versionReply = ForkserverRead<uint32_t>();
    if (!versionReply) {
        EMU_LOG_ERROR("Failed to receive reply from forkserver controller");
        return false;
    }

    const uint32_t expectedVersion = version ^ 0xffffffff;
    if (*versionReply != expectedVersion) {
        EMU_LOG_ERROR("Invalid version reply from forkserver controller ({:08x} vs {:08x})", *versionReply, expectedVersion);
        return false;
    }

    uint32_t status = FS_NEW_OPT_MAPSIZE;

    status |= FS_NEW_OPT_SHDMEM_FUZZ;

    // ...
    if (!ForkserverWrite(status)) {
        EMU_LOG_ERROR("Failed to write status to forkserver controller");
        return false;
    }

    // FS_NEW_OPT_MAPSIZE
    if (!ForkserverWrite(__afl_map_size)) {
        EMU_LOG_ERROR("Failed to write map size to forkserver controller");
        return false;
    }

    // FS_NEW_OPT_SHDMEM_FUZZ
    // Nothing to send

    // Send welcome
    if (!ForkserverWrite(version)) {
        EMU_LOG_ERROR("Failed to write welcome to forkserver controller");
        return false;
    }

    if (!MapFuzzShm()) {
        return false;
    }

    forkserverRunning = true;

    pid_t childPid = 0;
    ChildMessage lastChildMessage = ChildMessage::Exit;
    while (true) {
        // Check if the last run timed out
        auto timedOut = ForkserverRead<uint32_t>();
        if (!timedOut) {
            EMU_LOG_ERROR("Failed to read timed out from forkserver controller: {}", errno);
            return false;
        }

        // EMU_LOG_DEBUG("Last run {:}", *timedOut ? "timed out" : "didn't time out");

        // Handle race condition between afl-fuzz and stopped child
        if (*timedOut && lastChildMessage != ChildMessage::Exit) {
            EMU_LOG_WARN("Timed out without child exit");

            lastChildMessage = ChildMessage::Exit;
            if (::waitpid(childPid, nullptr, 0) < 0) {
                EMU_LOG_ERROR("Failed to wait for child: {}", errno);
            }

            ::close(childPipe[0]);
            ::close(parentPipe[1]);
        }

        if (lastChildMessage == ChildMessage::Exit) {
            // Create a pipe to receive messages from the child process
            if (::pipe(childPipe) == -1) {
                EMU_LOG_ERROR("Failed to create child pipe: {}", errno);
                return false;
            }

            // Create a pipe to send messages to the child process
            if (::pipe(parentPipe) == -1) {
                EMU_LOG_ERROR("Failed to create parent pipe: {}", errno);
                return false;
            }

            // Spawn a new child
            childPid = ::fork();
            if (childPid < 0) {
                EMU_LOG_ERROR("fork() failed");
                return false;
            }

            if (childPid == 0) {
                // We're the child, start running code
                // std::println("Hello world from child");

                ::close(FORKSRV_FD);
                ::close(FORKSRV_FD + 1);
                ::close(childPipe[0]); // Close receiving end of the pipe
                ::close(parentPipe[1]); // Close sending end of the pipe

                // Make sure forkserver doesn't give up
                // TODO can this be (re)moved?
                std::memset(__afl_area_ptr, 0, __afl_map_size);
                __afl_area_ptr[0] = 1;
                return true;
            }

            // Close sending end of the pipe
            ::close(childPipe[1]);
            ::close(parentPipe[0]);
        } else {
            // If we have a waiting child we can simply resume it by writing to the pipe
            int val = 0;
            if (::write(parentPipe[1], &val, sizeof(val)) != sizeof(val)) {
                EMU_LOG_ERROR("Failed to resume child: {}", errno);
            }
        }

        // We're the parent, tell forkserver controller about the child
        if (!ForkserverWrite<uint32_t>(childPid)) {
            EMU_LOG_ERROR("Failed to write childPid to forkserver controller: {}", errno);
            return false;
        }

        // Read status message from child
        if (::read(childPipe[0], &lastChildMessage, sizeof(lastChildMessage)) != sizeof(lastChildMessage)) {
            // Child closed writing end of the pipe, assume it exited without sending a message
            // lastChildMessage = ChildMessage::Exit;
            // TODO let's treat this as a crash, just in case we miss any reproducable crashes
            lastChildMessage = ChildMessage::Crash;
        }

        if (lastChildMessage != ChildMessage::Next) {
            // Wait for child to exit and send forkserver controller the status
            // We don't use the status from here as the status to send to AFL,
            // this would mean we would also notify AFL for crashes of the emulator itself
            if (::waitpid(childPid, nullptr, 0) < 0) {
                EMU_LOG_ERROR("Failed to wait for child: {}", errno);
                // Uh oh, we might have a zombie child now, let's continue on anyway
            }

            // Close remaning pipes
            ::close(childPipe[0]);
            ::close(parentPipe[1]);
        }

        uint32_t childStatus = lastChildMessage == ChildMessage::Crash ? GetValidWIFSIGNALED() : 0;
        if (!ForkserverWrite<uint32_t>(childStatus)) {
            EMU_LOG_ERROR("Failed to write childStatus to forkserver controller: {}", errno);
            return false;
        }

        // Anything other than Next, means the child exited
        if (lastChildMessage != ChildMessage::Next) {
            lastChildMessage = ChildMessage::Exit;
        }
    }

    return true;
}

bool afl::ForkserverRunning()
{
    return forkserverRunning;
}

std::span<const uint8_t> afl::GetFuzzData()
{
    return { __afl_fuzz_ptr, *__afl_fuzz_len };
}

void afl::UpdateCodeCoverage(uint32_t address)
{
    if (!__afl_area_ptr) {
        return;
    }

    uint32_t currentLocation = hash64(address) & (__afl_map_size - 1);
    __afl_area_ptr[currentLocation ^ prevLocation]++;
    prevLocation = currentLocation >> 1;
}

void afl::UpdateCmpCov(uint32_t address, uint32_t arg1, uint32_t arg2, uint32_t size)
{
    if (!__afl_area_ptr) {
        return;
    }

    // TODO can we reach sizes > 32?
    if (size > 32) {
        EMU_LOG_WARN("CmpCov: size > 32");
        size = 32;
    }

    uint32_t currentLocation = hash64(address) & (__afl_map_size - 1);
    size_t len = size / 8;

    // Don't need to add coverage for single byte comparisons
    if (len <= 1) {
        return;
    }

    // clamp len
    if (currentLocation + len >= __afl_map_size) {
        currentLocation -= len;
    }

    for (size_t i = len - 1; i > 0; i--) {
        uint32_t mask = (0xffu << i * 8);

        if ((arg1 & mask) != (arg2 & mask)) {
            break;
        }

        __afl_area_ptr[(currentLocation + (i - 1)) ^ prevLocation]++;
    }
}

void afl::StartLoop()
{
    // Wait for parent
    int val = 0;
    if (::read(parentPipe[0], &val, sizeof(val)) != sizeof(val)) {
        EMU_FATAL("Parent dead? (wait for parent)");
    }

    prevLocation = 0;

    // Make sure forkserver doesn't give up
    std::memset(__afl_area_ptr, 0, __afl_map_size);
    __afl_area_ptr[0] = 1;
}

void afl::EndLoop()
{
#if 1 // persistant fuzzing
    // Send next message to parent
    ChildMessage msg = ChildMessage::Next;
    if (::write(childPipe[1], &msg, sizeof(msg)) != sizeof(msg)) {
        EMU_FATAL("Parent dead? (send status)");
    }
#else
    ChildMessage msg = ChildMessage::Exit;
    ::write(childPipe[1], &msg, sizeof(msg));

    ::exit(0);
#endif
}

void afl::RaiseCrash()
{
    EMU_LOG_ERROR("Sending crash...");

    // Send crash message to parent
    ChildMessage msg = ChildMessage::Crash;
    ::write(childPipe[1], &msg, sizeof(msg));

    // TODO exit?
    ::exit(0);
}
