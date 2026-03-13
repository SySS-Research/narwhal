#pragma once

#include "common.hpp"

namespace arm
{

enum class CPUModel
{
    ARMCortexM3,
    ARMCortexM4,
    ARMCortexM7,

    Default = ARMCortexM7,
};

enum class Register : uint32_t
{
    // General registers
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    R12,
    SP,
    LR,
    PC,

    // Float registers
    S0,
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7,
    S8,
    S9,
    S10,
    S11,
    S12,
    S13,
    S14,
    S15,

    // Special registers
    XPSR,
    MSP,
    PSP,
    CONTROL,
    IPSR,
    PRIMASK,
    FAULTMASK,
    BASEPRI,
};

// TODO these differ between CPU models
// See Arm®v7-M Architecture Reference Manual - B1.5.2 Exception number definition
enum class Exception : int
{
    Invalid            = 0,
    Reset              = 1,
    NMI                = 2,
    HardFault          = 3,
    MemManage          = 4,
    BusFault           = 5,
    UsageFault         = 6,
    SVCall             = 11,
    DebugMonitor       = 12,
    PendSV             = 14,
    SysTick            = 15,
    ExternalInterrupt0 = 16,
};

class RegisterBase
{
public:
    inline RegisterBase(uint32_t value) : mValue(value) {}

    // Operators for convincience
    inline operator uint32_t() const { return mValue; }

    inline RegisterBase& operator&=(RegisterBase rhs)
    {
        mValue &= rhs.mValue;
        return *this;
    }

    inline RegisterBase& operator|=(RegisterBase rhs)
    {
        mValue |= rhs.mValue;
        return *this;
    }

    inline RegisterBase& operator^=(RegisterBase rhs)
    {
        mValue ^= rhs.mValue;
        return *this;
    }

    inline RegisterBase& operator<<=(uint32_t shift)
    {
        mValue <<= shift;
        return *this;
    }

    inline RegisterBase& operator>>=(uint32_t shift)
    {
        mValue >>= shift;
        return *this;
    }

    inline RegisterBase operator~() const
    {
        return RegisterBase(~mValue);
    }

    inline bool operator==(RegisterBase rhs) const
    {
        return mValue == rhs.mValue;
    }

    inline bool operator!=(RegisterBase rhs) const
    {
        return mValue != rhs.mValue;
    }

    // For handling byte and halfword access
    uint32_t Read(uint32_t offset, uint32_t size)
    {
        return (mValue >> offset*8) & ((1u << size*8) - 1u);
    }

    void Write(uint32_t offset, uint32_t size, uint32_t value)
    {
        uint32_t mask = ((1u << size*8) - 1u) << offset*8;

        mValue &= ~mask;
        mValue |= (value << offset*8) & mask;
    }

#define REGISTER_BASE_BIT(__name__, __bit__)                              \
    inline bool __name__() const { return !!(mValue & (1u << __bit__)); } \
    inline void set ## __name__() { mValue |= (1u << __bit__); }          \
    inline void clear ## __name__() { mValue &= ~(1u << __bit__); }

#define REGISTER_BASE_RANGE(__name__, __end__, __start__)                                       \
    inline uint32_t __name__() const {                                                          \
        constexpr uint32_t mask = ((1u << ((__end__) - (__start__) + 1)) - 1u) << (__start__);  \
        return (mValue & mask) >> (__start__);                                                  \
    }                                                                                           \
    inline void set ## __name__(uint32_t value) {                                               \
        constexpr uint32_t mask = ((1u << ((__end__) - (__start__) + 1)) - 1u) << (__start__);  \
        mValue = (mValue & ~mask) | ((value << (__start__)) & mask);                            \
    }

protected:
    uint32_t mValue;
};

// See Arm®v7-M Architecture Reference Manual - B1.4.2 The special-purpose Program Status Registers, xPSR
class XPSR : public RegisterBase
{
public:
    inline XPSR(uint32_t value) : RegisterBase(value) {}

    REGISTER_BASE_BIT(SPREALIGN, 8);
};

// See Arm®v7-M Architecture Reference Manual - B1.4.4 The special-purpose CONTROL register
class CONTROL : public RegisterBase
{
public:
    inline CONTROL(uint32_t value) : RegisterBase(value) {}

    REGISTER_BASE_BIT(SPSEL, 1);
    REGISTER_BASE_BIT(FPCA, 2);
};

// See Arm®v7-M Architecture Reference Manual - B1.5.8 Exception return behavior
// Technically not a register, but this class will do
class EXC_RETURN : public RegisterBase
{
public:
    inline EXC_RETURN(uint32_t value) : RegisterBase(value) {}
    explicit EXC_RETURN() : RegisterBase(0xFFFFFFE1) {}

    REGISTER_BASE_BIT(NoFP, 4);
    REGISTER_BASE_BIT(ThreadMode, 3);
    REGISTER_BASE_BIT(SPSEL, 2);
    REGISTER_BASE_RANGE(Reserved, 1, 0);
};

}; // namespace arm
