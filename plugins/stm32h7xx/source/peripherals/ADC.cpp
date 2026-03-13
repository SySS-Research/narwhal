#include "ADC.hpp"

namespace stm
{

ADC::ADC() : Peripheral()
{
}

ADC::~ADC()
{
}

uint32_t ADC::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x8: // CR
            return 1u << 28; // ADC_CR_ADVREGEN
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void ADC::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
