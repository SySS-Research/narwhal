#include "common.hpp"
#include "Emulator.hpp"
#include "Registry.hpp"
#include "plugins/Plugin.hpp"

#include "peripherals/DMA.hpp"
#include "peripherals/USART.hpp"

INITIALIZE_PLUGIN("stm32f1xx")
{
    emu::Registry::RegisterPeripheral<stm::USART>("emu.peripherals.stm32f1xx.usart");
    emu::Registry::RegisterPeripheral<stm::DMA>("emu.peripherals.stm32f1xx.dma");

    return true;
}

ON_EMULATION_START(emulator)
{
    EMU_LOG_DEBUG("Hello world from stm32f1xx plugin");
}
