#pragma once

#include "common.hpp"

#define INITIALIZE_PLUGIN(name)                     \
    extern "C" uint32_t GetSupportedVersion() {     \
        return 0; /* TODO */                        \
    }                                               \
    extern "C" const char* GetName() {              \
        return name;                                \
    }                                               \
    extern "C" bool Initialize()

#define DEINITIALIZE_PLUGIN() \
    extern "C" void Deinitialize()

#define ON_EMULATION_START(emulator) \
    extern "C" void EmulationStart(emu::Emulator* emulator)

#define ON_EMULATION_FINISH(emulator) \
    extern "C" void EmulationFinish(emu::Emulator* emulator)

namespace emu
{

class Emulator;

class Plugin
{
public:
    Plugin(const std::string& path);
    ~Plugin();

    bool Load();

    const char* GetName();
    void EmulationStart(emu::Emulator* emulator);
    void EmulationFinish(emu::Emulator* emulator);

    using GetSupportedVersionFn = uint32_t (*)(void);
    using GetNameFn = const char* (*)(void);
    using InitializeFn = bool (*)(void);
    using DeinitializeFn = bool (*)(void);
    using EmulationStartFn = void (*)(emu::Emulator*);
    using EmulationFinishFn = void (*)(emu::Emulator*);

private:
    std::string mPath;
    void* mHandle;

    GetSupportedVersionFn mGetSupportedVersion;
    GetNameFn mGetName;
    InitializeFn mInitialize;
    DeinitializeFn mDeinitialize;
    EmulationStartFn mEmulationStart;
    EmulationFinishFn mEmulationFinish;
};

} // namespace emu
