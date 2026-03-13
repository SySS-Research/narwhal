#include "SDMMC.hpp"
#include <utility>

namespace
{

// Commands from https://github.com/STMicroelectronics/stm32h7xx-hal-driver/blob/8bd31106a890ae7254f757bbe55d467a52cc8e03/Inc/stm32h7xx_ll_sdmmc.h#L207-L284 
enum class SDMMCCommand : uint32_t
{
    GoIdleState = 0U,
    SendOpCond = 1U,
    AllSendCid = 2U,
    SetRelAddr = 3U,
    SetDsr = 4U,
    SdmmcSenOpCond = 5U,
    HsSwitch = 6U,
    SelDeselCard = 7U,
    HsSendExtCsd = 8U,
    SendCsd = 9U,
    SendCid = 10U,
    VoltageSwitch = 11U,
    StopTransmission = 12U,
    SendStatus = 13U,
    HsBustestRead = 14U,
    GoInactiveState = 15U,
    SetBlocklen = 16U,
    ReadSingleBlock = 17U,
    ReadMultBlock = 18U,
    HsBustestWrite = 19U,
    WriteDatUntilStop = 20U,
    SetBlockCount = 23U,
    WriteSingleBlock = 24U,
    WriteMultBlock = 25U,
    ProgCid = 26U,
    ProgCsd = 27U,
    SetWriteProt = 28U,
    ClrWriteProt = 29U,
    SendWriteProt = 30U,
    SdEraseGrpStart = 32U,
    SdEraseGrpEnd = 33U,
    EraseGrpStart = 35U,
    EraseGrpEnd = 36U,
    Erase = 38U,
    FastIo = 39U,
    GoIrqState = 40U,
    LockUnlock = 42U,
    AppCmd = 55U,
    GenCmd = 56U,
    NoCmd = 64U,

    AppSdSetBuswidth = 6U,
    SdAppStatus = 13U,
    SdAppSendNumWriteBlocks = 22U,
    SdAppOpCond = 41U,
    SdAppSetClrCardDetect = 42U,
    SdAppSendScr = 51U,
    SdmmcRwDirect = 52U,
    SdmmcRwExtended = 53U,

    MmcSleepAwake = 5U,

    SdAppGetMkb = 43U,
    SdAppGetMid = 44U,
    SdAppSetCerRn1 = 45U,
    SdAppGetCerRn2 = 46U,
    SdAppSetCerRes2 = 47U,
    SdAppGetCerRes1 = 48U,
    SdAppSecureReadMultipleBlock = 18U,
    SdAppSecureWriteMultipleBlock = 25U,
    SdAppSecureErase = 38U,
    SdAppChangeSecureArea = 49U,
    SdAppSecureWriteMkb = 48U
};

} // namespace

