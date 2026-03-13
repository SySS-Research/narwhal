#include "common.hpp"
#include "Emulator.hpp"
#include "Registry.hpp"
#include "plugins/Plugin.hpp"

#include "peripherals/ADC.hpp"
#include "peripherals/CRYP.hpp"
#include "peripherals/FLASH.hpp"
#include "peripherals/FMC.hpp"
#include "peripherals/GPIO.hpp"
#include "peripherals/MDMA.hpp"
#include "peripherals/OTG.hpp"
#include "peripherals/PWR.hpp"
#include "peripherals/RCC.hpp"
#include "peripherals/SDMMC.hpp"
#include "peripherals/USART.hpp"

void RegisterSTM32H730()
{
    emu::Registry::RegisterPeripheral<stm::ADC>("emu.peripherals.stm32h730.adc");
    emu::Registry::RegisterPeripheral<stm::CRYP>("emu.peripherals.stm32h730.cryp");
    emu::Registry::RegisterPeripheral<stm::FLASH>("emu.peripherals.stm32h730.flash");
    emu::Registry::RegisterPeripheral<stm::FMC>("emu.peripherals.stm32h730.fmc");
    emu::Registry::RegisterPeripheral<stm::GPIO>("emu.peripherals.stm32h730.gpio");
    emu::Registry::RegisterPeripheral<stm::MDMA>("emu.peripherals.stm32h730.mdma");
    emu::Registry::RegisterPeripheral<stm::OTG>("emu.peripherals.stm32h730.otg");
    emu::Registry::RegisterPeripheral<stm::PWR>("emu.peripherals.stm32h730.pwr");
    emu::Registry::RegisterPeripheral<stm::RCC>("emu.peripherals.stm32h730.rcc");
    emu::Registry::RegisterPeripheral<stm::SDMMC>("emu.peripherals.stm32h730.sdmmc");
    emu::Registry::RegisterPeripheral<stm::USART>("emu.peripherals.stm32h730.usart");
}

INITIALIZE_PLUGIN("STM32H7XXPlugin")
{
    RegisterSTM32H730();

    return true;
}

ON_EMULATION_START(emulator)
{
    EMU_LOG_DEBUG("Hello world from STM32H7XX");

    // RSS_exitSecureArea
    emulator->AddBreakpoint(0x1ff08a5c, 4, [emulator]() {
        uint32_t vtorAddress = emulator->GetRegister(arm::Register::R0);
        emulator->SetRegister(arm::Register::SP, emulator->ReadMemory<uint32_t>(vtorAddress));
        emulator->SetRegister(arm::Register::PC, emulator->ReadMemory<uint32_t>(vtorAddress + 4));
    });
}
