#include "MDMA.hpp"
#include "Emulator.hpp"

namespace stm
{

MDMA::MDMA() : Peripheral(),
 mChannels()
{
    mChannels.reserve(16);
    for (int i = 0; i < 16; i++) {
        mChannels.emplace_back(this, i);
    }
}

MDMA::~MDMA()
{
}

uint32_t MDMA::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // GISR0
            return 0;
        default: break;
    };

    // Find the channel this read belongs to
    if (offset >= 0x40) {
        size_t idx = (offset - 0x40) / 0x40;
        if (idx < mChannels.size()) {
            return mChannels[idx].Read((offset - 0x40) % 0x40, size);
        }
    }

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void MDMA::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    // Find the channel this write belongs to
    if (offset >= 0x40) {
        size_t idx = (offset - 0x40) / 0x40;
        if (idx < mChannels.size()) {
            mChannels[idx].Write((offset - 0x40) % 0x40, size, value);
            return;
        }
    }

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

MDMA::Channel::Channel(MDMA* mdma, int channelIdx)
 : mMDMA(mdma),
 mChannelIdx(channelIdx),
 mCISR(0),
 mCCR(0),
 mCBNDTR(0),
 mCSAR(),
 mCDAR()
{
}

MDMA::Channel::~Channel()
{
}

uint32_t MDMA::Channel::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x00: // CISR
            return mCISR;
        case 0x0C: // CCR
            return mCCR;
        case 0x14: // CBNDTR
            return mCBNDTR;
        case 0x18: // CSAR
            return mCSAR;
        case 0x1C: // CDAR
            return mCDAR;
    };

    EMU_LOG_WARN("(Channel {}) Unknown read at offset 0x{:x}, size = {}", mChannelIdx, offset, size);
    return 0;
}

void MDMA::Channel::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x00: // CISR
            mCISR = value;
            return;
        case 0x0C: // CCR
            mCCR = value;

            // Perform transfer
            if (mCCR.EN() && mCCR.SWRQ()) {
                PerformTransfer();
            }
            mCCR.clearSWRQ(); // Immediately clear write-only bit
            return;
        case 0x14: // CBNDTR
            mCBNDTR = value;
            return;
        case 0x18: // CSAR
            mCSAR = value;
            return;
        case 0x1C: // CDAR
            mCDAR = value;
            return;
    };

    EMU_LOG_WARN("(Channel {}) Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", mChannelIdx, offset, size, value);
}

void MDMA::Channel::PerformTransfer()
{
    uint32_t srcAddress = mCSAR;
    uint32_t dstAddress = mCDAR;
    uint32_t blockDataLength = mCBNDTR.BNDT();
    uint32_t blockCount = mCBNDTR.BRC() + 1;

    // TODO check repeat flags, size, mem->periph, mem->mem, etc,
    uint32_t totalCount = blockCount * blockDataLength;
    for (uint32_t i = 0; i < totalCount / 2; i++) {
        uint16_t val;
        mMDMA->mEmulator->ReadMemory(srcAddress, &val, sizeof(val));
        srcAddress += 2;

        // MDMA_LITTLE_BYTE_ENDIANNESS_EXCHANGE
        val = __builtin_bswap16(val);

        // TODO this means we're iterating over all MMIO for each write
        mMDMA->mEmulator->WriteMMIO(dstAddress, 2, val);
    }

    // Set transfer complete flags
    mCISR.setCCTCIF(); // Full transfer

    // TODO verify
    mCCR.clearEN();
}

} // namespace stm
