#include "common.hpp"
#include "Emulator.hpp"
#include "Registry.hpp"
#include "plugins/Plugin.hpp"

#include "peripherals/ADC.hpp"
#include "peripherals/ETH.hpp"
#include "peripherals/DMA.hpp"
#include "peripherals/GPIO.hpp"
#include "peripherals/TIM.hpp"
#include "peripherals/USART.hpp"

INITIALIZE_PLUGIN("stm32f1xx")
{
    emu::Registry::RegisterPeripheral<stm::ADC>("emu.peripherals.stm32f1xx.adc");
    emu::Registry::RegisterPeripheral<stm::ETH>("emu.peripherals.stm32f1xx.eth");
    emu::Registry::RegisterPeripheral<stm::DMA>("emu.peripherals.stm32f1xx.dma");
    emu::Registry::RegisterPeripheral<stm::GPIO>("emu.peripherals.stm32f1xx.gpio");
    emu::Registry::RegisterPeripheral<stm::TIM>("emu.peripherals.stm32f1xx.tim");
    emu::Registry::RegisterPeripheral<stm::USART>("emu.peripherals.stm32f1xx.usart");

    return true;
}

ON_EMULATION_START(emulator)
{
    EMU_LOG_DEBUG("Hello world from stm32f1xx plugin");
}
