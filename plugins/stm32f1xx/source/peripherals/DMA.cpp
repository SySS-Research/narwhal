#include "DMA.hpp"
#include "Emulator.hpp"

namespace stm
{

DMA::DMA() : Peripheral(),
 mChannels(),
 mISR(0)
{
    mChannels.reserve(7);
    for (int i = 0; i < 7; i++) {
        mChannels.emplace_back(this, i);
    }
}

DMA::~DMA()
{
}

uint32_t DMA::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // ISR
            return mISR;
        case 0x4: // IFCR (write-only)
            EMU_LOG_WARN("Reading from write-only register IFCR");
            return 0;
        default: break;
    };

    // Find the channel this read belongs to
    if (offset >= 0x08) {
        size_t idx = (offset - 0x08) / 0x14;
        if (idx < mChannels.size()) {
            return mChannels[idx].Read((offset - 0x08) % 0x14, size);
        }
    }

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void DMA::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x0: // ISR (read-only)
            EMU_LOG_WARN("Writing to read-only register ISR");
            return;
        case 0x4: // IFCR
            // TODO
            return;
        default: break;
    };

    // Find the channel this write belongs to
    if (offset >= 0x08) {
        size_t idx = (offset - 0x08) / 0x14;
        if (idx < mChannels.size()) {
            mChannels[idx].Write((offset - 0x08) % 0x14, size, value);
            return;
        }
    }

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

void DMA::SetChannelTransferComplete(int channel)
{
    switch (channel) {
        case 0:
            mISR.setTCIF1();
            break;
        case 1:
            mISR.setTCIF2();
            break;
        case 2:
            mISR.setTCIF3();
            break;
        case 3:
            mISR.setTCIF4();
            break;
        case 4:
            mISR.setTCIF5();
            break;
        case 5:
            mISR.setTCIF6();
            break;
        case 6:
            mISR.setTCIF7();
            break;
    };
}

DMA::Channel::Channel(DMA* dma, int channelIdx)
 : mDMA(dma),
 mChannelIdx(channelIdx),
 mCCR(0),
 mCNDTR(),
 mCPAR(),
 mCMAR()
{
}

DMA::Channel::~Channel()
{
}

uint32_t DMA::Channel::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x00: // CCR
            return mCCR;
        case 0x04: // CNDTR
            return mCNDTR;
        case 0x08: // CPAR
            return mCPAR;
        case 0x0C: // CMAR
            return mCMAR;
    };

    EMU_LOG_WARN("(Channel {}) Unknown read at offset 0x{:x}, size = {}", mChannelIdx, offset, size);
    return 0;
}

void DMA::Channel::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x00: // CCR
            mCCR = value;
            if (mCCR.EN()) {
                PerformTransfer();
                mCCR.clearEN(); // TODO this should be cleared by software according to reference manual
            }
            return;
        case 0x04: // CNDTR
            mCNDTR = value;
            return;
        case 0x08: // CPAR
            mCPAR = value;
            return;
        case 0x0C: // CMAR
            mCMAR = value;
            return;
    };

    EMU_LOG_WARN("(Channel {}) Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", mChannelIdx, offset, size, value);
}

void DMA::Channel::PerformTransfer()
{
    // TODO mCCR.DIR()
    uint32_t srcAddress = mCMAR;
    uint32_t dstAddress = mCPAR;

    // TODO size, mem2mem, circular mode, etc.
    for (uint32_t i = 0; i < mCNDTR; i++) {
        uint8_t val;
        mDMA->mEmulator->ReadMemory(srcAddress, &val, sizeof(val));
        if (mCCR.MINC()) {
            srcAddress += 1;
        }

        // TODO this means we're iterating over all MMIO for each write
        mDMA->mEmulator->WriteMMIO(dstAddress, 1, val);
        if (mCCR.PINC()) {
            dstAddress += 1;
        }
    }

    if (mCCR.TCIE()) {
        mDMA->SetChannelTransferComplete(mChannelIdx);
        mDMA->mEmulator->SetExceptionPending(emu::from_underlying<arm::Exception>((0x6C / 4) + mChannelIdx)); // TODO DMA2 vs 1
    }
}

} // namespace stm
