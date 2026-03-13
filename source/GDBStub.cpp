#include "GDBStub.hpp"
#include "Emulator.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <functional>

namespace
{

template<std::integral T>
T socketRead(int socket)
{
    T ret = 0;
    read(socket, &ret, sizeof(ret));
    return ret;
}

// https://sourceware.org/gdb/current/onlinedocs/gdb.html/ARM-Features.html
// https://github.com/bminor/binutils-gdb/blob/d92ae8c993111a01b75b0d72210fc861eca8ae33/gdb/features/Makefile#L276-L282
constexpr std::string_view targetXml = R"(<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target>
  <architecture>arm</architecture>
  <xi:include href="arm-m-profile.xml"/>
  <xi:include href="arm-m-system.xml"/>
</target>
)";

// TODO support non-m-profiles
// https://github.com/bminor/binutils-gdb/blob/d92ae8c993111a01b75b0d72210fc861eca8ae33/gdb/features/arm/arm-m-profile.xml
constexpr std::string_view armMProfileXml = R"(<?xml version="1.0"?>
<!-- Copyright (C) 2010-2025 Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-target.dtd">
<feature name="org.gnu.gdb.arm.m-profile">
  <reg name="r0" bitsize="32"/>
  <reg name="r1" bitsize="32"/>
  <reg name="r2" bitsize="32"/>
  <reg name="r3" bitsize="32"/>
  <reg name="r4" bitsize="32"/>
  <reg name="r5" bitsize="32"/>
  <reg name="r6" bitsize="32"/>
  <reg name="r7" bitsize="32"/>
  <reg name="r8" bitsize="32"/>
  <reg name="r9" bitsize="32"/>
  <reg name="r10" bitsize="32"/>
  <reg name="r11" bitsize="32"/>
  <reg name="r12" bitsize="32"/>
  <reg name="sp" bitsize="32" type="data_ptr"/>
  <reg name="lr" bitsize="32"/>
  <reg name="pc" bitsize="32" type="code_ptr"/>
  <reg name="xpsr" bitsize="32" regnum="25"/>
</feature>
)";

constexpr std::string_view armMSystemXml = R"(<?xml version="1.0"?>
<!-- Copyright (C) 2022-2025 Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-target.dtd">
<feature name="org.gnu.gdb.arm.m-system">
  <reg name="msp" bitsize="32" type="data_ptr"/>
  <reg name="psp" bitsize="32" type="data_ptr"/>
</feature>
)";

} // namespace

