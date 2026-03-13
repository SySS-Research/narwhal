#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class OTG : public emu::Peripheral {
public:
    OTG();
    virtual ~OTG();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class GRSTCTL : public arm::RegisterBase
    {
    public:
        inline GRSTCTL(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(AHBIDL, 31);
        REGISTER_BASE_BIT(DMAREQ, 30);
        REGISTER_BASE_RANGE(TXFNUM, 10, 6);
        REGISTER_BASE_BIT(TXFFLSH, 5);
        REGISTER_BASE_BIT(RXFFLSH, 4);
        REGISTER_BASE_BIT(FCRST, 2);
        REGISTER_BASE_BIT(PSRST, 1);
        REGISTER_BASE_BIT(CSRST, 0);
    };

protected:
    GRSTCTL mGRSTCTL;
};

} // namespace stm
