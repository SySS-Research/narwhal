#include "Plugin.hpp"

#include <dlfcn.h>

namespace emu
{

Plugin::Plugin(const std::string& path)
 : mPath(path),
 mHandle(nullptr),
 mGetSupportedVersion(nullptr),
 mGetName(nullptr),
 mInitialize(nullptr),
 mEmulationStart(nullptr),
 mEmulationFinish(nullptr)
{
}

Plugin::~Plugin()
{
    if (mHandle) {
        dlclose(mHandle);
    }
}

bool Plugin::Load()
{
    mHandle = dlopen(mPath.c_str(), RTLD_NOW);
    if (!mHandle) {
        EMU_LOG_ERROR("dlopen failed: {}", dlerror());
        return false;
    }

    *reinterpret_cast<void**>(&mGetSupportedVersion) = dlsym(mHandle, "GetSupportedVersion");
    if (!mGetSupportedVersion) {
        return false;
    }

    *reinterpret_cast<void**>(&mGetName) = dlsym(mHandle, "GetName");
    if (!mGetName) {
        return false;
    }

    *reinterpret_cast<void**>(&mInitialize) = dlsym(mHandle, "Initialize");
    if (!mInitialize) {
        return false;
    }

    *reinterpret_cast<void**>(&mDeinitialize) = dlsym(mHandle, "Deinitialize");
    *reinterpret_cast<void**>(&mEmulationStart) = dlsym(mHandle, "EmulationStart");
    *reinterpret_cast<void**>(&mEmulationFinish) = dlsym(mHandle, "EmulationFinish");

    if (mGetSupportedVersion() != 0) {
        EMU_LOG_ERROR("Unsupported plugin version");
        return false;
    }

    if (!mInitialize()) {
        return false;
    }

    EMU_LOG_INFO("Loaded plugin: {}", GetName());

    return true;
}

const char* Plugin::GetName()
{
    return mGetName();
}

void Plugin::EmulationStart(emu::Emulator* emulator)
{
    if (mEmulationStart) {
        mEmulationStart(emulator);
    }
}

void Plugin::EmulationFinish(emu::Emulator* emulator)
{
    if (mEmulationFinish) {
        mEmulationFinish(emulator);
    }
}

} // namespace emu
