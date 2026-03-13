#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class FLASH : public emu::Peripheral {
public:
    FLASH();
    virtual ~FLASH();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class SCAR : public arm::RegisterBase
    {
    public:
        inline SCAR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(DMES, 31);
        REGISTER_BASE_RANGE(SEC_AREA_END, 27, 16);
        REGISTER_BASE_RANGE(SEC_AREA_START, 11, 0);
    };

protected:
    uint32_t mACR;
};

} // namespace stm
