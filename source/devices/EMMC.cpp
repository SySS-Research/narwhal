#include "EMMC.hpp"

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


EMMC::EMMC() : Device(),
 mCommandConnectorView("cmd"),
 mRxDataConnectorView("rx"),
 mTxDataConnectorView("tx"),
 mFile(nullptr, nullptr),
 mCID(),
 mCSD(),
 mExtCSD()
{
    EMU_FATAL("EMMC needs to be configured");
}

EMMC::EMMC(const YAML::Node& config) : Device(),
 mCommandConnectorView("cmd"),
 mRxDataConnectorView("rx"),
 mTxDataConnectorView("tx"),
 mFile(nullptr, nullptr),
 mCID(),
 mCSD(),
 mExtCSD()
{
    if (!config["file"] || !config["cid"] || !config["csd"] || !config["ext_csd"]) {
        EMU_FATAL("Missing required argument");
    }

    mFile = std::unique_ptr<std::FILE, int(*)(std::FILE*)>(std::fopen(config["file"].as<std::string>().c_str(), "rb"), &std::fclose);
    if (!mFile) {
        EMU_FATAL("Failed to open file");
    }

    mCID = emu::HexToBytes(config["cid"].as<std::string>());
    mCSD = emu::HexToBytes(config["csd"].as<std::string>());
    mExtCSD = emu::HexToBytes(config["ext_csd"].as<std::string>());

    GetInputConnectors().Add(mCommandConnectorView);
    GetInputConnectors().Add(mRxDataConnectorView);
    GetOutputConnectors().Add(mTxDataConnectorView);

    mCommandConnectorView.OnCall([this](uint32_t _cmd, uint32_t arg, uint32_t dataSize) {
        SDMMCCommand cmd = emu::from_underlying<SDMMCCommand>(_cmd);

        switch (cmd) {
            case SDMMCCommand::AllSendCid:
            case SDMMCCommand::SendCid:
                mTxDataConnectorView(_cmd, mCID);
                break;
            case SDMMCCommand::SendCsd:
                mTxDataConnectorView(_cmd, mCSD);
                break;
            case SDMMCCommand::HsSendExtCsd:
                mTxDataConnectorView(_cmd, mExtCSD);
                break;
            case SDMMCCommand::ReadSingleBlock:
            case SDMMCCommand::ReadMultBlock: {
                if (std::fseek(mFile.get(), arg * 512, SEEK_SET) == -1) {
                    EMU_FATAL("Failed to seek to sector {}", arg);
                }

                std::vector<uint8_t> mData;
                mData.resize(dataSize);
                if (std::fread(mData.data(), 1, dataSize, mFile.get()) != dataSize) {
                    EMU_FATAL("Failed to read from file");
                }

                mTxDataConnectorView(_cmd, mData);
                break;
            }
            default:
                EMU_FATAL("Unknown command");
                break;
        };
    });

    mRxDataConnectorView.OnCall([this](uint32_t _cmd, std::span<const uint8_t> data) {
        // TODO
    });
}

EMMC::~EMMC()
{

}

void EMMC::Init()
{

}
