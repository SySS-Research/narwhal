#include "TRNG.hpp"

#include <botan/system_rng.h>

namespace sam
{

TRNG::TRNG() : Peripheral(),
    mCTRLA(0)
{
}

TRNG::~TRNG()
{
}

uint32_t TRNG::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x00: // Control A
            return mCTRLA;
        case 0x0A: { // Interrupt Flag Status and Clear
            INTFLAG flg(0);
            flg.setDATARDY();
            return flg;
        }
        case 0x20: // Output Data
            return GetRandom();
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void TRNG::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x00: // Control A
            mCTRLA = value;
            break;
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

uint32_t TRNG::GetRandom()
{
    uint32_t value;
    Botan::system_rng().randomize(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

} // namespace sam
