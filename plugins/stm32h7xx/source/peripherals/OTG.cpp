#include "OTG.hpp"

namespace stm
{

OTG::OTG() : Peripheral(),
 mGRSTCTL(0x80000000)
{
}

OTG::~OTG()
{
}

uint32_t OTG::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x10: // OTG_GRSTCTL
            return mGRSTCTL;
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void OTG::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x10: // OTG_GRSTCTL
            mGRSTCTL = value;

            // Clear set-only bits
            mGRSTCTL.clearTXFFLSH();
            mGRSTCTL.clearRXFFLSH();
            mGRSTCTL.clearFCRST();
            mGRSTCTL.clearPSRST();
            mGRSTCTL.clearCSRST();

            mGRSTCTL.setAHBIDL();
            return;
    };

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
