#include "TIM.hpp"
#include "Emulator.hpp"

namespace stm
{

TIM::TIM() : Peripheral(),
 mDIER(0),
 mSR(0),
 mEGR(0),
 mCNT(0)
{
}

TIM::~TIM()
{
}

void TIM::Update()
{
    // TODO big hack
    if (mDIER.CC1IE()) {
        if (mCNT > 1) {
            mCNT--;
        } else if (mCNT == 1) {
            mCNT--;
            mSR.setCC1IF();

            IRQ();
        }
    }
}

uint32_t TIM::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0C: // DIER
            return mDIER;
        case 0x10: // SR
            return mSR;
    };

    EMU_LOG_MMIO("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void TIM::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x0C: // DIER
            mDIER = value;
            break;
        case 0x10: // SR
            mSR &= value;
            break;
        case 0x14:
            mEGR = value;
            if (mEGR.UG()) {
                mEGR.clearUG();

                // TODO hack
                mCNT = 8;
            }
            break;
        default:
            EMU_LOG_MMIO("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

void TIM::IRQ()
{
    arm::Exception exception = arm::Exception::Invalid;

    // TIM2
    if (GetStart() == 0x40000000) {
        exception = emu::from_underlying<arm::Exception>((0xB0 / 4));
    }

    if (exception != arm::Exception::Invalid) {
        mEmulator->SetExceptionPending(exception);
    }
}

} // namespace stm
