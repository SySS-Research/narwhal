#pragma once

#include <Device.hpp>
#include <connectors.hpp>

#include <yaml-cpp/yaml.h>

class EMMC : public emu::Device {
public:
    EMMC();
    EMMC(const YAML::Node& config);
    virtual ~EMMC();

    virtual void Init() override;

protected:
    MMCCommandConnectorView mCommandConnectorView;
    MMCDataConnectorView mRxDataConnectorView;
    MMCDataConnectorView mTxDataConnectorView;

    std::unique_ptr<std::FILE, int(*)(std::FILE*)> mFile;
    std::vector<uint8_t> mCID;
    std::vector<uint8_t> mCSD;
    std::vector<uint8_t> mExtCSD;
};
