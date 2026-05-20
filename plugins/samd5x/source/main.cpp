#include "common.hpp"
#include "Emulator.hpp"
#include "Registry.hpp"
#include "plugins/Plugin.hpp"

#include "peripherals/OSCCTRL.hpp"
#include "peripherals/SERCOM.hpp"
#include "peripherals/TRNG.hpp"

INITIALIZE_PLUGIN("samd5x")
{
    emu::Registry::RegisterPeripheral<sam::OSCCTRL>("emu.peripherals.samd5x.oscctrl");
    emu::Registry::RegisterPeripheral<sam::SERCOM>("emu.peripherals.samd5x.sercom");
    emu::Registry::RegisterPeripheral<sam::TRNG>("emu.peripherals.samd5x.trng");

    return true;
}

ON_EMULATION_START(emulator)
{
    EMU_LOG_DEBUG("Hello world from samd5x plugin");
}