namespace stm
{

SDMMC::SDMMC() : Peripheral(),
 mCommandConnectorView("cmd"),
 mTxDataConnectorView("tx"),
 mRxDataConnectorView("rx"),
 mARG(0),
 mCMD(0),
 mRESPCMD(0),
 mRESP(),
 mDTIME(0),
 mDLEN(0),
 mDCTRL(0),
 mSTA(0),
 mFIFO()
{
    GetOutputConnectors().Add(mCommandConnectorView);
    GetOutputConnectors().Add(mTxDataConnectorView);
    GetInputConnectors().Add(mRxDataConnectorView);

    mRxDataConnectorView.OnCall([this](uint32_t _cmd, std::span<const uint8_t> data) {
        SDMMCCommand cmd = emu::from_underlying<SDMMCCommand>(_cmd);

        switch (cmd){
            case SDMMCCommand::AllSendCid:
            case SDMMCCommand::SendCid:
            case SDMMCCommand::SendCsd:
                EMU_ASSERT(data.size() == 0x10);
                // Unpack CID/CSD
                // TODO is this the correct endianness?
                for (size_t i = 0; i < 4; i++) {
                    mRESP[i] = (static_cast<uint32_t>(data[i*4 + 0]) << 24) |
                               (static_cast<uint32_t>(data[i*4 + 1]) << 16) |
                               (static_cast<uint32_t>(data[i*4 + 2]) << 8) |
                               static_cast<uint32_t>(data[i*4 + 3]);
                }
                break;
            case SDMMCCommand::HsSendExtCsd:
            case SDMMCCommand::ReadSingleBlock:
            case SDMMCCommand::ReadMultBlock:
                // Write data to FIFO
                mFIFO.clear();
                for (size_t i = 0; i < data.size() / 4; i++) {
                    mFIFO.push_back((static_cast<uint32_t>(data[i*4 + 3]) << 24) |
                                    (static_cast<uint32_t>(data[i*4 + 2]) << 16) |
                                    (static_cast<uint32_t>(data[i*4 + 1]) << 8) |
                                    static_cast<uint32_t>(data[i*4 + 0]));
                }
                mSTA.setRXFIFOF();
                mSTA.setRXFIFOHF();
                break;
        };
    });
}

SDMMC::~SDMMC()
{
}

uint32_t SDMMC::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: { // POWER
            POWER power(0);
            power.setPWRCTRL(0b11); // Power-on
            return power;
        }
        case 0x8: // ARG
            return mARG;
        case 0xC: // CMD
            return mCMD;
        case 0x10:
            return mRESPCMD;
        case 0x14 ... 0x23: // RESPR
            offset -= 0x14;
            return mRESP[offset / 4];
        case 0x24: // DTIME
            return mDTIME;
        case 0x28: // DLEN
            return mDLEN;
        case 0x2C: // DCTRL
            return mDCTRL;
        case 0x34: // STA
            return mSTA;
        case 0x80: { // FIFO
            EMU_ASSERT(!mFIFO.empty());

            uint32_t value = mFIFO.front();
            mFIFO.pop_front();

            if (mFIFO.empty()) {
                mSTA.setRXFIFOE();
                mSTA.setDATAEND(); // TODO cheat for the large FIFO
            }
            // TODO we cheat for now and have a larger than hardware FIFO
            if (mFIFO.size() < 8) {
                mSTA.clearRXFIFOHF();
            }
            if (mFIFO.size() < 32) {
                mSTA.clearRXFIFOF();
            }

            return value;
        }
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void SDMMC::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x8: // ARG
            mARG = value;
            return;
        case 0xC: // CMD
            mCMD = value;

            HandleCommand();
            return;
        case 0x24: // DTIME
            mDTIME = value;
            return;
        case 0x28: // DLEN
            mDLEN = value;
            return;
        case 0x2C: // DCTRL
            mDCTRL = value;
            return;
        case 0x34: // STA: read-only
            return;
        case 0x38: // ICR (clears values in STA)
            mSTA &= ~value;
            return;

    };

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

void SDMMC::HandleCommand()
{
    SDMMCCommand cmd = emu::from_underlying<SDMMCCommand>(mCMD.CMDINDEX());
    uint32_t arg = mARG;

    switch (cmd) {
        case SDMMCCommand::GoIdleState:
            mSTA.setCMDSENT();
            break;
        case SDMMCCommand::SendOpCond:
            mRESP[0] = 1u << 31u | 0xc0u << 24u; // set validvoltage, high capacity
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::AllSendCid:
        case SDMMCCommand::SendCid:
        case SDMMCCommand::SendCsd:
            mCommandConnectorView(std::to_underlying(cmd), arg, 16);
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::SetRelAddr:
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::SelDeselCard:
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::HsSendExtCsd:
            mCommandConnectorView(std::to_underlying(cmd), arg, mDLEN);
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::SendStatus:
            mRESP[0] = 0x100U | 4u << 9u; // TODO
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::SetBlocklen:
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::HsSwitch:
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::ReadSingleBlock:
        case SDMMCCommand::ReadMultBlock:
            mCommandConnectorView(std::to_underlying(cmd), arg, mDLEN);
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;
        case SDMMCCommand::StopTransmission:
            mRESP[0] = 0;
            mRESPCMD = mCMD.CMDINDEX();
            mSTA.setCMDREND();
            break;

        default:
            EMU_LOG_WARN("Unknown command: {}", std::to_underlying(cmd));
    };
}

} // namespace stm
