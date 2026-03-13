#include "FMC.hpp"
#include "Emulator.hpp"

namespace stm
{

FMC::FMC() : Peripheral(),
 mCommandConnectorView("command"),
 mDataConnectorView("data")
{
    GetOutputConnectors().Add(mCommandConnectorView);
    GetOutputConnectors().Add(mDataConnectorView);
}

FMC::~FMC()
{
}

void FMC::Init()
{
    // TODO

    // FMC 1
    mEmulator->RegisterUnmanagedMMIO(0xC0000000, 0x4000000,
        [this](uint32_t offset, uint32_t size) -> uint32_t {
            // TODO
            return 0;
        },
        [this](uint32_t offset, uint32_t size, uint32_t value) -> void {
            if (offset == 0x0 && size == 0x1) {
                mCommandConnectorView(static_cast<uint8_t>(value));
                return;
            } else if (offset == 0x80) { // TODO
                mDataConnectorView(value, size);
                return;
            }

            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
        }
    );
}

uint32_t FMC::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {

    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void FMC::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace stm
