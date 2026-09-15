#include "ETH.hpp"
#include "Emulator.hpp"
#include <cstring>

namespace
{

uint16_t InternetChecksum(const uint8_t* data, size_t length)
{
    uint32_t sum = 0;

    while (length >= 2) {
        uint16_t word = ((uint16_t) data[0] << 8) | data[1];

        sum += word;

        data += 2;
        length -= 2;
    }

    if (length == 1) {
        sum += ((uint16_t) data[0] << 8);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }

    return (uint16_t) (~sum);
}

uint16_t TransportChecksum(const uint8_t* ip_header, const uint8_t* payload, size_t payload_length)
{
    std::vector<uint8_t> data(12 + payload_length);

    // Prepare IP pseudo header
    memcpy(&data[0], ip_header + 12, 4);
    memcpy(&data[4], ip_header + 16, 4);
    data[8] = 0;
    data[9] = ip_header[9];
    data[10] = (uint8_t)(payload_length >> 8);
    data[11] = (uint8_t)(payload_length & 0xff);

    // Append payload
    memcpy(&data[12], payload, payload_length);

    return InternetChecksum(data.data(), data.size());
}

} // namespace

namespace stm
{

ETH::ETH() : Peripheral(),
 mSMIConnectorView("smi"),
 mTxConnectorView("tx"),
 mRxConnectorView("rx"),
 mFrameQueue(),
 mRxSuspended(false),
 mMACCR(0),
 mMACMIIAR(0),
 mMACMIIDR(0),
 mDMARDLAR(0),
 mDMATDLAR(0),
 mDMASR(0),
 mDMAOMR(0),
 mDMAIER(0),
 mDMACHTDR(0),
 mDMACHRDR(0)
{
    GetOutputConnectors().Add(mSMIConnectorView);
    GetOutputConnectors().Add(mTxConnectorView);
    GetInputConnectors().Add(mRxConnectorView);

    mRxConnectorView.OnCall([this](std::span<const uint8_t> data) {
        // Add received frame to queue
        std::vector<uint8_t> frame;
        frame.assign_range(data);
        mFrameQueue.push(frame);
    });
}

ETH::~ETH()
{
}

void ETH::Update()
{
    if (mDMAOMR.SR()) {
        // HACK: Regularly tell PHY to update TAP over SMI
        mSMIConnectorView(0xff, std::nullopt);

        // Since ProcessRxDMA is non-blocking, we also need to regularly call it to process incoming frames from the queue
        ProcessRxDMA();
    }
}

uint32_t ETH::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x00:
            return mMACCR;
        case 0x10: // MACMIIAR
            return mMACMIIAR;
        case 0x14:
            return mMACMIIDR & 0xffff;
        case 0x100C:
            return mDMARDLAR;
        case 0x1010:
            return mDMATDLAR;
        case 0x1014:
            return mDMASR;
        case 0x1018:
            return mDMAOMR;
        case 0x1048:
            return mDMACHTDR;
        case 0x104C:
            return mDMACHRDR;
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void ETH::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x00:
            mMACCR = value;
            break;
        case 0x10: // MACMIIAR
            mMACMIIAR = value;
            HandleMACMIIAR();
            break;
        case 0x14:
            mMACMIIDR = value & 0xffff;
            break;
        case 0x100C:
            mDMARDLAR = mDMACHRDR = value;
            break;
        case 0x1010:
            mDMATDLAR = mDMACHTDR = value;
            break;
        case 0x1004: // DMATPDR
            // Writing to this register means there's Transmit poll demand
            ProcessTxDMA();
            break;
        case 0x1008: // DMARPDR
            mRxSuspended = false;
            break;
        case 0x1014: { // DMASR
            DMASR dmasr(value);
            // Handle write to clear
            if (dmasr.TBUS()) {
                mDMASR.clearTBUS();
            }
            if (dmasr.RBUS()) {
                mDMASR.clearRBUS();
            }
            if (dmasr.RS()) {
                mDMASR.clearRS();
            }
            break;
        }
        case 0x1018:
            mDMAOMR = value;
            if (mDMAOMR.ST()) {
                ProcessTxDMA();
            }
            if (mDMAOMR.SR()) {
                mRxSuspended = false;
            }
            break;
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

void ETH::HandleMACMIIAR()
{
    // Check if we need to do something
    if (!mMACMIIAR.MB()) {
        return;
    }

    if (mMACMIIAR.PA() != 0) {
        EMU_LOG_ERROR("Invalid PA {}!", mMACMIIAR.PA());
    }

    // Check if write or read
    if (mMACMIIAR.MW()) {
        mSMIConnectorView(mMACMIIAR.MR(), mMACMIIDR);
    } else {
        mMACMIIDR = mSMIConnectorView(mMACMIIAR.MR(), std::nullopt);
    }

    // Clear busy bit
    mMACMIIAR.clearMB();
}

void ETH::ProcessTxDMA()
{
    while (true) {
        // Read descriptor words
        TDES0 tdes0(0);
        mEmulator->ReadMemory(mDMACHTDR, &tdes0, sizeof(tdes0));
        TDES1 tdes1(0);
        mEmulator->ReadMemory(mDMACHTDR + 0x4, &tdes1, sizeof(tdes1));
        uint32_t tdes2;
        mEmulator->ReadMemory(mDMACHTDR + 0x8, &tdes2, sizeof(tdes2));
        uint32_t tdes3;
        mEmulator->ReadMemory(mDMACHTDR + 0xC, &tdes3, sizeof(tdes3));

        // Check if DMA owns this descriptor, otherwise suspend
        if (!tdes0.OWN()) {
            mDMASR.setTBUS();
            break;
        }

        // Read frame data from memory
        std::vector<uint8_t> frameData(tdes1.TBS1());
        mEmulator->ReadMemory(tdes2, frameData);

        // Check if we need to insert checksums
        if (tdes0.CIC() == 0b11) {
            // Calculate IPv4 checksum
            if (frameData[12] == 0x08 && frameData[13] == 0x00) {
                uint8_t* ipHeader = &frameData[14];
                size_t ipHeaderLength = (ipHeader[0] & 0x0F) * 4;
                size_t totalLength = ((size_t)ipHeader[2] << 8) | ipHeader[3];

                uint16_t ipChecksum = InternetChecksum(ipHeader, ipHeaderLength);
                ipHeader[10] = (uint8_t)(ipChecksum >> 8);
                ipHeader[11] = (uint8_t)(ipChecksum & 0xFF);

                uint8_t* payload = ipHeader + ipHeaderLength;
                size_t payloadLength = totalLength - ipHeaderLength;
                switch (ipHeader[9]) {
                    case 1: { // ICMP
                        uint16_t checksum = InternetChecksum(payload, payloadLength);
                        payload[2] = (uint8_t)(checksum >> 8);
                        payload[3] = (uint8_t)(checksum & 0xFF);
                        break;
                    }
                    case 6: { // TCP
                        uint16_t checksum = TransportChecksum(ipHeader, payload, payloadLength);
                        payload[16] = (uint8_t)(checksum >> 8);
                        payload[17] = (uint8_t)(checksum & 0xff);
                        break;
                    }
                    case 17: { // UDP
                        size_t udpLength = ((size_t)payload[4] << 8) | payload[5];
                        uint16_t checksum = TransportChecksum(ipHeader, payload, udpLength);

                        // 0 is treated as 0xffff, since 0 means ignore checksum
                        if (checksum == 0) {
                            checksum = 0xffff;
                        }

                        payload[6] = (uint8_t)(checksum >> 8);
                        payload[7] = (uint8_t)(checksum & 0xff);
                        break;
                    }
                }
            }
        }

        mTxConnectorView(frameData);

        tdes0.clearOWN();
        mEmulator->WriteMemory(mDMACHTDR, &tdes0, sizeof(tdes0));

        // Proceed to next descriptor
        // TODO ring structure vs chain structure
        if (tdes0.TCH()) {
            mDMACHTDR = tdes3;
        }
    }
}

void ETH::ProcessRxDMA()
{
    if (mRxSuspended) {
        return;
    }

    bool fetchNext = false;

    while (true) {
        // Read descriptor words
        RDES0 rdes0(0);
        mEmulator->ReadMemory(mDMACHRDR, &rdes0, sizeof(rdes0));
        RDES1 rdes1(0);
        mEmulator->ReadMemory(mDMACHRDR + 0x4, &rdes1, sizeof(rdes1));
        uint32_t rdes2;
        mEmulator->ReadMemory(mDMACHRDR + 0x8, &rdes2, sizeof(rdes2));
        uint32_t rdes3;
        mEmulator->ReadMemory(mDMACHRDR + 0xC, &rdes3, sizeof(rdes3));

        // Check if DMA owns this descriptor, otherwise suspend
        if (!rdes0.OWN()) {
            if (fetchNext) {
                mDMASR.setRBUS();
            }
            mRxSuspended = true;
            break;
        }

        // If the frame queue is empy but we're not suspended we wait to be called again with more frames available
        if (mFrameQueue.empty()) {
            break;
        }

        // Obtain frame from front of queue
        const auto& data = mFrameQueue.front();

        // TODO handle multiple buffers
        size_t size = data.size();
        if (rdes1.RBS1() < size) {
            EMU_LOG_WARN("Truncating frame: {} < {}", rdes1.RBS1(), size);
            size = rdes1.RBS1();
        }
        mEmulator->WriteMemory(rdes2, data.data(), size);

        // TODO
        rdes0.clearOWN();
        rdes0.setFL(size + 4); // TODO include missing checksum
        rdes0.clearES();
        rdes0.setFS();
        rdes0.setLS();
        mEmulator->WriteMemory(mDMACHRDR, &rdes0, sizeof(rdes0));

        // Remove from queue
        mFrameQueue.pop();

        // Trigger receive interrupt
        mDMASR.setRS();
        IRQ();

        // TODO ring structure vs chain structure
        if (rdes1.RCH()) {
            fetchNext = true;
            mDMACHRDR = rdes3;
        }
    }
}

void ETH::IRQ()
{
    mEmulator->SetExceptionPending(emu::from_underlying<arm::Exception>((0x134 / 4)));
}

} // namespace stm
