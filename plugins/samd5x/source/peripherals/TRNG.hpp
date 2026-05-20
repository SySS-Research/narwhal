#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

namespace sam
{

class TRNG : public emu::Peripheral
{
public:
    TRNG();
    virtual ~TRNG();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class CTRLA : public arm::RegisterBase8
    {
    public:
        inline CTRLA(uint8_t value) : arm::RegisterBase8(value) {}

        REGISTER_BASE_BIT(RUNSTDBY, 6);
        REGISTER_BASE_BIT(ENABLE, 1);
    };

    class INTFLAG : public arm::RegisterBase8
    {
    public:
        inline INTFLAG(uint8_t value) : arm::RegisterBase8(value) {}

        REGISTER_BASE_BIT(DATARDY, 0);
    };

protected:
    mbedtls_entropy_context mEntropy;
    mbedtls_ctr_drbg_context mCtrDrbg;

    CTRLA mCTRLA;

    uint32_t GetRandom();
};

} // namespace sam
