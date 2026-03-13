#include "UnicornEngine.hpp"
#include "capstone.hpp"

namespace
{

// Copied from `unicorn/qemu/target/arm/cpu.h`
// Not sure why this is not in `unicorn/arm.h`
#define EXCP_UDEF            1   /* undefined instruction */
#define EXCP_SWI             2   /* software interrupt */
#define EXCP_PREFETCH_ABORT  3
#define EXCP_DATA_ABORT      4
#define EXCP_IRQ             5
#define EXCP_FIQ             6
#define EXCP_BKPT            7
#define EXCP_EXCEPTION_EXIT  8   /* Return from v7M exception.  */
#define EXCP_KERNEL_TRAP     9   /* Jumped to kernel code page.  */
#define EXCP_HVC            11   /* HyperVisor Call */
#define EXCP_HYP_TRAP       12
#define EXCP_SMC            13   /* Secure Monitor Call */
#define EXCP_VIRQ           14
#define EXCP_VFIQ           15
#define EXCP_SEMIHOST       16   /* semihosting call */
#define EXCP_NOCP           17   /* v7M NOCP UsageFault */
#define EXCP_INVSTATE       18   /* v7M INVSTATE UsageFault */
#define EXCP_STKOF          19   /* v8M STKOF UsageFault */
#define EXCP_LAZYFP         20   /* v7M fault during lazy FP stacking */
#define EXCP_LSERR          21   /* v8M LSERR SecureFault */
#define EXCP_UNALIGNED      22   /* v7M UNALIGNED UsageFault */

int ConvertCPUModelToUCCPU(arm::CPUModel cpu)
{
    switch (cpu)
    {
        case arm::CPUModel::ARMCortexM3: return UC_CPU_ARM_CORTEX_M3;
        case arm::CPUModel::ARMCortexM4: return UC_CPU_ARM_CORTEX_M4;
        case arm::CPUModel::ARMCortexM7: return UC_CPU_ARM_CORTEX_M7;
    }

    EMU_FATAL("");
    return -1;
}

int ConvertRegisterToUCReg(arm::Register reg)
{
    switch (reg) {
        case arm::Register::R0:         return UC_ARM_REG_R0;
        case arm::Register::R1:         return UC_ARM_REG_R1;
        case arm::Register::R2:         return UC_ARM_REG_R2;
        case arm::Register::R3:         return UC_ARM_REG_R3;
        case arm::Register::R4:         return UC_ARM_REG_R4;
        case arm::Register::R5:         return UC_ARM_REG_R5;
        case arm::Register::R6:         return UC_ARM_REG_R6;
        case arm::Register::R7:         return UC_ARM_REG_R7;
        case arm::Register::R8:         return UC_ARM_REG_R8;
        case arm::Register::R9:         return UC_ARM_REG_R9;
        case arm::Register::R10:        return UC_ARM_REG_R10;
        case arm::Register::R11:        return UC_ARM_REG_R11;
        case arm::Register::R12:        return UC_ARM_REG_R12;
        case arm::Register::SP:         return UC_ARM_REG_SP;
        case arm::Register::LR:         return UC_ARM_REG_LR;
        case arm::Register::PC:         return UC_ARM_REG_PC;

        case arm::Register::S0:         return UC_ARM_REG_S0;
        case arm::Register::S1:         return UC_ARM_REG_S1;
        case arm::Register::S2:         return UC_ARM_REG_S2;
        case arm::Register::S3:         return UC_ARM_REG_S3;
        case arm::Register::S4:         return UC_ARM_REG_S4;
        case arm::Register::S5:         return UC_ARM_REG_S5;
        case arm::Register::S6:         return UC_ARM_REG_S6;
        case arm::Register::S7:         return UC_ARM_REG_S7;
        case arm::Register::S8:         return UC_ARM_REG_S8;
        case arm::Register::S9:         return UC_ARM_REG_S9;
        case arm::Register::S10:        return UC_ARM_REG_S10;
        case arm::Register::S11:        return UC_ARM_REG_S11;
        case arm::Register::S12:        return UC_ARM_REG_S12;
        case arm::Register::S13:        return UC_ARM_REG_S13;
        case arm::Register::S14:        return UC_ARM_REG_S14;
        case arm::Register::S15:        return UC_ARM_REG_S15;

        case arm::Register::XPSR:       return UC_ARM_REG_XPSR;
        case arm::Register::MSP:        return UC_ARM_REG_MSP;
        case arm::Register::PSP:        return UC_ARM_REG_PSP;
        case arm::Register::CONTROL:    return UC_ARM_REG_CONTROL;
        case arm::Register::IPSR:       return UC_ARM_REG_IPSR;
        case arm::Register::PRIMASK:    return UC_ARM_REG_PRIMASK;
        case arm::Register::FAULTMASK:  return UC_ARM_REG_FAULTMASK;
        case arm::Register::BASEPRI:    return UC_ARM_REG_BASEPRI;
    }

    EMU_FATAL("");
    return -1;
}

int ConvertMemoryFlagsToUCProt(emu::MemoryFlags flags)
{
    int perms = 0;
    if (!!(flags & emu::MemoryFlags::Read)) {
        perms |= UC_PROT_READ;
    }
    if (!!(flags & emu::MemoryFlags::Write)) {
        perms |= UC_PROT_WRITE;
    }
    if (!!(flags & emu::MemoryFlags::Exec)) {
        perms |= UC_PROT_EXEC;
    }

    return perms;
}

int ConvertWatchpointTypeToUCHook(emu::WatchpointType type)
{
    int uc_type = 0;
    switch (type) {
        case emu::WatchpointType::Read:
            uc_type = UC_HOOK_MEM_READ;
            break;
        case emu::WatchpointType::Write:
            uc_type = UC_HOOK_MEM_WRITE;
            break;
        case emu::WatchpointType::Access:
            uc_type = UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE;
            break;
    }

    return uc_type;
}

}

