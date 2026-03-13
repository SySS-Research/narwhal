#include "common.hpp"
#include "Emulator.hpp"
#include "Registry.hpp"
#include "plugins/Plugin.hpp"

#include "peripherals/BACKUP.hpp"
#include "peripherals/CPUSS.hpp"
#include "peripherals/SCB.hpp"
#include "peripherals/SRSS.hpp"

INITIALIZE_PLUGIN("psoc62")
{
    emu::Registry::RegisterPeripheral<psoc::BACKUP>("emu.peripherals.psoc62.backup");
    emu::Registry::RegisterPeripheral<psoc::CPUSS>("emu.peripherals.psoc62.cpuss");
    emu::Registry::RegisterPeripheral<psoc::SCB>("emu.peripherals.psoc62.scb");
    emu::Registry::RegisterPeripheral<psoc::SRSS>("emu.peripherals.psoc62.srss");

    return true;
}

ON_EMULATION_START(emulator)
{
    EMU_LOG_DEBUG("Hello world from psoc62 plugin");
}
