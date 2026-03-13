#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

#include <bitset>
#include <utility>

namespace emu
{

class NVIC : public Peripheral {
public:
    NVIC();
    virtual ~NVIC();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

    void ProcessPendingExceptions();

    void HandleException(arm::Exception exception);
    void HandleExceptionReturn(arm::EXC_RETURN excReturn);

    void SetExceptionPending(arm::Exception exception);
    void SetExceptionEnabled(arm::Exception exception, bool enabled);
    void SetExceptionPriority(arm::Exception exception, uint8_t priority);

    bool GetExceptionActive(arm::Exception exception);
    bool GetExceptionPending(arm::Exception exception);
    bool GetExceptionEnabled(arm::Exception exception);
    uint8_t GetExceptionPriority(arm::Exception exception);

    // According to the ARMv7-M Architexture Reference Manual the NVIC only supports 496 external interrupts
    // However we do something similar to STM's implementation which states:
    // > "All interrupts, including the core exceptions, are managed by the NVIC." (RM0468)
    static inline constexpr size_t sMaxExceptions = 512;
    static inline constexpr size_t sExternalIRQStart = std::to_underlying(arm::Exception::ExternalInterrupt0);

    static inline constexpr int16_t sMaxPriority = 255;

protected:
    void ExceptionEntry(arm::Exception exception);
    void ExceptionReturn(arm::EXC_RETURN excReturn);
    void PushStack();
    void PopStack(uint32_t frameptr, arm::EXC_RETURN excReturn);
    int16_t ExecutionPriority();

    bool mHandlerMode;
    bool mHasPendingExceptions;
    std::bitset<sMaxExceptions> mExceptionEnabled;
    std::bitset<sMaxExceptions> mExceptionPending;
    std::bitset<sMaxExceptions> mExceptionActive;
    std::array<int16_t, sMaxExceptions> mExceptionPriorities;
};

} // namespace emu
