#include "Application.hpp"
#include "Registry.hpp"
#include "plugins/PluginManager.hpp"
#include "plugins/PythonManager.hpp"
#include "afl.hpp"

#include <yaml-cpp/yaml.h>
#include <signal.h>

namespace
{

std::atomic_bool sKeepRunning = true;

// Only needed for headless mode, will be handled by SDL3 otherwise
void IntHandler(int dummy)
{
    sKeepRunning = false;
}

}

namespace emu
{

Application::Application()
 : mConfiguration(),
 mEmulator(),
 mUI()
{
    mEmulator = std::make_unique<Emulator>();

    // PluginManager::LoadPlugins("plugins");
}

Application::~Application()
{
}

void Application::Set(Application* application)
{
    sInstance = application;
}

Application* Application::Get()
{
    return sInstance;
}

int Application::Run(const argparse::ArgumentParser& args)
{
    if (args["--trace"] == true) {
        mEmulator->SetTraceEnabled(true);
    }

    if (args["--gdbstub"] == true) {
        mEmulator->EnableGDBStub();
    }

    if (args.present("--ezcov") || args.present("--drcov")) {
        mCoverageCollector = std::make_shared<CoverageCollector>();
        mEmulator->SetCoverageCollector(mCoverageCollector);
    }

    mConfiguration = Configuration::Load(args.get<std::string>("config"));
    if (!mConfiguration) {
        EMU_LOG_ERROR("Failed to load configuration");
        return 1;
    }

    for (auto memory : mConfiguration->GetMemoryMap()) {
        mEmulator->MapMemory(memory.start, (memory.end - memory.start) + 1, memory.flags);
    }

    for (auto loader : mConfiguration->GetMemoryLoaders()) {
        if (!loader.function(mEmulator.get(), loader.config)) {
            EMU_LOG_ERROR("Loader failed to load");
            return 1;
        }
    }

    mEmulator->Reset(mConfiguration->GetCpuVtor());

    // TODO Meh
    for (auto device : mConfiguration->GetDevices()) {
        device->Init();
    }

    for (auto peripheral : mConfiguration->GetPeripherals()) {
        mEmulator->RegisterPeripheral(peripheral);
        peripheral->Init();
    }

    PluginManager::StartPlugins(mEmulator.get());
    PythonManager::StartPlugins(mEmulator.get());

    if (args["--headless"] == true) {
        // Register interrupt handler to quit emulation loop
        signal(SIGINT, IntHandler);

        if (afl::ForkserverRunning()) {
            EMU_LOG_DEBUG("Forkserver running");

            auto input = afl::GetFuzzData();
            PythonManager::StartFuzzing(mEmulator.get(), input);
        } else {
            EMU_LOG_DEBUG("Forkserver not running");
        }

        EMU_LOG_DEBUG("Starting emulation");

        mEmulator->Start();

        // TODO
        while (mEmulator->IsRunning() && sKeepRunning)
            ;
    } else {
        mUI = std::make_unique<UserInterface>(mEmulator.get());
        mUI->Run();
        mUI.reset();
    }

    mEmulator->Stop();

    if (auto file = args.present("--ezcov")) {
        mCoverageCollector->WriteEZCOV(*file);
    }

    if (auto file = args.present("--drcov")) {
        mCoverageCollector->WriteDRCOV(*file);
    }

    return 0;
}

} // namespace emu
