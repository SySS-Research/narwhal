#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>
#include <connectors.hpp>
#include <deque>

namespace stm
{

class SDMMC : public emu::Peripheral {
public:
    SDMMC();
    virtual ~SDMMC();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

public:
    class POWER : public arm::RegisterBase
    {
    public:
        inline POWER(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(DIRPOL, 4);
        REGISTER_BASE_BIT(VSWITCHEN, 3);
        REGISTER_BASE_BIT(VSWITCH, 2);
        REGISTER_BASE_RANGE(PWRCTRL, 1, 0);
    };

    class CMD : public arm::RegisterBase
    {
    public:
        inline CMD(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(CMDSUSPEND, 16);
        REGISTER_BASE_BIT(BOOTEN, 15);
        REGISTER_BASE_BIT(BOOTMODE, 14);
        REGISTER_BASE_BIT(DTHOLD, 13);
        REGISTER_BASE_BIT(CPSMEN, 12);
        REGISTER_BASE_BIT(WAITPEND, 11);
        REGISTER_BASE_BIT(WAITINT, 10);
        REGISTER_BASE_RANGE(WAITRESP, 9, 8);
        REGISTER_BASE_BIT(CMDSTOP, 7);
        REGISTER_BASE_BIT(CMDTRANS, 6);
        REGISTER_BASE_RANGE(CMDINDEX, 5, 0);
    };


    class DCTRL : public arm::RegisterBase
    {
    public:
        inline DCTRL(uint32_t value) : RegisterBase(value) {}

    };

    class STA : public arm::RegisterBase
    {
    public:
        inline STA(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(IDMABTC, 28);
        REGISTER_BASE_BIT(IDMATE, 27);
        REGISTER_BASE_BIT(CKSTOP, 26);
        REGISTER_BASE_BIT(VSWEND, 25);
        REGISTER_BASE_BIT(ACKTIMEOUT, 24);
        REGISTER_BASE_BIT(ACKFAIL, 23);
        REGISTER_BASE_BIT(SDIOIT, 22);
        REGISTER_BASE_BIT(BUSYD0END, 21);
        REGISTER_BASE_BIT(BUSYD0, 20);
        REGISTER_BASE_BIT(RXFIFOE, 19);
        REGISTER_BASE_BIT(TXFIFOE, 18);
        REGISTER_BASE_BIT(RXFIFOF, 17);
        REGISTER_BASE_BIT(TXFIFOF, 16);
        REGISTER_BASE_BIT(RXFIFOHF, 15);
        REGISTER_BASE_BIT(TXFIFOHE, 14);
        REGISTER_BASE_BIT(CPSMACT, 13);
        REGISTER_BASE_BIT(DPSMACT, 12);
        REGISTER_BASE_BIT(DABORT, 11);
        REGISTER_BASE_BIT(DBCKEND, 10);
        REGISTER_BASE_BIT(DHOLD, 9);
        REGISTER_BASE_BIT(DATAEND, 8);
        REGISTER_BASE_BIT(CMDSENT, 7);
        REGISTER_BASE_BIT(CMDREND, 6);
        REGISTER_BASE_BIT(RXOVERR, 5);
        REGISTER_BASE_BIT(TXUNDERR, 4);
        REGISTER_BASE_BIT(DTIMEOUT, 3);
        REGISTER_BASE_BIT(CTIMEOUT, 2);
        REGISTER_BASE_BIT(DCRCFAIL, 1);
        REGISTER_BASE_BIT(CCRCFAIL, 0);
    };

protected:
    void HandleCommand();

    MMCCommandConnectorView mCommandConnectorView;
    MMCDataConnectorView mTxDataConnectorView;
    MMCDataConnectorView mRxDataConnectorView;

    uint32_t mARG;
    CMD mCMD;
    uint32_t mRESPCMD;
    uint32_t mRESP[4];
    uint32_t mDTIME;
    uint32_t mDLEN;
    DCTRL mDCTRL;
    STA mSTA;
    std::deque<uint32_t> mFIFO;
};

} // namespace stm
