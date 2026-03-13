#pragma once

#include <Peripheral.hpp>

namespace stm
{

class ADC : public emu::Peripheral {
public:
    ADC();
    virtual ~ADC();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;
};

} // namespace stm
