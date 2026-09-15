#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class TIM : public emu::Peripheral {
public:
    TIM();
    virtual ~TIM();

    virtual void Update() override;

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class DIER : public arm::RegisterBase16
    {
    public:
        inline DIER(uint32_t value) : arm::RegisterBase16(value) {}

        REGISTER_BASE_BIT(TDE, 14);

        REGISTER_BASE_BIT(CC4DE, 12);
        REGISTER_BASE_BIT(CC3DE, 11);
        REGISTER_BASE_BIT(CC2DE, 10);
        REGISTER_BASE_BIT(CC1DE, 9);
        REGISTER_BASE_BIT(UDE, 8);

        REGISTER_BASE_BIT(TIE, 6);

        REGISTER_BASE_BIT(CC4IE, 4);
        REGISTER_BASE_BIT(CC3IE, 3);
        REGISTER_BASE_BIT(CC2IE, 2);
        REGISTER_BASE_BIT(CC1IE, 1);
        REGISTER_BASE_BIT(UIE, 0);
    };

    class SR : public arm::RegisterBase16
    {
    public:
        inline SR(uint32_t value) : arm::RegisterBase16(value) {}

        REGISTER_BASE_BIT(CC4OF, 12);
        REGISTER_BASE_BIT(CC3OF, 11);
        REGISTER_BASE_BIT(CC2OF, 10);
        REGISTER_BASE_BIT(CC1OF, 9);

        REGISTER_BASE_BIT(TIF, 6);

        REGISTER_BASE_BIT(CC4IF, 4);
        REGISTER_BASE_BIT(CC3IF, 3);
        REGISTER_BASE_BIT(CC2IF, 2);
        REGISTER_BASE_BIT(CC1IF, 1);
        REGISTER_BASE_BIT(UIF, 0);
    };

    class EGR : public arm::RegisterBase16
    {
    public:
        inline EGR(uint32_t value) : arm::RegisterBase16(value) {}

        REGISTER_BASE_BIT(BG, 7);
        REGISTER_BASE_BIT(TG, 6);
        REGISTER_BASE_BIT(COMG, 5);
        REGISTER_BASE_BIT(CC4G, 4);
        REGISTER_BASE_BIT(CC3G, 3);
        REGISTER_BASE_BIT(CC2G, 2);
        REGISTER_BASE_BIT(CC1G, 1);
        REGISTER_BASE_BIT(UG, 0);
    };

protected:
    void IRQ();

    DIER mDIER;
    SR mSR;
    EGR mEGR;
    uint16_t mCNT;
};

} // namespace stm
