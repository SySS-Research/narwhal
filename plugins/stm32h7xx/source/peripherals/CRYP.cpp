#include "CRYP.hpp"
#include <botan/stream_cipher.h>
#include <cstring>

namespace stm
{

CRYP::CRYP() : Peripheral(),
 mCR(0),
 mSR(0x00000003),
 mInputFIFO(),
 mOutputFIFO()
{
}

CRYP::~CRYP()
{
}

uint32_t CRYP::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // CR
            return mCR;
        case 0x4: // SR
            return mSR;
        case 0x8: // DIN
            // TODO
            // When CRYP_DIN register is read:
            // • If CRYPEN = 0, the FIFO is popped, and then the data present in the Input FIFO are
            // returned, from the oldest one (first reading) to the newest one (last reading). The IFEM
            // flag must be checked before each read operation to make sure that the FIFO is not
            // empty.
            return 0;
        case 0xC: { // DOUT
            EMU_ASSERT(!mOutputFIFO.empty());

            uint32_t value = mOutputFIFO.front();
            mOutputFIFO.pop_front();

            // Check if FIFO is empty
            if (mOutputFIFO.empty()) {
                mSR.clearOFNE();

                // Reset input status once output is emptied
                mInputFIFO.clear();
                mSR.setIFNF();
                mSR.setIFEM();
            }
            return value;
        }
        case 0x20 ... 0x3F: // KXXX (write-only)
            EMU_LOG_WARN("Reading from write-only register");
            return 0;
        case 0x40 ... 0x4F: // IVXXX
            offset -= 0x20;
            return mIV[offset / 4];
    };

    EMU_LOG_MMIO("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void CRYP::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x0: // CR
            mCR = value;

            // Flush FIFO
            if (mCR.FFLUSH()) {
                mInputFIFO.clear();
                mOutputFIFO.clear();
                mCR.clearFFLUSH();
            }
            return;
        case 0x4: // SR (read-only)
            EMU_LOG_WARN("Writing to read only register");
            return;
        case 0x8: // DIN
            mSR.clearIFEM();
            mInputFIFO.push_back(value);

            if (mInputFIFO.size() >= 4) {
                PerformCrypt();

                // Input FIFO is now full
                mSR.clearIFNF();
            }
            return;
        case 0x20 ... 0x3F: // KXXX
            offset -= 0x20;
            mKEY[offset / 4] = __builtin_bswap32(value);
            return;
        case 0x40 ... 0x4F: // IVXXX
            offset -= 0x40;
            mIV[offset / 4] = __builtin_bswap32(value);
            return;
    };

    EMU_LOG_MMIO("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

void CRYP::PerformCrypt()
{
    // TODO encrypt, algorithm mode, keysize, etc.
    mOutputFIFO.clear();

    const auto cipher = Botan::StreamCipher::create_or_throw("CTR-BE(AES-128)");
    cipher->set_key(reinterpret_cast<uint8_t*>(&mKEY[4]), 16);

    // EMU_LOG_DEBUG("Key: {}", emu::BytesToHex(&mKEY[4], 16));
    // EMU_LOG_DEBUG("IV: {}", emu::BytesToHex(mIV, sizeof(mIV)));

    // Prepare iv
    cipher->set_iv(reinterpret_cast<uint8_t*>(&mIV[0]), sizeof(mIV));

    // Copy input into a linear buffer
    std::vector<uint32_t> inputData, outputData;
    std::move(mInputFIFO.begin(), mInputFIFO.end(), std::back_inserter(inputData));
    outputData.resize(mInputFIFO.size());

    // EMU_LOG_DEBUG("inputData: {} {}", inputData.size(), emu::BytesToHex(inputData.data(), inputData.size() * 4));

    cipher->cipher(reinterpret_cast<uint8_t*>(inputData.data()), reinterpret_cast<uint8_t*>(outputData.data()), mInputFIFO.size() * sizeof(uint32_t));

    // EMU_LOG_DEBUG("outputData: {} {}", outputData.size(), emu::BytesToHex(outputData.data(), outputData.size() * 4));

    // TODO Properly update the IV
    mIV[3] = __builtin_bswap32(__builtin_bswap32(mIV[3]) + mInputFIFO.size() / 4);

    std::move(outputData.begin(), outputData.end(), std::back_inserter(mOutputFIFO));

    // Notify that data is in the output fifo
    mSR.setOFNE();
}

} // namespace stm