namespace emu
{

UnicornEngine::UnicornEngine(arm::CPUModel cpu)
 : mEngine(nullptr),
 mUnmappedMemoryHook(),
 mCodeHook(),
 mIntrHook(),
 mPendingStop(false),
 mPeriodicUpdateCallback(),
 mUnmappedMemoryCallback(),
 mCodeCallback(),
 mExceptionCallback(),
 mExceptionReturnCallback(),
 mMMIOReadCallbacks(),
 mMMIOWriteCallbacks(),
 mWatchpoints(),
 mWatchpointMutex(),
 mUpdateMissingWatchpoints(false)
{
    uc_err err = uc_open(UC_ARCH_ARM, UC_MODE_THUMB, &mEngine);
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed on uc_open() with error returned: {} ({})", static_cast<int>(err), uc_strerror(err));
        return;
    }

    err = uc_ctl_set_cpu_model(mEngine, ConvertCPUModelToUCCPU(cpu));
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed on uc_ctl_set_cpu_model() with error returned: {} ({})", static_cast<int>(err), uc_strerror(err));
        return;
    }

    // intercept invalid memory events
    err = uc_hook_add(mEngine, &mUnmappedMemoryHook,
        UC_HOOK_MEM_UNMAPPED,
        reinterpret_cast<void*>(static_cast<uc_cb_eventmem_t>([](uc_engine* uc, uc_mem_type type, uint64_t address, int size, int64_t value, void* userData) -> bool {
            UnicornEngine* _this = static_cast<UnicornEngine*>(userData);
            
            MemoryFlags flags{};
            switch (type) {
                case UC_MEM_READ_UNMAPPED:
                    flags = MemoryFlags::Read;
                    break;
                case UC_MEM_WRITE_UNMAPPED:
                    flags = MemoryFlags::Write;
                    break;
                case UC_MEM_FETCH_UNMAPPED:
                    flags = MemoryFlags::Exec;
                    break;
                default:
                    break;             
            }

            _this->mUnmappedMemoryCallback(flags, static_cast<uint32_t>(address), static_cast<uint32_t>(size), static_cast<uint32_t>(value));

            // Stop emulation, will be continued in main emulation loop
            return false;
        })), this, 1, 0 // Always call hook
    );
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to add necessary hook: {} ({})", static_cast<int>(err), uc_strerror(err));
        return;
    }

    err = uc_hook_add(mEngine, &mBlockHook,
        UC_HOOK_BLOCK,
        reinterpret_cast<void*>(static_cast<uc_cb_hookcode_t>([](uc_engine* uc, uint64_t address, uint32_t size, void* userData) -> void {
            UnicornEngine* _this = static_cast<UnicornEngine*>(userData);

            _this->mBlockCallback(static_cast<uint32_t>(address), size);
        })), this, 1, 0 // Always call hook
    );
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to add necessary hook: {} ({})", static_cast<int>(err), uc_strerror(err));
        return;
    }

#if 1
    // hook every instruction, will be executed just before an instruction is executed
    err = uc_hook_add(mEngine, &mCodeHook,
        UC_HOOK_CODE,
        reinterpret_cast<void*>(static_cast<uc_cb_hookcode_t>([](uc_engine* uc, uint64_t address, uint32_t size, void* userData) -> void {
            UnicornEngine* _this = static_cast<UnicornEngine*>(userData);

            // Create potential missing watchpoint hooks
            _this->CreateMissingWatchpointHooks();

            _this->mCodeCallback(static_cast<uint32_t>(address), size);
        })), this, 1, 0 // Always call hook
    );
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to add necessary hook: {} ({})", static_cast<int>(err), uc_strerror(err));
        return;
    }
