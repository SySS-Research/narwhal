#include "PHY.hpp"

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

namespace
{

int TapAlloc(char* dev)
{
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
       return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (*dev) {
       strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, static_cast<void*>(&ifr))) < 0) {
       close(fd);
       return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}

} // namespace

PHY::PHY() : Device(),
 mTapFd(-1),
 mPCAP(),
 mPCAPDumper(),
 mLinkConnectorView("link"),
 mSMIConnectorView("smi"),
 mTxConnectorView("tx"),
 mRxConnectorView("rx"),
 mCONTROL(0b0011000100000000),
 mSTATUS(0b0111100001001001)
{
    const char* narwhalIf = std::getenv("NARWHAL_TAPIF");
    if (narwhalIf) {
        // Open tap interface
        char ifname[IFNAMSIZ];
        strncpy(ifname, narwhalIf, IFNAMSIZ);
        mTapFd = TapAlloc(ifname);
        if (mTapFd < 0) {
            EMU_LOG_ERROR("Failed to open tap");
        } else {
            // Make fd non blocking
            fcntl(mTapFd, F_SETFL, fcntl(mTapFd, F_GETFL, 0) | O_NONBLOCK);
        }
    }

    mPCAP = pcap_open_dead(DLT_EN10MB, 65535);
    mPCAPDumper = pcap_dump_open(mPCAP, "dump.pcap");

    GetInputConnectors().Add(mLinkConnectorView);
    GetInputConnectors().Add(mSMIConnectorView);
    GetInputConnectors().Add(mTxConnectorView);
    GetOutputConnectors().Add(mRxConnectorView);

    mLinkConnectorView.OnCall([this](bool state) {
        if (state) {
            mSTATUS.setLINK_STATUS();
            mSTATUS.setAUTO_N_COMPL();
        } else {
            mSTATUS.clearLINK_STATUS();
            mSTATUS.clearAUTO_N_COMPL();
        }
    });

    mSMIConnectorView.OnCall([this](uint8_t reg, std::optional<uint16_t> value) -> uint16_t {
        // HACK: ETH tells PHY to poll for data over SMI here
        if (reg == 0xff) {
            UpdateTap();
            return 0;
        }

        if (value) {
            OnRegWrite(reg, *value);
            return 0;
        } else {
            return OnRegRead(reg);
        }
    });

    mTxConnectorView.OnCall([this](std::span<const uint8_t> data) {
        OnMIIData(std::move(data));
    });
}

PHY::~PHY()
{
    pcap_dump_close(mPCAPDumper);
    pcap_close(mPCAP);
}

void PHY::Init()
{
}

uint16_t PHY::OnRegRead(uint8_t reg)
{
    switch (reg) {
        case 0x00: // CONTROL
            return mCONTROL;
        case 0x01: // STATUS
            return mSTATUS;
        case 0x02: // PHYID1
            return 0x0181;
        case 0x03: // PHYID2
            return 0xb8a0; // dm9162iep
        case 0x11: { // DSCSR (DAVICOM specific)
            DSCSR dscsr(0);
            dscsr.set_100FDX(); // 100M full duplex
            return dscsr;
        }
    };

    EMU_LOG_WARN("Unknown read from register 0x{:x}", reg);
    return 0;
}

void PHY::OnRegWrite(uint8_t reg, uint16_t value)
{
    switch (reg) {
        case 0x00: // CONTROL
            mCONTROL = value;
            break;
        default:
            EMU_LOG_WARN("Unknown write to register 0x{:x}, value = 0x{:x}", reg, value);
            break;
    }
}

void PHY::OnMIIData(std::span<const uint8_t> data)
{
    // Save transfered data to pcap
    struct pcap_pkthdr hdr;
    gettimeofday(&hdr.ts, NULL);
    hdr.len = data.size();
    hdr.caplen = data.size();
    pcap_dump((u_char*) mPCAPDumper, &hdr, data.data());

    write(mTapFd, data.data(), data.size());
}

void PHY::UpdateTap()
{
    std::vector<uint8_t> buf(0x1000);
    const int len = read(mTapFd, buf.data(), buf.size());
    if (len > 0) {
        // Save received data to pcap
        struct pcap_pkthdr hdr;
        gettimeofday(&hdr.ts, NULL);
        hdr.len = len;
        hdr.caplen = len;
        pcap_dump((u_char*) mPCAPDumper, &hdr, buf.data());

        // Transfer data to ETH
        mRxConnectorView(std::span{buf.data(), static_cast<size_t>(len)});
    }
}
