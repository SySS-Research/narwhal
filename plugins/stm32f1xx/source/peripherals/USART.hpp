#pragma once

#include <Peripheral.hpp>
#include <connectors.hpp>
#include <arm.hpp>

#include <queue>

namespace stm
{

class USART : public emu::Peripheral {
public:
    USART();
    virtual ~USART();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class SR : public arm::RegisterBase
    {
    public:
        inline SR(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(CTS, 9);
        REGISTER_BASE_BIT(LBD, 8);
        REGISTER_BASE_BIT(TXE, 7);
        REGISTER_BASE_BIT(TC, 6);
        REGISTER_BASE_BIT(RXNE, 5);
        REGISTER_BASE_BIT(IDLE, 4);
        REGISTER_BASE_BIT(ORE, 3);
        REGISTER_BASE_BIT(NE, 2);
        REGISTER_BASE_BIT(FE, 1);
        REGISTER_BASE_BIT(PE, 0);
    };

    class CR1 : public arm::RegisterBase
    {
    public:
        inline CR1(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(UE, 13);
        REGISTER_BASE_BIT(M, 12);
        REGISTER_BASE_BIT(WAKE, 11);
        REGISTER_BASE_BIT(PCE, 10);
        REGISTER_BASE_BIT(PS, 9);
        REGISTER_BASE_BIT(PEIE, 8);
        REGISTER_BASE_BIT(TXEIE, 7);
        REGISTER_BASE_BIT(TCIE, 6);
        REGISTER_BASE_BIT(RXNEIE, 5);
        REGISTER_BASE_BIT(IDLEIE, 4);
        REGISTER_BASE_BIT(TE, 3);
        REGISTER_BASE_BIT(RE, 2);
        REGISTER_BASE_BIT(RWU, 1);
        REGISTER_BASE_BIT(SBK, 0);
    };

protected:
    void ProcessRx(uint32_t data);
    void IRQ();

    USARTDataConnectorView mTxConnectorView;
    USARTDataConnectorView mRxConnectorView;

    std::queue<uint32_t> mRxQueue;

    SR mSR;
    CR1 mCR1;
};

} // namespace stm