#endif

    // hook interrupts
    err = uc_hook_add(mEngine, &mIntrHook,
        UC_HOOK_INTR,
        reinterpret_cast<void*>(static_cast<uc_cb_hookintr_t>([](uc_engine* uc, uint32_t intno, void* userData) -> void{
            UnicornEngine* _this = static_cast<UnicornEngine*>(userData);

            switch(intno) {
                // From Unicorn FAQ:
                // > Note that for cortex-m exec_return, Unicorn has a magic software exception with interrupt number 8.
                // > You may register a hook to handle that.
                case EXCP_EXCEPTION_EXIT:
                    _this->mExceptionReturnCallback(arm::EXC_RETURN(_this->GetRegister(arm::Register::PC) | 1));
                    return;

                case EXCP_SWI:
                    _this->mExceptionCallback(arm::Exception::SVCall);
                    return;

                case EXCP_PREFETCH_ABORT:
                case EXCP_DATA_ABORT:
                    _this->mExceptionCallback(arm::Exception::HardFault);
                    return;
            }

            EMU_FATAL("Unhandled interrupt {}", intno);
        })), this, 1, 0 // Always call hook
    );
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to add necessary hook: {} ({})", static_cast<int>(err), uc_strerror(err));
        return;
    }
}

UnicornEngine::~UnicornEngine()
{
    if (mEngine) {
        uc_close(mEngine);
    }
}

void UnicornEngine::Run()
{
    uint32_t pc = GetRegister(arm::Register::PC) | 1;

    while (true) {
        // Start/continue emulation
        // TODO allow configuring timeout/instruction count
        uc_err err = uc_emu_start(mEngine, pc, 0, 0, 100);
        if (err) {
            // Ignore any unmapped read and writes for now
            if (err == UC_ERR_READ_UNMAPPED || err == UC_ERR_WRITE_UNMAPPED) {
                pc = GetRegister(arm::Register::PC);

                // Figure out opcode size to skip (slow?)
                std::array<uint8_t, 4> opcode;
                if (!ReadMemory(pc, opcode.data(), opcode.size())) {
                    EMU_FATAL("");
                }
                auto instr = capstone::Disassemble(pc, opcode);
                if (!instr) {
                    EMU_FATAL("");
                }

                // Skip instruction
                // This wouldn't work if this would be a branch to a different location
                // But I don't think a unmapped read or write can happen during a branch
                pc = (pc + instr->size) | 1;
                continue;
            }

            EMU_FATAL("Failed on uc_emu_start() with error returned {}: {} (PC: 0x{:08x})", static_cast<int>(err), uc_strerror(err), GetRegister(arm::Register::PC));
            break;
        } else {
            // Update everything that needs periodic updating
            mPeriodicUpdateCallback();

            // Continue execution
            pc = GetRegister(arm::Register::PC) | 1;
        }

        if (mPendingStop) {
            break;
        }
    }
}

void UnicornEngine::Stop()
{
    mPendingStop = true;
    uc_emu_stop(mEngine);
}

void UnicornEngine::SetRegister(arm::Register reg, uint32_t val)
{
    uc_err err = uc_reg_write(mEngine, ConvertRegisterToUCReg(reg), &val);
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to write register");
    }
}

uint32_t UnicornEngine::GetRegister(arm::Register reg)
{
    uint32_t val = 0;
    uc_err err = uc_reg_read(mEngine, ConvertRegisterToUCReg(reg), &val);
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to read register");
    }

    return val;
}

void UnicornEngine::MapMemory(uint32_t address, uint32_t size, MemoryFlags flags)
{
    uc_err err = uc_mem_map(mEngine, address, size, ConvertMemoryFlagsToUCProt(flags));
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to map memory");
    }
}

void UnicornEngine::MapMMIO(uint32_t address, uint32_t size,
    MMIOReadCallbackFn readCallback,
    MMIOWriteCallbackFn writeCallback)
{
    // Allocate callbacks and append to callbacks for lifetime storage
    auto readCallbackPtr = std::make_unique<MMIOReadCallbackFn>(std::move(readCallback));
    auto writeCallbackPtr = std::make_unique<MMIOWriteCallbackFn>(std::move(writeCallback));

    // Map MMIO
    uc_err err = uc_mmio_map(mEngine, address, size,
        [](uc_engine* uc, uint64_t offset, unsigned size, void* userData) -> uint64_t {
            MMIOReadCallbackFn* cb = static_cast<MMIOReadCallbackFn*>(userData);

            return (*cb)(static_cast<uint32_t>(offset), size);
        }, readCallbackPtr.get(),
        [](uc_engine* uc, uint64_t offset, unsigned size, uint64_t value, void* userData) -> void {
            MMIOWriteCallbackFn* cb = static_cast<MMIOWriteCallbackFn*>(userData);

            (*cb)(static_cast<uint32_t>(offset), size, static_cast<uint32_t>(value));
        }, writeCallbackPtr.get()
    );
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to map MMIO");
    }

    mMMIOReadCallbacks.push_back(std::move(readCallbackPtr));
    mMMIOWriteCallbacks.push_back(std::move(writeCallbackPtr));
}

