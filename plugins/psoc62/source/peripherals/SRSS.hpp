#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace psoc
{

class SRSS : public emu::Peripheral
{
public:
    SRSS();
    virtual ~SRSS();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class FLL_STATUS : public arm::RegisterBase
    {
    public:
        inline FLL_STATUS(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(CCO_READY, 2);
        REGISTER_BASE_BIT(UNLOCK_OCCURRED, 1);
        REGISTER_BASE_BIT(LOCKED, 0);
    };

    class PLL_STATUS : public arm::RegisterBase
    {
    public:
        inline PLL_STATUS(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(UNLOCK_OCCURRED, 1);
        REGISTER_BASE_BIT(LOCKED, 0);
    };

protected:
};

} // namespace psoc
