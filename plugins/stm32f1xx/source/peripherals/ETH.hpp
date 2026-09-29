#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>
#include <connectors.hpp>

#include <queue>

namespace stm
{

class ETH : public emu::Peripheral {
public:
    ETH();
    virtual ~ETH();

    virtual void Update() override;

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

    virtual void SaveState(emu::State& state) override;
    virtual void RestoreState(emu::State& state) override;

public:
    class MACCR : public arm::RegisterBase
    {
    public:
        inline MACCR(uint32_t value) : arm::RegisterBase(value) {}
    };

    class MACMIIAR : public arm::RegisterBase
    {
    public:
        inline MACMIIAR(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_RANGE(PA, 15, 11);
        REGISTER_BASE_RANGE(MR, 10, 6);
        REGISTER_BASE_RANGE(CR, 4, 2);
        REGISTER_BASE_BIT(MW, 1);
        REGISTER_BASE_BIT(MB, 0);
    };

    class DMASR : public arm::RegisterBase
    {
    public:
        inline DMASR(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(TSTS, 29);
        REGISTER_BASE_BIT(PMTS, 28);
        REGISTER_BASE_BIT(MMCS, 27);

        REGISTER_BASE_RANGE(EBS, 25, 23);
        REGISTER_BASE_RANGE(TPS, 22, 20);
        REGISTER_BASE_RANGE(RPS, 19, 17);
        REGISTER_BASE_BIT(NIS, 16);
        REGISTER_BASE_BIT(ERS, 15);
        REGISTER_BASE_BIT(FBES, 13);

        REGISTER_BASE_BIT(ETS, 10);
        REGISTER_BASE_BIT(RWTS, 9);
        REGISTER_BASE_BIT(RPSS, 8);
        REGISTER_BASE_BIT(RBUS, 7);
        REGISTER_BASE_BIT(RS, 6);
        REGISTER_BASE_BIT(TUS, 5);
        REGISTER_BASE_BIT(ROS, 4);
        REGISTER_BASE_BIT(TJTS, 3);
        REGISTER_BASE_BIT(TBUS, 2);
        REGISTER_BASE_BIT(TPSS, 1);
        REGISTER_BASE_BIT(TS, 0);
    };

    class DMAOMR : public arm::RegisterBase
    {
    public:
        inline DMAOMR(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(DTCEFD, 26);
        REGISTER_BASE_BIT(RSF, 25);
        REGISTER_BASE_BIT(DFRF, 24);

        REGISTER_BASE_BIT(TSF, 21);
        REGISTER_BASE_BIT(FTF, 20);

        REGISTER_BASE_RANGE(TTC, 16, 14);
        REGISTER_BASE_BIT(ST, 13);

        REGISTER_BASE_BIT(FEF, 7);
        REGISTER_BASE_BIT(FUGF, 6);

        REGISTER_BASE_RANGE(RTC, 4, 3);
        REGISTER_BASE_BIT(OSF, 2);
        REGISTER_BASE_BIT(SR, 1);
    };

    class DMAIER : public arm::RegisterBase
    {
    public:
        inline DMAIER(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(NISE, 16);
        REGISTER_BASE_BIT(AISE, 15);
        REGISTER_BASE_BIT(ERIE, 14);
        REGISTER_BASE_BIT(FBEIE, 13);

        REGISTER_BASE_BIT(ETIE, 10);
        REGISTER_BASE_BIT(RWTIE, 9);
        REGISTER_BASE_BIT(RPSIE, 8);
        REGISTER_BASE_BIT(RBUIE, 7);
        REGISTER_BASE_BIT(RIE, 6);
        REGISTER_BASE_BIT(TUIE, 5);
        REGISTER_BASE_BIT(ROIE, 4);
        REGISTER_BASE_BIT(TJTIE, 3);
        REGISTER_BASE_BIT(TBUIE, 2);
        REGISTER_BASE_BIT(TPSIE, 1);
        REGISTER_BASE_BIT(TIE, 0);
    };

