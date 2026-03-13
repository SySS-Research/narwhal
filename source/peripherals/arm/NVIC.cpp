#include "NVIC.hpp"
#include "Emulator.hpp"
#include "SCB.hpp"

namespace emu
{

NVIC::NVIC() : Peripheral(),
 mHandlerMode(false),
 mHasPendingExceptions(false),
 mExceptionEnabled(),
 mExceptionPending(),
 mExceptionActive(),
 mExceptionPriorities()
{
    SetBounds(0xE000E100, 0xE000ECFC);

    // We enable core CPU exceptions by default
    mExceptionEnabled[std::to_underlying(arm::Exception::Reset)]        = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::NMI)]          = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::HardFault)]    = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::MemManage)]    = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::BusFault)]     = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::UsageFault)]   = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::SVCall)]       = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::DebugMonitor)] = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::PendSV)]       = true;
    mExceptionEnabled[std::to_underlying(arm::Exception::SysTick)]      = true;

    // Initialize default priorities
    mExceptionPriorities[std::to_underlying(arm::Exception::Reset)]         = -3;
    mExceptionPriorities[std::to_underlying(arm::Exception::NMI)]           = -2;
    mExceptionPriorities[std::to_underlying(arm::Exception::HardFault)]     = -1;
    mExceptionPriorities[std::to_underlying(arm::Exception::MemManage)]     = 0;
    mExceptionPriorities[std::to_underlying(arm::Exception::BusFault)]      = 1;
    mExceptionPriorities[std::to_underlying(arm::Exception::UsageFault)]    = 2;
    mExceptionPriorities[std::to_underlying(arm::Exception::SVCall)]        = 3;
    mExceptionPriorities[std::to_underlying(arm::Exception::DebugMonitor)]  = 4;
    mExceptionPriorities[std::to_underlying(arm::Exception::PendSV)]        = 5;
    mExceptionPriorities[std::to_underlying(arm::Exception::SysTick)]       = 6;

    // For all external interrupts we increment the priority (this is what STM32H7 does by default at least)
    for (size_t i = std::to_underlying(arm::Exception::ExternalInterrupt0); i < mExceptionPriorities.size(); i++) {
        mExceptionPriorities[i] = std::min(sMaxPriority, static_cast<int16_t>(7 + i));
    }
}

NVIC::~NVIC()
{
}

