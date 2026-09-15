#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class ADC : public emu::Peripheral {
public:
    ADC();
    virtual ~ADC();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class SR : public arm::RegisterBase
    {
    public:
        inline SR(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(STRT, 4);
        REGISTER_BASE_BIT(JSTRT, 3);
        REGISTER_BASE_BIT(JEOC, 2);
        REGISTER_BASE_BIT(EOC, 1);
        REGISTER_BASE_BIT(AWD, 0);
    };

protected:
};

} // namespace stm
