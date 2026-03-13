#include "PluginManager.hpp"

#include <filesystem>

namespace emu
{

bool PluginManager::LoadPlugins(const std::string& path)
{
    if (!std::filesystem::exists(path)) {
        return false;
    }

    for (const auto& file : std::filesystem::directory_iterator(path)) {
        if (file.path().extension() == ".so") {
            sPlugins.emplace_back(file.path());
        }
    }

    for (auto& plugin : sPlugins) {
        if (!plugin.Load()) {
            EMU_LOG_ERROR("Failed to load plugin");
            // return false;
        }
    }

    return false;
}

void PluginManager::StartPlugins(Emulator* emulator)
{
    for (auto& plugin : sPlugins) {
        plugin.EmulationStart(emulator);
    }

}


} // namespace emu