uint32_t NVIC::Read(uint32_t offset, uint32_t size)
{
    if (offset >= 0x000 && offset < 0x040) { // ISER
        return slice_bitset<uint32_t>(mExceptionEnabled, sExternalIRQStart + offset*8);
    } else if (offset >= 0x080 && offset < 0x0C0) { // ICER
        offset -= 0x80;
        return slice_bitset<uint32_t>(mExceptionEnabled, sExternalIRQStart + offset*8);
    } else if (offset >= 0x100 && offset < 0x140) { // ISPR
        offset -= 0x100;
        return slice_bitset<uint32_t>(mExceptionPending, sExternalIRQStart + offset*8);
    } else if (offset >= 0x180 && offset < 0x1C0) { // ICPR
        offset -= 0x180;
        return slice_bitset<uint32_t>(mExceptionPending, sExternalIRQStart + offset*8);
    } else if (offset >= 0x200 && offset < 0x240) { // IABR
        offset -= 0x200;
        return slice_bitset<uint32_t>(mExceptionActive, sExternalIRQStart + offset*8);
    } else if (offset >= 0x300 && offset < 0x4F0) { // IPR
        // TODO maybe just do uint32_t v = 0; v |= mPriority[4*n + 0] << 0; v |= mPriority[4*n + 1] << 8; v |= mPriority[4*n + 2] << 16; v |= mPriority[4*n + 3] << 24; return v;
        EMU_ASSERT(size == 1);

        offset -= 0x300;
        return mExceptionPriorities[sExternalIRQStart + offset];
    }

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void NVIC::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    if (offset >= 0x000 && offset < 0x040) { // ISER
        for (size_t i = 0; i < 32; i++) {
            if (value & (1u << i)) {
                mExceptionEnabled[(sExternalIRQStart + offset*8) + i] = true;
            }
        }
        return;
    } else if (offset >= 0x080 && offset < 0x0C0) { // ICER
        offset -= 0x80;
        for (size_t i = 0; i < 32; i++) {
            if (value & (1u << i)) {
                mExceptionEnabled[(sExternalIRQStart + offset*8) + i] = false;
            }
        }
        return;
    } else if (offset >= 0x100 && offset < 0x140) { // ISPR
        offset -= 0x100;
        for (size_t i = 0; i < 32; i++) {
            if (value & (1u << i)) {
                mExceptionPending[(sExternalIRQStart + offset*8) + i] = true;
                mHasPendingExceptions = true;
            }
        }
        return;
    } else if (offset >= 0x180 && offset < 0x1C0) { // ICPR
        offset -= 0x180;
        for (size_t i = 0; i < 32; i++) {
            if (value & (1u << i)) {
                mExceptionPending[(sExternalIRQStart + offset*8) + i] = false;
                mHasPendingExceptions = mExceptionPending.any();
            }
        }
        return;
    } else if (offset >= 0x300 && offset < 0x4F0) { // IPR
        EMU_ASSERT(size == 1);

        offset -= 0x300;
        mExceptionPriorities[sExternalIRQStart + offset] = value;
        return;
    }

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

void NVIC::ProcessPendingExceptions()
{
    // Optimization
    if (!mHasPendingExceptions) {
        return;
    }

    // Iterate over all pending exceptions and find the one with the lowest priority
    arm::Exception exception = arm::Exception::Invalid;
    int16_t minPriority = ExecutionPriority();
    for (size_t i = 2; i < mExceptionPending.size(); i++) {
        // From reference manual:
        // "When an interrupt is disabled, interrupt assertion causes the interrupt to become pending, but the interrupt cannot become active."
        if (mExceptionPending[i] && mExceptionEnabled[i] && !mExceptionActive[i] && mExceptionPriorities[i] < minPriority) {
            exception = emu::from_underlying<arm::Exception>(i);
            minPriority = mExceptionPriorities[i];
        }
    }

    if (exception != arm::Exception::Invalid) {
        ExceptionEntry(exception);
        mExceptionPending[std::to_underlying(exception)] = false;
        mHasPendingExceptions = mExceptionPending.any();
    }
}

void NVIC::HandleException(arm::Exception exception)
{
    // TODO priority of exception to handle?

    ExceptionEntry(exception);
}

void NVIC::HandleExceptionReturn(arm::EXC_RETURN excReturn)
{
    ExceptionReturn(excReturn);
}

void NVIC::SetExceptionPending(arm::Exception exception)
{
    size_t i = std::to_underlying(exception);

    if (mExceptionPending[i]) {
        // Can probably remove this warning, this is (should be?) fine
        // EMU_LOG_WARN("Exception {} already pending", i);
        return;
    }

    mExceptionPending[i] = true;
    mHasPendingExceptions = true;
}

void NVIC::SetExceptionEnabled(arm::Exception exception, bool enabled)
{
    mExceptionEnabled[std::to_underlying(exception)] = enabled;
}

void NVIC::SetExceptionPriority(arm::Exception exception, uint8_t priority)
{
    mExceptionPriorities[std::to_underlying(exception)] = priority;
}

bool NVIC::GetExceptionActive(arm::Exception exception)
{
    return mExceptionActive[std::to_underlying(exception)];
}

bool NVIC::GetExceptionPending(arm::Exception exception)
{
    return mExceptionPending[std::to_underlying(exception)];
}

bool NVIC::GetExceptionEnabled(arm::Exception exception)
{
    return mExceptionEnabled[std::to_underlying(exception)];
}

uint8_t NVIC::GetExceptionPriority(arm::Exception exception)
{
    return mExceptionPriorities[std::to_underlying(exception)];
}

// See Arm®v7-M Architecture Reference Manual - B1.5.6 Exception entry behavior - ExceptionEntry() & ExceptionTaken()
void NVIC::ExceptionEntry(arm::Exception exception)
{
    uint32_t exceptionNumber = std::to_underlying(exception);

    // Sanity check, this should never happen
    if (mExceptionActive[exceptionNumber]) {
        EMU_FATAL("Exception {} already active", exceptionNumber);
        return;
    }

    // Push current state to stack
    PushStack();

    // Update PC
    uint32_t vectorTable = mEmulator->GetSCB()->GetVTOR();
    uint32_t pc = mEmulator->ReadMemory<uint32_t>(vectorTable + exceptionNumber * 4);
    mEmulator->SetRegister(arm::Register::PC, pc);

    // We're now in handler mode
    mHandlerMode = true;

    // Update control Register
    // NOTE: It is important!!! that this is done *before* updating IPSR,
    // otherwise unicorn no longer allows updating the SPSEL bit
    arm::CONTROL control = mEmulator->GetRegister(arm::Register::CONTROL);
    control.clearFPCA();
    control.clearSPSEL();
    mEmulator->SetRegister(arm::Register::CONTROL, control);

    // Write exception number to IPSR
    mEmulator->SetRegister(arm::Register::IPSR, exceptionNumber & 0xFF);

    // TODO EPSR?

    // Mark exception as active
    mExceptionActive[exceptionNumber] = true;

    // TODO what is the SCS/event/whatever other stuff about?
}

// See Arm®v7-M Architecture Reference Manual - B1.5.8 Exception return behavior - ExceptionReturn()
void NVIC::ExceptionReturn(arm::EXC_RETURN excReturn)
{
    if (!mHandlerMode) {
        EMU_FATAL("Unexpected exception return");
    }

    if (excReturn.Reserved() != 0b01) {
        EMU_FATAL("Illegal EXC_RETURN ({})", excReturn.Reserved());
    }

    uint32_t returningExceptionNumber = mEmulator->GetRegister(arm::Register::IPSR) & 0xFF;
    uint32_t nestedActivation = mExceptionActive.count();
    arm::CONTROL control = mEmulator->GetRegister(arm::Register::CONTROL);

    if (!mExceptionActive[returningExceptionNumber]) {
        EMU_FATAL("Returning from an inactive handler");
        // TODO this should technically trigger a usage fault
    }

    // Decide if returning to handler or thread and which stack to use
    arm::Register stack;
    if (!excReturn.ThreadMode()) { // return to handler
        // Make sure SP_main is used
        if (excReturn.SPSEL()) {
            EMU_FATAL("Illegal EXC_RETURN");
        }

        stack = arm::Register::MSP;
        mHandlerMode = true;
        control.clearSPSEL();
    } else { // return to thread
        // TODO NONBASETHRDENA
        if (nestedActivation != 1) {
            EMU_FATAL("Returning to thread with active exception");
        }

        // Choose stack
        stack = excReturn.SPSEL() ? arm::Register::PSP : arm::Register::MSP;

        // Update CONTROL
        excReturn.SPSEL() ? control.setSPSEL() : control.clearSPSEL();

        mHandlerMode = false;
    }

    // DeActivate()
    mExceptionActive[returningExceptionNumber] = false;
    // TODO FAULTMASK

    uint32_t frameptr = mEmulator->GetRegister(stack);
    PopStack(frameptr, excReturn);

    // NOTE: It is important!!! that this is done *after* PopStack restored IPSR using xPSR,
    // otherwise unicorn doesn't allow updating the SPSEL bit
    mEmulator->SetRegister(arm::Register::CONTROL, control);
    // Not sure what happens if an exception was in process mode?
    // Adding this sanity check here to catch it
    EMU_ASSERT(mEmulator->GetRegister(arm::Register::CONTROL) == control);

    // TODO check for consistent IPSR
}

// See Arm®v7-M Architecture Reference Manual - B1.5.6 Exception entry behavior - PushStack()
void NVIC::PushStack()
{
    arm::CONTROL control = mEmulator->GetRegister(arm::Register::CONTROL);

    // Check how large the frame should be
    uint32_t framesize;
    bool forcealign;
    if (control.FPCA()) {
        framesize = 0x68;
        forcealign = true;
    } else {
        framesize = 0x20;
        forcealign = true; // TODO CCR
    }

    // Figure out which stack should be used
    arm::Register stack = (control.SPSEL() && !mHandlerMode) ? arm::Register::PSP : arm::Register::MSP;

    // Get the current stack address
    uint32_t frameptr = mEmulator->GetRegister(stack);

    // Check if the stack needs to be aligned
    const bool frameptralign = (frameptr & 0b100) && forcealign;
    if (frameptralign) {
        frameptr &= ~0b100;
    }

    frameptr -= framesize;

    // Set new stack pointer
    mEmulator->SetRegister(stack, frameptr);

    // Save registers to frame
    mEmulator->WriteMemory(frameptr,        mEmulator->GetRegister(arm::Register::R0)); 
    mEmulator->WriteMemory(frameptr + 0x4,  mEmulator->GetRegister(arm::Register::R1));
    mEmulator->WriteMemory(frameptr + 0x8,  mEmulator->GetRegister(arm::Register::R2));
    mEmulator->WriteMemory(frameptr + 0xC,  mEmulator->GetRegister(arm::Register::R3));
    mEmulator->WriteMemory(frameptr + 0x10, mEmulator->GetRegister(arm::Register::R12));
    mEmulator->WriteMemory(frameptr + 0x14, mEmulator->GetRegister(arm::Register::LR));
    mEmulator->WriteMemory(frameptr + 0x18, mEmulator->GetRegister(arm::Register::PC)); // TODO check nextins/curins?

    // Write XPSR to frame, set align bit if needed
    arm::XPSR xpsr = mEmulator->GetRegister(arm::Register::XPSR);
    if (frameptralign) {
        xpsr.setSPREALIGN();
    }
    mEmulator->WriteMemory<uint32_t>(frameptr + 0x1C, xpsr);

    // Store float state
    if (control.FPCA()) {
        // TODO FPCCR

        for (int i = 0; i < 16; i++) {
            mEmulator->WriteMemory(frameptr + 0x20 + (4 * i),
                mEmulator->GetRegister(emu::from_underlying<arm::Register>(std::to_underlying(arm::Register::S0) + i))
            );

            // TODO FPCCR
            mEmulator->WriteMemory<uint32_t>(frameptr + 0x60, 0);
        }
    }

    // Prepare EXC_RETURN LR
    arm::EXC_RETURN excReturn;
    // Set FPCA bit
    if (!control.FPCA()) {
        excReturn.setNoFP();
    }
    // Set thread mode bit
    if (!mHandlerMode) {
        excReturn.setThreadMode();
    }
    // Set SPSEL bit
    if (control.SPSEL()) {
        excReturn.setSPSEL();
    }
    mEmulator->SetRegister(arm::Register::LR, excReturn);
}

// See Arm®v7-M Architecture Reference Manual - B1.5.8 Exception return behavior - PopStack()
void NVIC::PopStack(uint32_t frameptr, arm::EXC_RETURN excReturn)
{
    uint32_t framesize;
    bool forcealign;
    if (!excReturn.NoFP()) {
        framesize = 0x68;
        forcealign = true;
    } else {
        framesize = 0x20;
        forcealign = true; // TODO CCR.STKALIGN
    }

    // Restore registers
    mEmulator->SetRegister(arm::Register::R0,   mEmulator->ReadMemory<uint32_t>(frameptr));
    mEmulator->SetRegister(arm::Register::R1,   mEmulator->ReadMemory<uint32_t>(frameptr + 0x4));
    mEmulator->SetRegister(arm::Register::R2,   mEmulator->ReadMemory<uint32_t>(frameptr + 0x8));
    mEmulator->SetRegister(arm::Register::R3,   mEmulator->ReadMemory<uint32_t>(frameptr + 0xC));
    mEmulator->SetRegister(arm::Register::R12,  mEmulator->ReadMemory<uint32_t>(frameptr + 0x10));
    mEmulator->SetRegister(arm::Register::LR,   mEmulator->ReadMemory<uint32_t>(frameptr + 0x14));
    mEmulator->SetRegister(arm::Register::PC,   mEmulator->ReadMemory<uint32_t>(frameptr + 0x18));

    arm::XPSR xpsr = mEmulator->ReadMemory<uint32_t>(frameptr + 0x1C);


    if (!excReturn.NoFP()) {
        for (int i = 0; i < 16; i++) {
            mEmulator->SetRegister(emu::from_underlying<arm::Register>(std::to_underlying(arm::Register::S0) + i),
                mEmulator->ReadMemory<uint32_t>(frameptr + 0x20 + (4 * i))
            );

            // TODO FPCCR
        }

        // TODO FPCA
    }

    // Check if it needs to be realigned
    if (xpsr.SPREALIGN() && forcealign) {
        framesize |= 0b100;
    }

    arm::Register stack = excReturn.SPSEL() ? arm::Register::PSP : arm::Register::MSP;
    mEmulator->SetRegister(stack, mEmulator->GetRegister(stack) + framesize);

    // Restore XPSR
    xpsr.clearSPREALIGN(); // TODO check if this really isnt restored
    mEmulator->SetRegister(arm::Register::XPSR, xpsr);
}

// See Arm®v7-M Architecture Reference Manual - B1.5.4 Exception priorities and preemption - ExecutionPriority()
int16_t NVIC::ExecutionPriority()
{
    int16_t highestPri = sMaxPriority + 1;
    int16_t boostedPri = sMaxPriority + 1;

    // TODO is this correct? 0b01?
    uint32_t groupValue = 0b10u << mEmulator->GetSCB()->GetAIRCR().PRIGROUP();

    // Find the active exception with the "highest" priority
    for (size_t i = 2; i < sMaxExceptions; i++) {
        if (mExceptionActive[i]) {
            if (mExceptionPriorities[i] < highestPri) {
                highestPri = mExceptionPriorities[i];

                int16_t subgroupValue = highestPri % groupValue;
                highestPri = highestPri - subgroupValue;
            }
        }
    }

    uint32_t basepri = mEmulator->GetRegister(arm::Register::BASEPRI) & 0xFF;
    if (basepri != 0) {
        boostedPri = basepri;

        int16_t subgroupValue = boostedPri % groupValue;
        boostedPri = boostedPri - subgroupValue;
    }

    // Setting primask raises execution priority to 0,
    // to prevent any exceptions with configurable priority from becoming active
    if (mEmulator->GetRegister(arm::Register::PRIMASK) & 1) {
        boostedPri = 0;
    }

    // Similar as primask but raises prio to -1 (same as HardFault)
    if (mEmulator->GetRegister(arm::Register::FAULTMASK) & 1) {
        boostedPri = -1;
    }

    // Return the "higher" priority
    return std::min(boostedPri, highestPri);
}

} // namespace emu
