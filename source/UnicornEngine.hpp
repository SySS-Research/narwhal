#pragma once

#include "common.hpp"
#include "arm.hpp"

#include <unicorn/unicorn.h>
#include <functional>
#include <atomic>
#include <list>
#include <unordered_map>

namespace emu
{

class UnicornEngine
{
public:
    UnicornEngine(arm::CPUModel cpu);
    virtual ~UnicornEngine();

    void Run();
    void Stop();

    void SetRegister(arm::Register reg, uint32_t val);
    uint32_t GetRegister(arm::Register reg);

    using MMIOReadCallbackFn = std::function<uint32_t(uint32_t, uint32_t)>;
    using MMIOWriteCallbackFn = std::function<void(uint32_t, uint32_t, uint32_t)>;

    void MapMemory(uint32_t address, uint32_t size, MemoryFlags flags);
    void MapMMIO(uint32_t address, uint32_t size,
        MMIOReadCallbackFn readCallback,
        MMIOWriteCallbackFn writeCallback);

    bool WriteMemory(uint32_t address, const void* ptr, size_t size);
    bool ReadMemory(uint32_t address, void* ptr, size_t size);

    uintptr_t AddBreakpoint(uint32_t address, uint32_t size, std::function<void(void)> callback);
    void RemoveBreakpoint(uintptr_t handle);

    void AddWatchpoint(uint32_t address, uint32_t size, WatchpointType type, std::function<void(void)> callback);
    void RemoveWatchpoint(uint32_t address, uint32_t size, WatchpointType type);

    void OnPeriodicUpdate(std::function<void(void)> callback)
    {
        mPeriodicUpdateCallback = std::move(callback);
    }

    void OnUnmappedMemory(std::function<void(MemoryFlags, uint32_t, uint32_t, uint32_t)> callback)
    {
        mUnmappedMemoryCallback = std::move(callback);
    }

    void OnBlock(std::function<void(uint32_t, uint32_t)> callback)
    {
        mBlockCallback = std::move(callback);
    }

    void OnCode(std::function<void(uint32_t, uint32_t)> callback)
    {
        mCodeCallback = std::move(callback);
    }

    void OnException(std::function<void(arm::Exception)> callback)
    {
        mExceptionCallback = std::move(callback);
    }

    void OnExceptionReturn(std::function<void(arm::EXC_RETURN)> callback)
    {
        mExceptionReturnCallback = std::move(callback);
    }

protected:
    void CreateMissingWatchpointHooks();

    uc_engine* mEngine;
    uc_hook mUnmappedMemoryHook;
    uc_hook mBlockHook;
    uc_hook mCodeHook;
    uc_hook mIntrHook;

    std::atomic<bool> mPendingStop;

    std::function<void(void)> mPeriodicUpdateCallback;
    std::function<void(MemoryFlags, uint32_t, uint32_t, uint32_t)> mUnmappedMemoryCallback;
    std::function<void(uint32_t, uint32_t)> mBlockCallback;
    std::function<void(uint32_t, uint32_t)> mCodeCallback;
    std::function<void(arm::Exception)> mExceptionCallback;
    std::function<void(arm::EXC_RETURN)> mExceptionReturnCallback;
    std::vector<std::unique_ptr<MMIOReadCallbackFn>> mMMIOReadCallbacks;
    std::vector<std::unique_ptr<MMIOWriteCallbackFn>> mMMIOWriteCallbacks;

    struct Breakpoint {
        uint32_t address;
        uint32_t size;
        uc_hook hook;
        std::function<void(void)> callback;
    };
    std::unordered_map<uintptr_t, std::unique_ptr<Breakpoint>> mBreakpoints;

    struct Watchpoint {
        uint32_t address;
        uint32_t size;
        WatchpointType type;
        uc_hook hook;
        std::function<void(void)> callback;
    };
    std::list<std::unique_ptr<Watchpoint>> mWatchpoints;
    std::mutex mWatchpointMutex; // not sure if this mutex is actually needed, but just in case
    std::atomic_bool mUpdateMissingWatchpoints;
};

} // namespace emu
