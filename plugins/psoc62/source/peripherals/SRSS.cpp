#include "SRSS.hpp"

namespace psoc
{

SRSS::SRSS() : Peripheral()
{
}

SRSS::~SRSS()
{
}

uint32_t SRSS::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x590: { // FLL_STATUS
            FLL_STATUS status(0);
            status.setLOCKED();
            status.setCCO_READY();
            return status;
        }
        case 0x640 ... 0x678: { // PLL_STATUSX
            PLL_STATUS status(0);
            status.setLOCKED();
            return status;
        }
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void SRSS::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace psoc
