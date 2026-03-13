#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace stm
{

class DMA : public emu::Peripheral {
public:
    DMA();
    virtual ~DMA();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    friend class Channel;
    class Channel
    {
    public:
        Channel(DMA* dma, int channelIdx);
        ~Channel();

        uint32_t Read(uint32_t offset, uint32_t size);
        void Write(uint32_t offset, uint32_t size, uint32_t value);

    public:
        class CCR : public arm::RegisterBase
        {
        public:
            inline CCR(uint32_t value) : RegisterBase(value) {}

            REGISTER_BASE_BIT(MEM2MEM, 14);
            REGISTER_BASE_RANGE(PL,    13, 12);
            REGISTER_BASE_RANGE(MSIZE, 11, 10);
            REGISTER_BASE_RANGE(PSIZE, 9,  8);
            REGISTER_BASE_BIT(MINC, 7);
            REGISTER_BASE_BIT(PINC, 6);
            REGISTER_BASE_BIT(CIRC, 5);
            REGISTER_BASE_BIT(DIR,  4);
            REGISTER_BASE_BIT(TEIE, 3);
            REGISTER_BASE_BIT(HTIE, 2);
            REGISTER_BASE_BIT(TCIE, 1);
            REGISTER_BASE_BIT(EN,   0);
        };

    private:
        void PerformTransfer();

        DMA* mDMA;
        int mChannelIdx;

        CCR mCCR;
        uint32_t mCNDTR;
        uint32_t mCPAR;
        uint32_t mCMAR;
    };

    class ISR : public arm::RegisterBase
    {
    public:
        inline ISR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(TCIF7, 25);
        REGISTER_BASE_BIT(TCIF6, 21);
        REGISTER_BASE_BIT(TCIF5, 17);
        REGISTER_BASE_BIT(TCIF4, 13);
        REGISTER_BASE_BIT(TCIF3, 9);
        REGISTER_BASE_BIT(TCIF2, 5);
        REGISTER_BASE_BIT(TCIF1, 1);
    };

protected:
    void SetChannelTransferComplete(int channel);

    std::vector<Channel> mChannels;

    ISR mISR;
};

} // namespace stm
