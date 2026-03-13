#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace psoc
{

class BACKUP : public emu::Peripheral
{
public:
    BACKUP();
    virtual ~BACKUP();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class STATUS : public arm::RegisterBase
    {
    public:
        inline STATUS(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(WCO_OK, 2);
        REGISTER_BASE_BIT(RTC_BUSY, 0);
    };

protected:
};

} // namespace psoc
