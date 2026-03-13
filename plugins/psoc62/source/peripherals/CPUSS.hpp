#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace psoc
{

class CPUSS : public emu::Peripheral
{
public:
    CPUSS();
    virtual ~CPUSS();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class CM4_CLOCK_CTL : public arm::RegisterBase
    {
    public:
        inline CM4_CLOCK_CTL(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_RANGE(FAST_INT_DIV, 15, 8);
    };

protected:
};

} // namespace psoc
