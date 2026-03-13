#include "common.hpp"
#include "Emulator.hpp"
#include "plugins/Plugin.hpp"

INITIALIZE_PLUGIN("TestPlugin")
{
    return true;
}

ON_EMULATION_START(emulator)
{
    EMU_LOG_DEBUG("Hello world from test plugin");

    emulator->PrintContext();
}
