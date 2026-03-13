#include "GPIO.hpp"

namespace
{

constexpr size_t sNumGPIOPins = 16;

} // namespace

namespace stm
{

GPIO::GPIO() : Peripheral(),
 mGPIOInputConnectors(),
 mGPIOOutputConnectors(),
 mIDR(),
 mODR()
{
    mGPIOInputConnectors.reserve(sNumGPIOPins);
    mGPIOOutputConnectors.reserve(sNumGPIOPins);

    for (size_t i = 0; i < sNumGPIOPins; i++) {
        auto& input = mGPIOInputConnectors.emplace_back("pin" + std::to_string(i));
        GetInputConnectors().Add(input);

        auto& output = mGPIOOutputConnectors.emplace_back("pin" + std::to_string(i));
        GetOutputConnectors().Add(output);

        input.OnCall([this, i](bool enabled) {
            if (enabled) {
                mIDR |= (1u << i);
            } else {
                mIDR &= ~(1u << i);
            }
        });
    }
}

GPIO::~GPIO()
{
}

uint32_t GPIO::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x10: // IDR
            return mIDR;
        case 0x14: // ODR
            return mODR;
        case 0x18: // BSSR (write-only)
            EMU_LOG_WARN("Reading from write-only register");
            return 0;
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void GPIO::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x10: // IDR
            mIDR = value;
            return;
        case 0x14: // ODR
            mODR = value;
            return;
        case 0x18: // BSSR (write-only)
            for (size_t i = 0; i < sNumGPIOPins; i++) {
                // Set
                if (value & (1u << i)) {
                    mODR |= (1u << i);

                    // Call callback
                    mGPIOOutputConnectors[i](true);
                }

                // Clear
                if (value & (1u << (i + sNumGPIOPins))) {
                    mODR &= ~(1u << i);

                    // Call callback
                    mGPIOOutputConnectors[i](false);
                }
            }
            return;
    };

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
