#pragma once

#include "Plugin.hpp"

#include <list>

namespace emu
{

class PluginManager
{
public:
    PluginManager() = delete;

    static bool LoadPlugins(const std::string& path);

    static void StartPlugins(Emulator* emulator);

private:
    static inline std::list<Plugin> sPlugins;
};

} // namespace emu
