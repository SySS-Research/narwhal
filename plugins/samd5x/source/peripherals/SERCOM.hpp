#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace sam
{

class SERCOM : public emu::Peripheral
{
public:
    SERCOM();
    virtual ~SERCOM();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class INTFLAG : public arm::RegisterBase
    {
    public:
        inline INTFLAG(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(RXC, 2);
        REGISTER_BASE_BIT(TXC, 1);
        REGISTER_BASE_BIT(DRE, 0);
    };

protected:
};

} // namespace sam
