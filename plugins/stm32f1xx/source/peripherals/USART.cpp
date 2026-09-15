#include "USART.hpp"
#include "Emulator.hpp"

namespace stm
{

USART::USART() : Peripheral(),
 mTxConnectorView("tx"),
 mRxConnectorView("rx"),
 mRxQueue(),
 mSR(0),
 mCR1(0)
{
    GetOutputConnectors().Add(mTxConnectorView);
    GetInputConnectors().Add(mRxConnectorView);

    // Add received data to queue (emulated "shift register")
    mRxConnectorView.OnCall([this](uint32_t data) {
        ProcessRx(data);
    });

    // Transmit data register is always empty
    mSR.setTXE();
}

USART::~USART()
{
}

uint32_t USART::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // SR
            return mSR;
        case 0x4: { // RDR
            uint32_t data = mRxQueue.front();
            mRxQueue.pop();

            mSR.clearRXNE();
            return data;
        }
        case 0x0C: //CR1
            return mCR1;
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void USART::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x00: // SR
            mSR = value & 0b1101100000;

            // Process rx queue
            if (!mRxQueue.empty()) {
                mSR.setRXNE();
                if (mCR1.RXNEIE()) {
                    IRQ();
                }
            }
            break;
        case 0x04: // TDR
            if (mTxConnectorView) {
                mTxConnectorView(value);
            }
            break;
        case 0x0C: // CR1
            mCR1 = value;
            break;
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

void USART::ProcessRx(uint32_t data)
{
    mRxQueue.push(data);

    mSR.setRXNE();
    if (mCR1.RXNEIE()) {
        IRQ();
    }
}

void USART::IRQ()
{
    arm::Exception exception = arm::Exception::Invalid;

    // UART5
    if (GetStart() == 0x40005000) {
        exception = emu::from_underlying<arm::Exception>((0x114 / 4));
    }

    if (exception != arm::Exception::Invalid) {
        mEmulator->SetExceptionPending(exception);
    }
}

} // namespace stm
