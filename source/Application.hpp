#pragma once

#include "common.hpp"
#include "Emulator.hpp"
#include "UserInterface.hpp"
#include "Peripheral.hpp"
#include "Device.hpp"
#include "Configuration.hpp"

#include <argparse/argparse.hpp>

namespace emu
{

class Application
{
public:
    Application();
    ~Application();

    static void Set(Application* application);
    static Application* Get();

    int Run(const argparse::ArgumentParser& args);

    Configuration* GetConfiguration() const { return mConfiguration.get(); }
    Emulator* GetEmulator() const { return mEmulator.get(); }
    UserInterface* GetUI() const { return mUI.get(); }

private:
    // Global application instance
    static inline Application* sInstance = nullptr;

private:
    std::shared_ptr<Configuration> mConfiguration;
    std::unique_ptr<Emulator> mEmulator;
    std::unique_ptr<UserInterface> mUI;

    std::shared_ptr<CoverageCollector> mCoverageCollector;
};

} // namespace emu
