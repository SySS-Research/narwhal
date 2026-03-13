#include "CPUSS.hpp"

namespace psoc
{

CPUSS::CPUSS() : Peripheral()
{
}

CPUSS::~CPUSS()
{
}

uint32_t CPUSS::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x08: { // CM4_CLOCK_CTL
            CM4_CLOCK_CTL ctl(0);
            ctl.setFAST_INT_DIV(255); // Set a really large devider to compensate for the slow emulation
            return ctl;
        }
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void CPUSS::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace psoc
