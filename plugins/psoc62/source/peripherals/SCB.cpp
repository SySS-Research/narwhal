#include "SCB.hpp"

namespace psoc
{

SCB::SCB() : Peripheral(),
 mTxConnectorView("tx")
{
    GetOutputConnectors().Add(mTxConnectorView);
}

SCB::~SCB()
{
}

uint32_t SCB::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {

    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void SCB::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x240: // TX_FIFO_WR
            if (mTxConnectorView) {
                mTxConnectorView(value);
            }
            break;
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace psoc
