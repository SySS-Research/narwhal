#pragma once

#include <Peripheral.hpp>
#include <connectors.hpp>

namespace stm
{

class FMC : public emu::Peripheral {
public:
    FMC();
    virtual ~FMC();

    virtual void Init() override;
    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

private:
    ST7789CommandConnectorView mCommandConnectorView;
    ST7789DataConnectorView mDataConnectorView;
};

} // namespace stm
