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
        case 0x0: // SR
            SR sr(0);
            sr.setEOC();
            return sr;
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void ADC::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x08: // CR2
            break;
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace stm
