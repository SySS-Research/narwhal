#pragma once

#include "Device.hpp"
#include "connectors.hpp"

class ST7789 : public emu::Device {
public:
    ST7789();
    virtual ~ST7789();

    virtual void Init() override;

    virtual void DrawNode() override;

    const std::vector<uint8_t>& GetPixels() const { return mPixels; }
    // TODO
    size_t GetWidth() const { return 240; }
    size_t GetHeight() const { return 320; }

protected:
    GPIOConnectorView mSelectConnectorView;
    ST7789CommandConnectorView mCommandConnectorView;
    ST7789DataConnectorView mDataConnectorView;

    uint8_t mCurrentCommand;
    uint32_t mWriteOffset;
    std::vector<uint8_t> mPixels;
};
