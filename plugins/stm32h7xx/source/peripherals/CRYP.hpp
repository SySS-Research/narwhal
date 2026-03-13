#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>
#include <deque>

namespace stm
{

class CRYP : public emu::Peripheral {
public:
    CRYP();
    virtual ~CRYP();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class CR : public arm::RegisterBase
    {
    public:
        inline CR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_RANGE(NPBLB, 23, 20);
        REGISTER_BASE_RANGE(GCM_CCMPH, 17, 16);
        REGISTER_BASE_BIT(CRYPEN, 15);
        REGISTER_BASE_BIT(FFLUSH, 14);
        REGISTER_BASE_RANGE(KEYSIZE, 9, 8);
        REGISTER_BASE_RANGE(DATATYPE, 7, 6);
        REGISTER_BASE_RANGE(ALGOMODE, 5, 3);
        REGISTER_BASE_BIT(ALGODIR, 2);
    };

    class SR : public arm::RegisterBase
    {
    public:
        inline SR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(BUSY, 4);
        REGISTER_BASE_BIT(OFFU, 3);
        REGISTER_BASE_BIT(OFNE, 2);
        REGISTER_BASE_BIT(IFNF, 1);
        REGISTER_BASE_BIT(IFEM, 0);
    };

protected:
    void PerformCrypt();

    CR mCR;
    SR mSR;
    std::deque<uint32_t> mInputFIFO;
    std::deque<uint32_t> mOutputFIFO;
    uint32_t mKEY[8];
    uint32_t mIV[4];
};

} // namespace stm
