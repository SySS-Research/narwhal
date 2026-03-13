#include "RCC.hpp"

namespace stm
{

RCC::RCC() : Peripheral(),
 mCR(0x00000001),
 mCFGR(0),
 mPLLCKSELR(0x02020200),
 mPLLCFGR(0x01FF0000),
 mPLL1DIVR(0x01010280),
 mPLL1FRACR()
{
}

RCC::~RCC()
{
}

uint32_t RCC::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // CR
            return mCR;
        case 0x10: // CFGR
            return mCFGR;
        case 0x28: // PLLCKSELR
            return mPLLCKSELR;
        case 0x2C: // PLLCFGR
            return mPLLCFGR;
        case 0x30: // PLL1DIVR
            return mPLL1DIVR;
        case 0x34: // PLL1FRACR
            return mPLL1FRACR;
        case 0x70: // BDCR
            return 1u << 1; // LSERDY
        case 0x74: // CSR
            return 0x2; // LSIRDY
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void RCC::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x0: // CR
            mCR = value;

            // Update ready flags based on enabled and disabled state
            mCR.HSION() ? mCR.setHSIRDY() : mCR.clearHSIRDY();
            mCR.HSI48ON() ? mCR.setHSI48RDY() : mCR.clearHSI48RDY();
            mCR.PLL1ON() ? mCR.setPLL1RDY() : mCR.clearPLL1RDY();
            mCR.PLL2ON() ? mCR.setPLL2RDY() : mCR.clearPLL2RDY();
            mCR.PLL3ON() ? mCR.setPLL3RDY() : mCR.clearPLL3RDY();
            return;
        case 0x10: // CFGR
            mCFGR = value;

            // Update status
            mCFGR.setSWS(mCFGR.SW());
            return;
        case 0x28: // PLLCKSELR
            mPLLCKSELR = value;
            return;
        case 0x2C: // PLLCFGR
            mPLLCFGR = value;
            return;
        case 0x30: // PLL1DIVR
            mPLL1DIVR = value;
            return;
        case 0x34: // PLL1FRACR
            mPLL1FRACR = value;
            return;
    };

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
