#pragma once

#include <Peripheral.hpp>
#include <connectors.hpp>

namespace stm
{

class USART : public emu::Peripheral {
public:
    USART();
    virtual ~USART();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

protected:
    USARTDataConnectorView mTxConnectorView;
};

} // namespace stm
