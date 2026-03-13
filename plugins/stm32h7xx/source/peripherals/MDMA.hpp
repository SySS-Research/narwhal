#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class MDMA : public emu::Peripheral {
public:
    MDMA();
    virtual ~MDMA();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    friend class Channel;
    class Channel
    {
    public:
        Channel(MDMA* mdma, int channelIdx);
        ~Channel();

        uint32_t Read(uint32_t offset, uint32_t size);
        void Write(uint32_t offset, uint32_t size, uint32_t value);

    public:
        class CISR : public arm::RegisterBase
        {
        public:
            inline CISR(uint32_t value) : RegisterBase(value) {}

            REGISTER_BASE_BIT(CLTCIF, 4);
            REGISTER_BASE_BIT(CBTIF,  3);
            REGISTER_BASE_BIT(CBRTIF, 2);
            REGISTER_BASE_BIT(CCTCIF, 1);
            REGISTER_BASE_BIT(CTEIF,  0);
        };

        class CCR : public arm::RegisterBase
        {
        public:
            inline CCR(uint32_t value) : RegisterBase(value) {}

            REGISTER_BASE_BIT(SWRQ, 16);
            REGISTER_BASE_BIT(EN, 0);
        };

        class CBNDTR : public arm::RegisterBase
        {
        public:
            inline CBNDTR(uint32_t value) : RegisterBase(value) {}

            REGISTER_BASE_RANGE(BRC,  31, 20);
            REGISTER_BASE_RANGE(BNDT, 16, 0);
        };

    private:
        void PerformTransfer();

        MDMA* mMDMA;
        int mChannelIdx;

        CISR mCISR;
        CCR mCCR;
        CBNDTR mCBNDTR;
        uint32_t mCSAR;
        uint32_t mCDAR;
    };

protected:
    std::vector<Channel> mChannels;
};

} // namespace stm
