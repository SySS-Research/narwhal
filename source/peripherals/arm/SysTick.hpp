#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

#include <chrono>

namespace emu
{

class SysTick : public Peripheral
{
public:
    SysTick();
    virtual ~SysTick();

    virtual void Update() override;
    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

    virtual void SaveState(emu::State& state) override;
    virtual void RestoreState(emu::State& state) override;

    void SetEnabled(bool enabled);

    // Returns true if there is a pending tick interrupt
    bool Tick();

public:
    class CSR : public arm::RegisterBase
    {
    public:
        inline CSR(uint32_t value) : arm::RegisterBase(value) {}

        REGISTER_BASE_BIT(COUNTFLAG, 16);
        REGISTER_BASE_BIT(CLKSOURCE, 2);
        REGISTER_BASE_BIT(TICKINT,   1);
        REGISTER_BASE_BIT(ENABLE,    0);
    };

protected:
    // Returns the delay between two interrupts in microseconds
    uint64_t GetTickDelay();

    CSR mCSR;
    uint32_t mLoad;
    uint32_t mValue;

    std::chrono::time_point<std::chrono::high_resolution_clock> mLastTick;

    bool mEnabled;
};

} // namespace emu
