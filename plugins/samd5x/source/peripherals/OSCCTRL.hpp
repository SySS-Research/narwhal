#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace sam
{

class OSCCTRL : public emu::Peripheral
{
public:
    OSCCTRL();
    virtual ~OSCCTRL();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class STATUS : public arm::RegisterBase
    {
    public:
        inline STATUS(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(DFLLRDY, 8);
    };

    class DPLLSTATUS : public arm::RegisterBase
    {
    public:
        inline DPLLSTATUS(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(CLKRDY, 1);
        REGISTER_BASE_BIT(LOCK, 0);
    };

protected:
};

} // namespace sam
