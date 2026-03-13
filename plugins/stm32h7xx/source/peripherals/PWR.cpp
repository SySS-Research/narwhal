#include "PWR.hpp"

namespace stm
{

PWR::PWR() : Peripheral()
{
}

PWR::~PWR()
{
}

uint32_t PWR::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // CR1
            return 1u << 8u; // DBP
        case 0x4: // CSR1
            return 1u << 13u; // ACTVOSRDY
        case 0x18: // SRDCR
            return 1u << 13u; // VOSRDY
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void PWR::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
