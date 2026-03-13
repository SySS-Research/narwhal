#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class RCC : public emu::Peripheral {
public:
    RCC();
    virtual ~RCC();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class CR : public arm::RegisterBase
    {
    public:
        inline CR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(PLL3RDY, 29);
        REGISTER_BASE_BIT(PLL3ON, 28);
        REGISTER_BASE_BIT(PLL2RDY, 27);
        REGISTER_BASE_BIT(PLL2ON, 26);
        REGISTER_BASE_BIT(PLL1RDY, 25);
        REGISTER_BASE_BIT(PLL1ON, 24);

        REGISTER_BASE_BIT(HSI48RDY, 13);
        REGISTER_BASE_BIT(HSI48ON, 12);

        REGISTER_BASE_BIT(HSIRDY, 2);
        REGISTER_BASE_BIT(HSION, 0);
    };

    class CFGR : public arm::RegisterBase
    {
    public:
        inline CFGR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_RANGE(SWS, 5, 3);
        REGISTER_BASE_RANGE(SW, 2, 0);
    };

    class PLLCKSELR : public arm::RegisterBase
    {
    public:
        inline PLLCKSELR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_RANGE(DIVM3, 25, 20);
        REGISTER_BASE_RANGE(DIVM2, 17, 12);
        REGISTER_BASE_RANGE(DIVM1, 9, 4);
        REGISTER_BASE_RANGE(PLLSRC, 1, 0);
    };

protected:
    CR mCR;
    CFGR mCFGR;
    PLLCKSELR mPLLCKSELR;
    uint32_t mPLLCFGR;
    uint32_t mPLL1DIVR;
    uint32_t mPLL1FRACR;
};

} // namespace stm
