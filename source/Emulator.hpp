#pragma once

#include "common.hpp"
#include "arm.hpp"
#include "UnicornEngine.hpp"
#include "Peripheral.hpp"
#include "capstone.hpp"
#include "GDBStub.hpp"
#include "CoverageCollector.hpp"

#include <unordered_map>
#include <bitset>
#include <mutex>

namespace emu
{

class NVIC;
class SCB;

class Emulator
{
public:
    Emulator();
    virtual ~Emulator();
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;

    void Start();
    void Stop();
    bool IsRunning() const { return mIsRunning; }
    void Reset(uint32_t vtorAddress);

    void SetRegister(arm::Register reg, uint32_t val);
    uint32_t GetRegister(arm::Register reg);

    void MapMemory(uint32_t address, uint32_t size, MemoryFlags flags = MemoryFlags::RWX);

    bool WriteMemory(uint32_t address, const void* ptr, size_t size);
    bool WriteMemory(uint32_t address, const std::span<const uint8_t>& bytes);
    bool ReadMemory(uint32_t address, void* ptr, size_t size);
    bool ReadMemory(uint32_t address, const std::span<uint8_t>& bytes);

    bool WriteMMIO(uint32_t address, uint32_t size, uint32_t value);

    void RegisterPeripheral(std::shared_ptr<Peripheral> peripheral);
    // TODO
    void RegisterUnmanagedMMIO(uint32_t address, uint32_t size,
        UnicornEngine::MMIOReadCallbackFn readCallback,
        UnicornEngine::MMIOWriteCallbackFn writeCallback);

    template<std::integral T>
    bool WriteMemory(uint32_t address, T value)
    {
        return WriteMemory(address, &value, sizeof(value));
    }

    template<std::integral T>
    T ReadMemory(uint32_t address)
    {
        T value = 0;
        if (!ReadMemory(address, &value, sizeof(value))) {
            EMU_FATAL("Failed to read memory at address 0x{:08x}", address);
        }
        return value;
    }

    uintptr_t AddBreakpoint(uint32_t address, uint32_t size, std::function<void(void)> callback);
    void RemoveBreakpoint(uintptr_t handle);

    void AddWatchpoint(uint32_t address, uint32_t size, WatchpointType type);
    void RemoveWatchpoint(uint32_t address, uint32_t size, WatchpointType type);

    void SetExceptionPending(arm::Exception exception);

    void PrintContext(emu::LogType type = emu::LogType::Debug);
    std::optional<capstone::Instruction> Disassemble(uint32_t address);

    // TODO do we want to expose the engine just like this?
    UnicornEngine& GetEngine() { return mEngine; };

    // We expose this for NVIC/SCB since these are tightly coupled
    auto& GetNVIC() { return mNVIC; }
    auto& GetSCB() { return mSCB; }

    void SetTraceEnabled(bool enabled)
    {
        mTraceEnabled = enabled;
    }

    void SetCoverageCollector(std::shared_ptr<CoverageCollector> coverageCollector)
    {
        mCoverageCollector = coverageCollector;
    }

    void EnableGDBStub();

protected:
    struct MMIORegion {
        uint32_t start;
        uint32_t end;
        std::vector<std::shared_ptr<Peripheral>> peripherals;
    };

    struct UnmanagedMMIO {
        uint32_t start;
        uint32_t end;
        UnicornEngine::MMIOReadCallbackFn readCallback;
        UnicornEngine::MMIOWriteCallbackFn writeCallback;
    };

protected:
    void EmulationThread(std::stop_token stoken);
    void PeriodicUpdate();
    std::shared_ptr<MMIORegion> FindMMIORegionForPeripheral(std::shared_ptr<Peripheral> peripheral);
    std::shared_ptr<Peripheral> FindPeripheral(std::shared_ptr<MMIORegion> region, uint32_t address);

private:
    uint32_t OnMMIORegionRead(std::shared_ptr<MMIORegion> region, uint32_t offset, uint32_t size);
    void OnMMIORegionWrite(std::shared_ptr<MMIORegion> region, uint32_t offset, uint32_t size, uint32_t value);
    void OnUnmappedMemory(MemoryFlags type, uint32_t address, uint32_t size, uint32_t value);
    void OnCode(uint32_t address, uint32_t size);
    void OnException(arm::Exception exception);

protected:
    UnicornEngine mEngine;
    std::jthread mEmulationThread;
    std::atomic<bool> mIsRunning;
    std::condition_variable_any mRunningCond;
    std::mutex mRunningMutex;

    std::vector<std::shared_ptr<MMIORegion>> mMMIORegions;
    std::vector<UnmanagedMMIO> mUnmanagedMMIO;

    // CPU peripherals
    std::shared_ptr<NVIC> mNVIC;
    std::shared_ptr<SCB> mSCB;

    GDBStub mGDBStub;

    bool mTraceEnabled;
    std::shared_ptr<CoverageCollector> mCoverageCollector;
};

} // namespace emu
