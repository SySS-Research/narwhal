#include "USART.hpp"

namespace stm
{

USART::USART() : Peripheral(),
 mTxConnectorView("tx")
{
    GetOutputConnectors().Add(mTxConnectorView);
}

USART::~USART()
{
}

uint32_t USART::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // SR
            return 0;
        case 0x4: // DR
            return 0; // TODO
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void USART::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x04: // DR
            if (mTxConnectorView) {
                mTxConnectorView(value);
            }
            break;
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace stm
