#include "afl.hpp"
#include "common.hpp"

#include <optional>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <afl/types.h>

// TODO do these need to be exported? // old unicornafl doesn't export them so probably not
// extern "C"
namespace
{

static uint32_t __afl_fuzz_len_dummy;

uint8_t* __afl_area_ptr;

uint8_t* __afl_fuzz_ptr;
uint32_t* __afl_fuzz_len = &__afl_fuzz_len_dummy;

uint32_t __afl_map_size = MAP_SIZE;

} // extern "C"

namespace
{

enum class ChildMessage : uint32_t
{
    Exit,
    Crash,
};

bool forkserverRunning = false;

int childPipe[2];

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
    char* shmidStr = ::getenv(SHM_FUZZ_ENV_VAR);
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
    char* shmidStr = ::getenv(SHM_ENV_VAR);
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

    while (true) {
        // Check if the last run timed out
        auto timedOut = ForkserverRead<uint32_t>();
        if (!timedOut) {
            EMU_LOG_ERROR("Failed to read timed out from forkserver controller");
            return false;
        }

        // EMU_LOG_DEBUG("Last run {:}", *timedOut ? "timed out" : "didn't time out");

        // Create a pipe to receive messages from the child process
        if (::pipe(childPipe) == -1) {
            EMU_LOG_ERROR("Failed to create child pipe");
            return false;
        }

        // Spawn a new child
        pid_t childPid = ::fork();
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

            // Make sure forkserver doesn't give up
            // TODO can this be (re)moved?
            std::memset(__afl_area_ptr, 0, __afl_map_size);
            __afl_area_ptr[0] = 1;
            return true;
        }

        // Close sending end of the pipe
        ::close(childPipe[1]);

        // We're the parent, tell forkserver controller about the child
        if (!ForkserverWrite<uint32_t>(childPid)) {
            EMU_LOG_ERROR("Failed to write childPid to forkserver controller");
            return false;
        }

        // Read status message from child
        ChildMessage message;
        if (::read(childPipe[0], &message, sizeof(message)) != sizeof(message)) {
            // Child closed writing end of the pipe, assume it exited without sending a message
            message = ChildMessage::Exit;
        }

        // Wait for child to exit and send forkserver controller the status
        // We don't use the status from here as the status to send to AFL,
        // this would mean we would also notify AFL for crashes of the emulator itself
        if (::waitpid(childPid, nullptr, 0) < 0) {
            EMU_LOG_ERROR("Failed to wait for child");
            return false; 
        }

        uint32_t childStatus = message == ChildMessage::Crash ? GetValidWIFSIGNALED() : 0;
        if (!ForkserverWrite<uint32_t>(childStatus)) {
            EMU_LOG_ERROR("Failed to write childStatus to forkserver controller");
            return false;
        }

        // Close remaning pipe
        close(childPipe[0]);
    }

    return true;
}

bool afl::ForkserverRunning()
{
    return forkserverRunning;
}

std::span<uint8_t> afl::GetFuzzData()
{
    return { __afl_fuzz_ptr, *__afl_fuzz_len };
}

void afl::UpdateCodeCoverage(uint32_t address)
{
    // From https://aflplus.plus/docs/technical_details/
    // TODO replace with proper hashing? See https://github.com/AFLplusplus/unicornafl/blob/7b79cd88c61efc1cddef89406aee2fe25ff83d54/src/hash.rs

    if (!__afl_area_ptr) {
        return;
    }

    static uint32_t prevLocation = 0;
    uint32_t currentLocation = ((address >> 4) ^ (address << 8)) & (__afl_map_size - 1);
    __afl_area_ptr[currentLocation ^ prevLocation]++;
    prevLocation = currentLocation >> 1;
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
