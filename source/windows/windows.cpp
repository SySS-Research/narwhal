#include "windows.hpp"
#include "Registry.hpp"

#include "EmulationStateWindow.hpp"
#include "DisassemblyWindow.hpp"
#include "NodeEditorWindow.hpp"
#include "SerialConsoleWindow.hpp"

void windows::Register()
{
    emu::Registry::RegisterWindow<EmulationStateWindow>();
    emu::Registry::RegisterWindow<DisassemblyWindow>();
    emu::Registry::RegisterWindow<NodeEditorWindow>();
    // SerialConsoleWindow is registered upon SerialConsole device creation
    // TODO not great
    // emu::Registry::RegisterWindow<SerialConsoleWindow>();
}
