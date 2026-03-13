#include "devices.hpp"
#include "Registry.hpp"

#include "EMMC.hpp"
#include "SerialConsole.hpp"
#include "ST7789.hpp"

void devices::Register()
{
    emu::Registry::RegisterDevice<EMMC>("emu.devices.emmc");
    emu::Registry::RegisterDevice<SerialConsole>("emu.devices.console");
    emu::Registry::RegisterDevice<ST7789>("emu.devices.st7789");
}

