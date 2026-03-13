#include "FLASH.hpp"

namespace stm
{

FLASH::FLASH() : Peripheral(),
 mACR(0x37)
{
}

FLASH::~FLASH()
{
}

uint32_t FLASH::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // ACR
            return mACR;
        case 0x1C: // OPTSR_CUR
            return 0x2020bb00; // alarmo expected value
        case 0x30: { // SCAR_CUR
            SCAR scar(0);
            // alarmo expected values
            scar.setDMES();
            scar.setSEC_AREA_END(0x1ff);
            scar.setSEC_AREA_START(0);
            return scar;
        }
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void FLASH::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x0: // ACR
            mACR = value;
            return;
    };

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
