#include "OSCCTRL.hpp"

namespace sam
{

OSCCTRL::OSCCTRL() : Peripheral()
{
}

OSCCTRL::~OSCCTRL()
{
}

uint32_t OSCCTRL::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x10: { // STATUS
            STATUS status(0);
            status.setDFLLRDY();
            return status;
        }
        // TODO something doesn't match with the datasheet here
        case 0x54: { // DPLL Statu
            DPLLSTATUS status(0);
            status.setCLKRDY();
            status.setLOCK();
            return status;
        }
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void OSCCTRL::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace sam
