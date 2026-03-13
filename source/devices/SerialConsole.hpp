#pragma once

#include "Device.hpp"
#include "connectors.hpp"

class SerialConsole : public emu::Device {
public:
    SerialConsole();
    virtual ~SerialConsole();

    virtual void Init() override;

    void OnData(uint32_t data);

    std::string GetBuffer();

protected:
    USARTDataConnectorView mRxConnectorView;
    std::string mBuffer;
    std::mutex mBufferMutex;
};
