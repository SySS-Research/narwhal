#include "SERCOM.hpp"

namespace sam
{

SERCOM::SERCOM() : Peripheral()
{
}

SERCOM::~SERCOM()
{
}

uint32_t SERCOM::Read(uint32_t offset, uint32_t size)
{
    // TODO USART vs SPI vs I2C

    switch (offset) {
        case 0x18: { // Interrupt Flag Status and Clear
            INTFLAG flag(0);
            flag.setTXC();
            flag.setRXC();
            return flag;
        }
    };

    EMU_LOG_MMIO("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void SERCOM::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        default:
            EMU_LOG_MMIO("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace sam