    class TDES0 : public arm::RegisterBase
    {
    public:
        inline TDES0(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(OWN, 31);
        REGISTER_BASE_BIT(IC, 30);
        REGISTER_BASE_BIT(LS, 29);
        REGISTER_BASE_BIT(FS, 28);
        REGISTER_BASE_BIT(DC, 27);
        REGISTER_BASE_BIT(DP, 26);
        REGISTER_BASE_BIT(TTSE, 25);

        REGISTER_BASE_RANGE(CIC, 23, 22);
        REGISTER_BASE_BIT(TER, 21);
        REGISTER_BASE_BIT(TCH, 20);

        REGISTER_BASE_BIT(TTSS, 17);
        REGISTER_BASE_BIT(IHE, 16);
        REGISTER_BASE_BIT(ES, 15);
        REGISTER_BASE_BIT(JT, 14);
        REGISTER_BASE_BIT(FF, 13);
        REGISTER_BASE_BIT(IPE, 12);
        REGISTER_BASE_BIT(LCA, 11);
        REGISTER_BASE_BIT(NC, 10);
        REGISTER_BASE_BIT(LCO, 9);
        REGISTER_BASE_BIT(EC, 8);
        REGISTER_BASE_BIT(VF, 7);
        REGISTER_BASE_RANGE(CC, 6, 3);
        REGISTER_BASE_BIT(ED, 2);
        REGISTER_BASE_BIT(UF, 1);
        REGISTER_BASE_BIT(DB, 0);
    };

    class TDES1 : public arm::RegisterBase
    {
    public:
        inline TDES1(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_RANGE(TBS2, 28, 16);
        REGISTER_BASE_RANGE(TBS1, 12, 0);
    };

    class RDES0 : public arm::RegisterBase
    {
    public:
        inline RDES0(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(OWN, 31);
        REGISTER_BASE_BIT(AFM, 30);
        REGISTER_BASE_RANGE(FL, 29, 16);
        REGISTER_BASE_BIT(ES, 15);
        REGISTER_BASE_BIT(DE, 14);
        REGISTER_BASE_BIT(SAF, 13);
        REGISTER_BASE_BIT(LE, 12);
        REGISTER_BASE_BIT(OE, 11);
        REGISTER_BASE_BIT(VLAN, 10);
        REGISTER_BASE_BIT(FS, 9);
        REGISTER_BASE_BIT(LS, 8);
        REGISTER_BASE_BIT(IPHCE, 7);
        REGISTER_BASE_BIT(LCO, 6);
        REGISTER_BASE_BIT(FT, 5);
        REGISTER_BASE_BIT(RWT, 4);
        REGISTER_BASE_BIT(RE, 3);
        REGISTER_BASE_BIT(DBE, 2); // DE -> DBE
        REGISTER_BASE_BIT(CE, 1);
        REGISTER_BASE_BIT(PCE, 0);
    };

    class RDES1 : public arm::RegisterBase
    {
    public:
        inline RDES1(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(DIC, 31);

        REGISTER_BASE_RANGE(RBS2, 28, 16);

        REGISTER_BASE_BIT(RER, 15);
        REGISTER_BASE_BIT(RCH, 14);

        REGISTER_BASE_RANGE(RBS1, 12, 0);
    };

protected:
    void HandleMACMIIAR();
    void ProcessTxDMA();
    void ProcessRxDMA();
    void IRQ();

    SMIConnectorView mSMIConnectorView;
    MIIDataConnectorView mTxConnectorView;
    MIIDataConnectorView mRxConnectorView;

    std::queue<std::vector<uint8_t>> mFrameQueue;
    bool mRxSuspended;

    MACCR mMACCR;
    MACMIIAR mMACMIIAR;
    uint32_t mMACMIIDR;

    uint32_t mDMARDLAR;
    uint32_t mDMATDLAR;
    DMASR mDMASR;
    DMAOMR mDMAOMR;
    DMAIER mDMAIER;
    uint32_t mDMACHTDR;
    uint32_t mDMACHRDR;
};

} // namespace stm
