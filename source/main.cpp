#include "Application.hpp"

#include "devices/devices.hpp"
#include "windows/windows.hpp"
#include "FileLoader.hpp"
#include "afl.hpp"
#include "plugins/PluginManager.hpp"
#include "plugins/PythonManager.hpp"

#include <argparse/argparse.hpp>

int main(int argc, char const* argv[])
{
    argparse::ArgumentParser program("narwhal");

    program.add_argument("config")
        .help("Path to the configuration file to use");    

    program.add_argument("--loglevel")
        .help("Set the log level (default: info)")
        .choices("debug", "info", "warn", "error");  

    program.add_argument("--headless")
        .help("Start the emulator without a GUI")
        .flag();

    program.add_argument("--trace")
        .help("Trace all instructions to stdout (slow)")
        .flag();

    program.add_argument("--gdbstub")
        .help("Start the emulator with the debugger (halts at the first instruction and waits for a connection)")
        .flag();

    program.add_argument("--ezcov")
        .help("Output EZCOV coverage to file.");

    program.add_argument("--drcov")
        .help("Output DRCOV coverage to file.");

    program.add_argument("--generate-pystub")
        .help("Generate python stubs to folder.");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Set log level as soon as possible
    if (auto logLevel = program.present("--loglevel")) {
        static std::map<std::string, emu::LogType> logLevels = {
            { "debug", emu::LogType::Debug },
            { "info", emu::LogType::Info },
            { "warn", emu::LogType::Warn },
            { "error", emu::LogType::Error },
        };
        emu::SetLogLevel(logLevels.at(*logLevel));
    }

    // Try to load from current plugins folder first
    emu::PluginManager::LoadPlugins("plugins");
    // Try to load from build folder
    emu::PluginManager::LoadPlugins("build/plugins");

    // Initializing python plugins takes a long time, do this before forking for fuzzing
    emu::PythonManager::Init();

    if (auto output = program.present("--generate-pystub")) {
        emu::PythonManager::GenerateStub("emu", *output);
        EMU_LOG_INFO("Generated emu stubs to {}", *output);
        return 0;
    } else {
        emu::PythonManager::LoadPlugins("scripts");
    }

    // Register all of our things
    devices::Register();
    windows::Register();
    FileLoader::RegisterLoaders();

    // TODO where would be best to start the forkserver?
    if (program["--headless"] == true && afl::StartForkserver()) {
        EMU_LOG_DEBUG("Forkserver started");
    }

    // Create the application
    auto app = std::make_unique<emu::Application>();
    // TODO this is not great
    emu::Application::Set(app.get());
    return app->Run(program);
}

// ImGUI/SDL introduces a lot of leaks so get rid of this for now
extern "C" const char* __asan_default_options() { return "detect_leaks=0"; }