namespace emu
{

GDBStub::GDBStub(ushort port)
 : mEmulator(nullptr),
 mServerSocket(-1),
 mPort(port),
 mThread(),
 mClientSocket(-1),
 mIsHalted(),
 mStateMutex(),
 mStateCond(),
 mStopOnNextCode(false),
 mSoftwareBreakpoints(),
 mHardwareBreakpoints()
{
}

GDBStub::~GDBStub()
{
    shutdown(mClientSocket, SHUT_RDWR);
    close(mClientSocket);
    mClientSocket = -1;

    shutdown(mServerSocket, SHUT_RDWR);
    close(mServerSocket);
    mServerSocket = -1;

    // Release block in case halted
    Continue();
}

bool GDBStub::Init(Emulator* emulator)
{
    mEmulator = emulator;

    // Start halted
    mIsHalted = true;

    mServerSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (mServerSocket < 0) {
        EMU_LOG_ERROR("Failed to create socket");
        return false;
    }

    int reuseEnabled = 1;
    if (setsockopt(mServerSocket, SOL_SOCKET, SO_REUSEADDR, &reuseEnabled, sizeof(reuseEnabled)) == -1) {
        close(mServerSocket);
        return false;
    }

    int nodelayEnabled = 1;
    if (setsockopt(mServerSocket, IPPROTO_TCP, TCP_NODELAY, &nodelayEnabled, sizeof(nodelayEnabled)) == -1) {
        close(mServerSocket);
        return false;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(mPort);
    if (bind(mServerSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        close(mServerSocket);
        EMU_LOG_ERROR("Failed to bind socket");
        return false;
    }

    if (listen(mServerSocket, 1) == -1) {
        close(mServerSocket);
        return false;
    }

    mThread = std::jthread(&GDBStub::ServerThread, this);
    return true;
}

void GDBStub::OnCode(uint32_t address)
{
    if (mStopOnNextCode) {
        mIsHalted = true;
        mStopOnNextCode = false;

        // Notify client that we stopped
        SendPacket("S05");
    }

    if (mSoftwareBreakpoints.contains(address)) {
        mIsHalted = true;

        // Notify client that breakpoint was triggered
        SendPacket("T05swbreak:;");
    }

    if (mHardwareBreakpoints.contains(address)) {
        mIsHalted = true;

        // Notify client that breakpoint was triggered
        SendPacket("T05hwbreak:;");
    }

    BlockIfHalted();
}

void GDBStub::OnWatchpoint(uint32_t address, WatchpointType type)
{
    EMU_LOG_DEBUG("Watchpoint triggered {:x}, pc {:x}", address, mEmulator->GetRegister(arm::Register::PC));

    const char* reason = nullptr;
    switch (type) {
        case WatchpointType::Read:
            reason = "rwatch";
            break;
        case WatchpointType::Write:
            reason = "watch";
            break;
        case WatchpointType::Access:
            reason = "awatch";
            break;
    }

    // Notify client that watchpoint was triggered
    SendPacket(std::format("T05{}:{:08x};", reason, address));

    // Halt execution
    mIsHalted = true;
    BlockIfHalted();
}

void GDBStub::Break()
{
    // Notify client that we stopped
    SendPacket("S05");

    // Halt execution
    mIsHalted = true;
    BlockIfHalted();
}

void GDBStub::Continue()
{
    {
        std::scoped_lock lk(mStateMutex);

        mIsHalted = false;
    }

    mStateCond.notify_one();
}

void GDBStub::Step()
{
    mStopOnNextCode = true;
    Continue();
}

void GDBStub::BlockIfHalted()
{
    if (!mIsHalted) {
        return;
    }

    // Block code from executing while halted
    std::unique_lock lk(mStateMutex);
    mStateCond.wait(lk, [&]{ return !mIsHalted; });
}

void GDBStub::ServerThread(std::stop_token stoken)
{
    EMU_LOG_INFO("Listening for connections on port {}", mPort);

    mClientSocket = accept(mServerSocket, nullptr, nullptr);
    if (mClientSocket < 0) {
        EMU_LOG_ERROR("Failed to accept client");
        return;
    }

    while (!stoken.stop_requested()) {
        char prefix = socketRead<char>(mClientSocket);
        switch (prefix) {
            case '$': { // packet
                std::string packet = ReadPacket();
                // EMU_LOG_DEBUG("Received packet {}", packet);
                HandlePacket(packet);
            }
                break;
            case '\x00': // read error
                EMU_LOG_DEBUG("Read error");
                mThread.request_stop();
                break;
            case '\x03': // interrupt request
                EMU_LOG_DEBUG("Received interrupt request");
                mStopOnNextCode = true;
                break;
            case '+': // ack
            case '-': // nack
                // EMU_LOG_DEBUG("Received '{:c}'", prefix);
                break;
            default:
                EMU_LOG_WARN("Received unknown prefix 0x{:x}", prefix);
                break;
        }
    }
}

std::string GDBStub::ReadPacket()
{
    std::string packet;

    char c;
    uint8_t checksum = 0;
    while ((c = socketRead<char>(mClientSocket)) != '#') {
        packet += c;
        checksum += c;
    }

    std::string checksumStr;
    checksumStr += socketRead<char>(mClientSocket);
    checksumStr += socketRead<char>(mClientSocket);
    if (std::stoi(checksumStr, nullptr, 16) != checksum) {
        EMU_LOG_WARN("Received packet with invalid checksum");
    }

    return packet;
}

void GDBStub::HandlePacket(const std::string& cmd)
{
    // Acknowledge packet
    SendAck();

    char request = cmd[0];
    switch (request) {
        case 'q':
            HandleQuery(cmd);
            return;
        case 'v':
            HandleVPacket(cmd);
            return;
        case '?':
            HandleStopReason();
            return;
        case 'g':
            HandleReadGeneralRegisters();
            return;
        case 'm':
            HandleReadMemory(cmd);
            return;
        case 'Z':
            HandleInsertBreakpoint(cmd);
            return;
        case 'z':
            HandleRemoveBreakpoint(cmd);
            return;
        default:
            break;
    }

    EMU_LOG_WARN("Unknown Packet {}", cmd);
    SendPacket("");
}

void GDBStub::HandleQuery(const std::string& cmd)
{
    auto args = emu::SplitString(cmd, ':');
    if (args.empty()) {
        EMU_LOG_WARN("Invalid Query {}", cmd);
        SendPacket("");
        return;
    }

    if (args[0] == "qSupported") {
        // TODO verify
        std::string supported;
        supported += "PacketSize=4096";
        supported += ";qXfer:features:read+";
        // supported += ";qXfer:threads:read+";
        // supported += ";qXfer:libraries:read+";
        supported += ";swbreak+";
        supported += ";hwbreak+";
        supported += ";vContSupported+";
        SendPacket(supported);
        return;
    } else if (args[0] == "qXfer") {
        // Feature read
        if (args.size() >= 5 && args[1] == "features" && args[2] == "read") {
            auto annex = args[3];
            auto params = emu::SplitString(args[4], ',');
            auto offset = std::stoul(params[0], nullptr, 16);
            auto length = std::stoul(params[1], nullptr, 16);
            if (annex == "target.xml") {
                if (offset != 0 || length < targetXml.size()) {
                    EMU_FATAL("Not implemented");
                }

                SendPacket(std::string("l") + std::string(targetXml));
                return;
            } else if (annex == "arm-m-profile.xml") {
                if (offset != 0 || length < armMProfileXml.size()) {
                    EMU_FATAL("Not implemented");
                }

                SendPacket(std::string("l") + std::string(armMProfileXml));
                return;
            } else if (annex == "arm-m-system.xml") {
                if (offset != 0 || length < armMSystemXml.size()) {
                    EMU_FATAL("Not implemented");
                }

                SendPacket(std::string("l") + std::string(armMSystemXml));
                return;
            }
        }
    } else if (args[0] == "qAttached") {
        SendPacket("1"); // attached to existing process
        return;
    }

    EMU_LOG_WARN("Unknown/Invalid Query {}", cmd);
    SendPacket("");
}

void GDBStub::HandleVPacket(const std::string& cmd)
{
    auto args = emu::SplitString(cmd, ';');
    if (args.empty()) {
        EMU_LOG_WARN("Invalid vpacket {}", cmd);
        SendPacket("");
        return;
    }

    if (args[0] == "vCont?") {
        // TODO verify
        std::string supported;
        supported += "vCont";
        // continue
        supported += ";c";
        supported += ";C";
        // step
        supported += ";s";
        supported += ";S";
        SendPacket(supported);
        return;
    } else if (args[0] == "vCont" && args.size() > 1) {
        // Since we only have a single "thread" we only use the first state
        auto params = emu::SplitString(args[1], ':');

        // Decide if we should continue or single step
        if (params[0] == "c" || params[0][0] == 'C') {
            Continue();
        } else if (params[0] == "s" || params[0][0] == 'S') {
            Step();
        } else {
            EMU_LOG_WARN("Unimplemented vCont action {}", params[0]);
        }
        return;
    } else if (args[0] == "vMustReplyEmpty") {
        SendPacket("");
        return;
    } else if (args[0] == "vKill") {
        mEmulator->Stop(); // TODO
        SendPacket("OK");

        // Return from code hook to allow emulator to exit
        Continue();
        return;
    }

    EMU_LOG_WARN("Unknown vpacket {}", cmd);
    SendPacket("");
}

void GDBStub::HandleStopReason()
{
    SendPacket("S05");
}

void GDBStub::HandleReadGeneralRegisters()
{
    // Register list matching target.xml
    static const arm::Register registers[] = {
        arm::Register::R0, arm::Register::R1, arm::Register::R2, arm::Register::R3, 
        arm::Register::R4, arm::Register::R5, arm::Register::R6, arm::Register::R7, 
        arm::Register::R8, arm::Register::R9, arm::Register::R10, arm::Register::R11, 
        arm::Register::R12, arm::Register::SP, arm::Register::LR, arm::Register::PC,
        arm::Register::XPSR, arm::Register::MSP, arm::Register::PSP, 
    };

    std::string response;
    for (auto reg : registers) {
        // Read registers and byteswap into native order
        response += std::format("{:08X}", __builtin_bswap32(mEmulator->GetRegister(reg)));
    }

    SendPacket(response);
}

void GDBStub::HandleReadMemory(const std::string& cmd)
{
    auto params = emu::SplitString(cmd.c_str() + 1, ',');
    if (params.size() < 2) {
        EMU_LOG_WARN("Invalid memory read {}", cmd);
        SendPacket("");
        return;
    }

    auto address = std::stoul(params[0], nullptr, 16);
    auto length = std::stoul(params[1], nullptr, 16);

    std::vector<uint8_t> data(length);
    if (!mEmulator->ReadMemory(address, data)) {
        SendPacket("E01");
        return;
    }

    std::string response;
    for (auto b : data) {
        response += std::format("{:02X}", b);
    }
    SendPacket(response);
}

void GDBStub::HandleInsertBreakpoint(const std::string& cmd)
{
    auto params = emu::SplitString(cmd, ',');
    if (params.size() < 3) {
        EMU_LOG_WARN("Invalid breakpoint insert {}", cmd);
        SendPacket("");
        return;
    }

    auto address = std::stoul(params[1], nullptr, 16);
    auto kind = std::stoul(params[2], nullptr, 16);

    char type = params[0][1];
    switch (type) {
        case '0':
            mSoftwareBreakpoints.insert(address);
            break;
        case '1':
            mHardwareBreakpoints.insert(address);
            break;
        case '2':
            mEmulator->AddWatchpoint(address, kind, WatchpointType::Write);
            break;
        case '3':
            mEmulator->AddWatchpoint(address, kind, WatchpointType::Read);
            break;
        case '4':
            mEmulator->AddWatchpoint(address, kind, WatchpointType::Access);
            break;
        default:
            EMU_LOG_WARN("Invalid breakpoint type");
            SendPacket("");
            return;
    }

    SendPacket("OK");
}

void GDBStub::HandleRemoveBreakpoint(const std::string& cmd)
{
    auto params = emu::SplitString(cmd, ',');
    if (params.size() < 3) {
        EMU_LOG_WARN("Invalid breakpoint remove {}", cmd);
        SendPacket("");
        return;
    }

    auto address = std::stoul(params[1], nullptr, 16);
    auto kind = std::stoul(params[2], nullptr, 16);

    char type = params[0][1];
    switch (type) {
        case '0':
            mSoftwareBreakpoints.erase(address);
            break;
        case '1':
            mHardwareBreakpoints.erase(address);
            break;
        case '2':
            mEmulator->RemoveWatchpoint(address, kind, WatchpointType::Write);
            break;
        case '3':
            mEmulator->RemoveWatchpoint(address, kind, WatchpointType::Read);
            break;
        case '4':
            mEmulator->RemoveWatchpoint(address, kind, WatchpointType::Access);
            break;
        default:
            EMU_LOG_WARN("Invalid breakpoint type");
            SendPacket("");
            return;
    }

    SendPacket("OK");
}

std::string GDBStub::EscapeData(const std::string data)
{
    std::string escaped;
    for (auto c : data) {
        if (c == '#' || c == '$' || c == '}' || c == '*') {
            escaped += '}';
            escaped += static_cast<char>(c ^ 0x20);
        } else {
            escaped += c;
        }
    }

    return escaped;
}

void GDBStub::SendAck()
{
    send(mClientSocket, "+", 1, 0);
}

void GDBStub::SendPacket(const std::string data)
{
    std::string escaped = EscapeData(data);

    // Calculate checksum
    uint8_t checksum = 0;
    for (auto c : escaped) {
        checksum += c;
    }

    std::string packet = std::format("${}#{:02x}", escaped, checksum);
    send(mClientSocket, packet.data(), packet.size(), 0);
}

} // namespace emu
