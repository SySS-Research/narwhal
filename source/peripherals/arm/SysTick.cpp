#include "SysTick.hpp"
#include "Emulator.hpp"

namespace emu
{

SysTick::SysTick() : Peripheral(),
 mCSR(0)
{
    SetBounds(0xE000E010, 0xE000E0FF);
}

SysTick::~SysTick()
{
}

void SysTick::Update()
{
    Peripheral::Update();

    if (Tick()) {
        mEmulator->SetExceptionPending(arm::Exception::SysTick);
    }
}


uint32_t SysTick::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // SYST_CSR
            return mCSR;
        case 0x4: // SYST_RVR
            return mLoad;
        case 0x8: // SYST_CVR
            return mValue;
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void SysTick::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x0: // SYST_CSR
            mCSR = value;
            return;
        case 0x4: // SYST_RVR
            mLoad = value;
            return;
        case 0x8: // SYST_CVR
            // TODO this currently doesn't work
            // we would have to do something like changing mLastTick in order to tick sooner/later
            mValue = value;
            return;
    };

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

bool SysTick::Tick()
{
    const auto tickDelay = GetTickDelay();
    if (tickDelay == 0) {
        return false;
    }

    auto current = std::chrono::high_resolution_clock::now();
    auto timePassed = current - mLastTick;
    uint64_t microsecondsPassed = std::chrono::duration_cast<std::chrono::microseconds>(timePassed).count();

    mValue = (microsecondsPassed / (double) tickDelay) * mLoad;

    // Check if we should tick
    if (microsecondsPassed > tickDelay) {
        mLastTick = current;

        // TODO this is not the right way to do it
        if (!mCSR.ENABLE() || !mCSR.TICKINT()) {
            return false;
        }

        return true;
    }

    return false;
}

uint64_t SysTick::GetTickDelay()
{
    // Value of the Internal oscillator in Hz
    // TODO this value is not the same for all chips and the sysclock source can be configured as well
    constexpr uint64_t HSI_VALUE = 64000000UL;

    if (mLoad == 0) {
        return 0;
    }

    return HSI_VALUE / mLoad;
}

} // namespace emu
