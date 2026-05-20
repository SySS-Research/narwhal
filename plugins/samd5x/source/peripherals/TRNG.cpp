#include "TRNG.hpp"

#include <mbedtls/entropy.h>

namespace sam
{

TRNG::TRNG() : Peripheral(),
    mEntropy(),
    mCtrDrbg(),
    mCTRLA(0)
{
    mbedtls_entropy_init(&mEntropy);
    mbedtls_ctr_drbg_init(&mCtrDrbg);

    mbedtls_ctr_drbg_seed(&mCtrDrbg, mbedtls_entropy_func, &mEntropy, nullptr, 0);
}

TRNG::~TRNG()
{
    mbedtls_ctr_drbg_free(&mCtrDrbg);
    mbedtls_entropy_free(&mEntropy);
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
    mbedtls_ctr_drbg_random(&mCtrDrbg, reinterpret_cast<unsigned char*>(&value), sizeof(value));
    return value;
}

} // namespace sam
