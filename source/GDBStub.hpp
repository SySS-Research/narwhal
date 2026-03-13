#pragma once

#include "common.hpp"

#include <thread>
#include <unordered_set>
#include <condition_variable>
#include <atomic>

namespace emu
{

class Emulator;

class GDBStub
{
public:
    GDBStub(uint16_t port);
    virtual ~GDBStub();

    bool Init(Emulator* emulator);

    void OnCode(uint32_t address);
    void OnWatchpoint(uint32_t address, WatchpointType type);
    void Break();

private:
    void Continue();
    void Step();
    void BlockIfHalted();

    void ServerThread(std::stop_token stoken);
    std::string ReadPacket();
    void HandlePacket(const std::string& cmd);

    void HandleQuery(const std::string& cmd);
    void HandleVPacket(const std::string& cmd);
    void HandleStopReason();
    void HandleReadGeneralRegisters();
    void HandleReadMemory(const std::string& cmd);
    void HandleInsertBreakpoint(const std::string& cmd);
    void HandleRemoveBreakpoint(const std::string& cmd);

    std::string EscapeData(const std::string data);
    void SendAck();
    void SendPacket(const std::string data);

    Emulator* mEmulator;

    int mServerSocket;
    uint16_t mPort;
    std::jthread mThread;
    int mClientSocket;

    std::atomic_bool mIsHalted;
    std::mutex mStateMutex;
    std::condition_variable mStateCond;

    bool mStopOnNextCode;
    std::unordered_set<uint32_t> mSoftwareBreakpoints;
    std::unordered_set<uint32_t> mHardwareBreakpoints;
};

} // namespace emu