bool UnicornEngine::WriteMemory(uint32_t address, const void* ptr, size_t size)
{
    return uc_mem_write(mEngine, address, ptr, size) == UC_ERR_OK;
}

bool UnicornEngine::ReadMemory(uint32_t address, void* ptr, size_t size)
{
    return uc_mem_read(mEngine, address, ptr, size) == UC_ERR_OK;
}

uintptr_t UnicornEngine::AddBreakpoint(uint32_t address, uint32_t size, std::function<void(void)> callback)
{
    auto bp = std::make_unique<Breakpoint>();
    bp->address = address;
    bp->size = size;
    bp->callback = std::move(callback);

    uc_err err = uc_hook_add(mEngine, &bp->hook,
        UC_HOOK_CODE,
        reinterpret_cast<void*>(static_cast<uc_cb_hookcode_t>([](uc_engine* uc, uint64_t address, uint32_t size, void* userData) -> void {
            auto bp = static_cast<Breakpoint*>(userData);

            // Create a copy of the callback before calling it, as the pointer might get freed while in the callback
            auto cb = bp->callback;
            bp = nullptr;

            cb();
        })), bp.get(), address, address + size
    );
    if (err != UC_ERR_OK) {
        EMU_FATAL("Failed to add hook: {} ({})", static_cast<int>(err), uc_strerror(err));
    }

    // Use the underlying unicorn hook handle as a handle
    uintptr_t handle = static_cast<uintptr_t>(bp->hook);
    mBreakpoints.emplace(handle, std::move(bp));
    return handle;
}

void UnicornEngine::RemoveBreakpoint(uintptr_t handle)
{
    auto bp = mBreakpoints.find(handle);
    if (bp == mBreakpoints.end()) {
        EMU_FATAL("Invalid breakpoint handle");
        return;
    }

    uc_hook_del(mEngine, bp->second->hook);
    mBreakpoints.erase(bp);
}

void UnicornEngine::AddWatchpoint(uint32_t address, uint32_t size, WatchpointType type, std::function<void(void)> callback)
{
    std::lock_guard lk(mWatchpointMutex);

    auto wp = std::make_unique<Watchpoint>();
    wp->address = address;
    wp->size = size;
    wp->type = type;
    // we can't add a hook while still in the hook callback, so create it on next code
    wp->hook = static_cast<uc_hook>(-1);
    wp->callback = std::move(callback);
    mWatchpoints.push_back(std::move(wp));
    mUpdateMissingWatchpoints = true;
}

void UnicornEngine::RemoveWatchpoint(uint32_t address, uint32_t size, WatchpointType type)
{
    std::lock_guard lk(mWatchpointMutex);

    for (auto it = mWatchpoints.begin(); it != mWatchpoints.end();) {
        if ((*it)->address == address && (*it)->size == size && (*it)->type == type) {
            if ((*it)->hook != static_cast<uc_hook>(-1)) {
                uc_hook_del(mEngine, (*it)->hook);
            }
            it = mWatchpoints.erase(it);
        } else {
            it++;
        }
    }
}

void UnicornEngine::CreateMissingWatchpointHooks()
{
    if (!mUpdateMissingWatchpoints) {
        return;
    }

    std::lock_guard lk(mWatchpointMutex);

    for (auto& wp : mWatchpoints) {
        if (wp->hook == static_cast<uc_hook>(-1)) {
            uc_hook_add(mEngine, &wp->hook, ConvertWatchpointTypeToUCHook(wp->type),
                reinterpret_cast<void*>(static_cast<uc_cb_hookmem_t>([](uc_engine* uc, uc_mem_type type, uint64_t address, int size, int64_t value, void* userData) -> void {
                    Watchpoint* wp = static_cast<Watchpoint*>(userData);

                    // Create a copy of the callback before calling it, as the pointer might get freed while in the callback
                    auto cb = wp->callback;
                    wp = nullptr;

                    cb();
                })), wp.get(), wp->address, wp->address + wp->size - 1
            );
        }
    }

    mUpdateMissingWatchpoints = false;
}

} // namespace emu
