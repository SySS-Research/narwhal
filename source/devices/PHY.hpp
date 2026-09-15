#pragma once

#include "Device.hpp"
#include "arm.hpp"
#include "connectors.hpp"

#include <pcap/pcap.h>

class PHY : public emu::Device {
public:
    PHY();
    virtual ~PHY();

    virtual void Init() override;

public:
    class CONTROL : public arm::RegisterBase16
    {
    public:
        inline CONTROL(uint16_t value) : arm::RegisterBase16(value) {}

        REGISTER_BASE_BIT(RESET, 15);
        REGISTER_BASE_BIT(LOOPBACK, 14);
        REGISTER_BASE_BIT(SPEED_SEL, 13);
        REGISTER_BASE_BIT(AUTO_N_EN, 12);
        REGISTER_BASE_BIT(POWER_DOWN, 11);
        REGISTER_BASE_BIT(ISOLATE, 10);
        REGISTER_BASE_BIT(RESTART_AUTO_N, 9);
        REGISTER_BASE_BIT(FULL_DUPLEX, 8);
        REGISTER_BASE_BIT(COLL_TEST, 7);
    };

    class STATUS : public arm::RegisterBase16
    {
    public:
        inline STATUS(uint16_t value) : arm::RegisterBase16(value) {}

        REGISTER_BASE_BIT(T4_CAP, 15);
        REGISTER_BASE_BIT(TX_FDX_CAP, 14);
        REGISTER_BASE_BIT(TX_HDX_CAP, 13);
        REGISTER_BASE_BIT(FDX_CAP, 12);
        REGISTER_BASE_BIT(HDX_CAP, 11);

        REGISTER_BASE_BIT(PREAM_SUPR, 6);
        REGISTER_BASE_BIT(AUTO_N_COMPL, 5);
        REGISTER_BASE_BIT(REM_FAULT, 4);
        REGISTER_BASE_BIT(AUTO_N_CAP, 3);
        REGISTER_BASE_BIT(LINK_STATUS, 2);
        REGISTER_BASE_BIT(JABBER_DETECT, 1);
        REGISTER_BASE_BIT(EXTD_CAP, 0);
    };

    class DSCSR : public arm::RegisterBase16
    {
    public:
        inline DSCSR(uint16_t value) : arm::RegisterBase16(value) {}

        REGISTER_BASE_BIT(_100FDX, 15);
        REGISTER_BASE_BIT(_100HDX, 14);
        REGISTER_BASE_BIT(_10FDX, 13);
        REGISTER_BASE_BIT(_10HDX, 12);
        
        REGISTER_BASE_RANGE(PHYADR, 8, 4);
        REGISTER_BASE_RANGE(ANMB, 3, 0);
    };

protected:
    uint16_t OnRegRead(uint8_t reg);
    void OnRegWrite(uint8_t reg, uint16_t value);
    void OnMIIData(std::span<const uint8_t> data);
    void UpdateTap();

    int mTapFd;
    pcap_t* mPCAP;
    pcap_dumper_t* mPCAPDumper;

    GPIOConnectorView mLinkConnectorView;
    SMIConnectorView mSMIConnectorView;
    MIIDataConnectorView mTxConnectorView;
    MIIDataConnectorView mRxConnectorView;

    CONTROL mCONTROL;
    STATUS mSTATUS;
};
