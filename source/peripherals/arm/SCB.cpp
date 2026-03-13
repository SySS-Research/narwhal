#include "SCB.hpp"
#include "Emulator.hpp"
#include "NVIC.hpp"

namespace emu
{

SCB::SCB() : Peripheral(),
 mVTOR(0),
 mICSR(0),
 mAIRCR(0)
{
    SetBounds(0xE000ED00, 0xE000ED8F);
}

SCB::~SCB()
{
}

uint32_t SCB::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x0: // CPUID
            return 0; // TODO
        case 0x4: // ICSR
            return mICSR;
        case 0x8: // VTOR
            return mVTOR;
        case 0xC: // AIRCR
            mAIRCR.setVECTKEY(0xFA05);
            return mAIRCR;
        case 0x24: { // SHCSR
            SHCSR shcsr(0);
            auto& nvic = mEmulator->GetNVIC();
            if (nvic->GetExceptionEnabled(arm::Exception::UsageFault)) shcsr.setUSGFAULTENA();
            if (nvic->GetExceptionEnabled(arm::Exception::BusFault)) shcsr.setBUSFAULTENA();
            if (nvic->GetExceptionEnabled(arm::Exception::MemManage)) shcsr.setMEMFAULTENA();

            if (nvic->GetExceptionPending(arm::Exception::SVCall)) shcsr.setSVCALLPENDED();
            if (nvic->GetExceptionPending(arm::Exception::BusFault)) shcsr.setBUSFAULTPENDED();
            if (nvic->GetExceptionPending(arm::Exception::MemManage)) shcsr.setMEMFAULTPENDED();
            if (nvic->GetExceptionPending(arm::Exception::UsageFault)) shcsr.setUSGFAULTPENDED();

            if (nvic->GetExceptionActive(arm::Exception::SysTick)) shcsr.setSYSTICKACT();
            if (nvic->GetExceptionActive(arm::Exception::PendSV)) shcsr.setPENDSVACT();
            if (nvic->GetExceptionActive(arm::Exception::DebugMonitor)) shcsr.setMONITORACT();
            if (nvic->GetExceptionActive(arm::Exception::SVCall)) shcsr.setSVCALLACT();
            if (nvic->GetExceptionActive(arm::Exception::UsageFault)) shcsr.setUSGFAULTACT();
            if (nvic->GetExceptionActive(arm::Exception::BusFault)) shcsr.setBUSFAULTACT();
            if (nvic->GetExceptionActive(arm::Exception::MemManage)) shcsr.setMEMFAULTACT();
            return shcsr;
        }
    };

    if (offset >= 0x18 && offset < 0x24) { // SHPR
        offset -= 0x18;

        uint32_t v = 0;
        v |= mEmulator->GetNVIC()->GetExceptionPriority(emu::from_underlying<arm::Exception>(offset + 4 + 3)) << 24;
        v |= mEmulator->GetNVIC()->GetExceptionPriority(emu::from_underlying<arm::Exception>(offset + 4 + 2)) << 16;
        v |= mEmulator->GetNVIC()->GetExceptionPriority(emu::from_underlying<arm::Exception>(offset + 4 + 1)) << 8;
        v |= mEmulator->GetNVIC()->GetExceptionPriority(emu::from_underlying<arm::Exception>(offset + 4));
        return v;
    }

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void SCB::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        case 0x4:
            mICSR = value;
            // TODO this is not right
            if (mICSR.PENDSVSET()) {
                mEmulator->SetExceptionPending(arm::Exception::PendSV);
                mICSR.clearPENDSVSET();
            }
            return;
        case 0x8: // VTOR
            mVTOR = value & ~0x7F;
            return;
        case 0xC: { // AIRCR
            AIRCR aircr(value);
            // "Register writes must write 0x05FA to this field, otherwise the write is ignored." (reference manual)
            if (aircr.VECTKEY() != 0x05FA) {
                return;
            }
            mAIRCR = aircr;
            return;
        }
        case 0x24: { // SHCSR
            SHCSR shcsr(value);
            auto& nvic = mEmulator->GetNVIC();
            nvic->SetExceptionEnabled(arm::Exception::UsageFault, shcsr.USGFAULTENA());
            nvic->SetExceptionEnabled(arm::Exception::BusFault, shcsr.BUSFAULTENA());
            nvic->SetExceptionEnabled(arm::Exception::MemManage, shcsr.MEMFAULTENA());
            // TODO
            return;
        }
    };

    if (offset >= 0x18 && offset < 0x24) { // SHPR
        offset -= 0x18;

        for (uint32_t i = 0; i < size; i++) {
            mEmulator->GetNVIC()->SetExceptionPriority(emu::from_underlying<arm::Exception>(offset + i + 4), (value >> i*8) & 0xFF);
        }
        return;
    }

    EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
}

} // namespace emu
