#pragma once

#include <Peripheral.hpp>
#include <connectors.hpp>

#include <atomic>

namespace stm
{

class GPIO : public emu::Peripheral {
public:
    GPIO();
    virtual ~GPIO();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

private:
    std::vector<GPIOConnectorView> mGPIOInputConnectors;
    std::vector<GPIOConnectorView> mGPIOOutputConnectors;

    std::atomic_uint32_t mIDR;
    std::atomic_uint32_t mODR;
};

} // namespace stm
