#include "Emulator.hpp"
#include "peripherals/arm.hpp"
#include "afl.hpp"

#include <utility>

namespace emu
{

Emulator::Emulator()
 : mEngine(arm::CPUModel::Default),
 mEmulationThread(),
 mIsRunning(false),
 mRunningCond(),
 mRunningMutex(),
 mMMIORegions(),
 mNVIC(),
 mSCB(),
 mGDBStub(3000),
 mTraceEnabled(false),
 mCoverageCollector(nullptr)
{
    // Register required hooks
    mEngine.OnPeriodicUpdate([this](){
        // TODO Without this a crash happens sometimes when fuzzing?
        if (!mIsRunning) {
            return;
        }

        PeriodicUpdate();

        // Make sure exceptions are processed _after_ other PeriodicUpdates
        mNVIC->ProcessPendingExceptions();
    });
    mEngine.OnUnmappedMemory([this](MemoryFlags flags, uint32_t address, uint32_t size, uint32_t value){
        OnUnmappedMemory(flags, address, size, value);
    });
    mEngine.OnBlock([this](uint32_t address, uint32_t size){
        if (mCoverageCollector) {
            mCoverageCollector->UpdateCodeCoverage(address, size);
        }

        afl::UpdateCodeCoverage(address);
    });
    mEngine.OnCode([this](uint32_t address, uint32_t size){
        OnCode(address, size);
    });
    mEngine.OnException([this](arm::Exception exception){
        OnException(exception);
    });
    mEngine.OnExceptionReturn([this](arm::EXC_RETURN excReturn){
        mNVIC->HandleExceptionReturn(excReturn);
    });

    // Map system control space
    // See Arm®v7-M Architecture Reference Manual - B3.2 System Control Space (SCS)
    // System Control Space, address range 0xE000E000 to 0xE000EFFF
    // TODO register the Private Peripheral Bus (PPB) range here instead?
    MapMemory(0xE000E000, 0x1000, MemoryFlags::MMIO);

    // Register private peripherals
    RegisterPeripheral((mNVIC = std::make_shared<NVIC>()));
    RegisterPeripheral((mSCB = std::make_shared<SCB>()));
    // TODO systick messes with fuzzing coverage
    if (!afl::ForkserverRunning()) {
        RegisterPeripheral(std::make_shared<SysTick>());
    }

    // Make sure emulation thread is only started once everything is set up
    mEmulationThread = std::jthread(&Emulator::EmulationThread, this);
}

Emulator::~Emulator()
{
    mEmulationThread.request_stop();
    mRunningCond.notify_all();
}

void Emulator::Start()
{
    {
        std::scoped_lock lk(mRunningMutex);
        mIsRunning = true;
    }

    mRunningCond.notify_one();
}

void Emulator::Stop()
{
    {
        std::scoped_lock lk(mRunningMutex);
        mIsRunning = false;
    }

    mEngine.Stop();
}

void Emulator::Reset(uint32_t vtorAddress)
{
    mSCB->SetVTOR(vtorAddress);

    // See Arm®v7-M Architecture Reference Manual - B1.5.3 The vector table
    SetRegister(arm::Register::SP, ReadMemory<uint32_t>(vtorAddress));
    SetRegister(arm::Register::PC, ReadMemory<uint32_t>(vtorAddress + 4));
}

void Emulator::SetRegister(arm::Register reg, uint32_t val)
{
    return mEngine.SetRegister(reg, val);
}

uint32_t Emulator::GetRegister(arm::Register reg)
{
    return mEngine.GetRegister(reg);
}

void Emulator::MapMemory(uint32_t address, uint32_t size, MemoryFlags flags)
{
    if (!(flags & MemoryFlags::MMIO)) { // Map regular memory
        mEngine.MapMemory(address, size, flags & ~MemoryFlags::MMIO);
    } else { // Map MMIO region
        auto region = std::make_shared<MMIORegion>();
        region->start = address;
        region->end = address + size;
        mMMIORegions.push_back(region);

        mEngine.MapMMIO(address, size,
            [this, region = std::weak_ptr(region)](uint32_t offset, uint32_t size) -> uint32_t {
                return OnMMIORegionRead(region.lock(), offset, size);
            },
            [this, region = std::weak_ptr(region)](uint32_t offset, uint32_t size, uint32_t value) -> void {
                OnMMIORegionWrite(region.lock(), offset, size, value);
            }
        );
    }
}

bool Emulator::WriteMemory(uint32_t address, const void* ptr, size_t size)
{
    return mEngine.WriteMemory(address, ptr, size);
}

bool Emulator::WriteMemory(uint32_t address, const std::span<const uint8_t>& bytes)
{
    return WriteMemory(address, bytes.data(), bytes.size());
}

bool Emulator::ReadMemory(uint32_t address, void* ptr, size_t size)
{
    return mEngine.ReadMemory(address, ptr, size);
}

bool Emulator::ReadMemory(uint32_t address, const std::span<uint8_t>& bytes)
{
    return ReadMemory(address, bytes.data(), bytes.size());
}

bool Emulator::WriteMMIO(uint32_t address, uint32_t size, uint32_t value)
{
    for (auto& region : mMMIORegions) {
        if (address >= region->start && address + size < region->end) {
            OnMMIORegionWrite(region, address - region->start, size, value);
            return true;
        }
    }

    // Also check unmanaged MMIO if we haven't found one yet
    for (auto& mmio : mUnmanagedMMIO) {
        if (address >= mmio.start && address + size < mmio.end) {
            mmio.writeCallback(address - mmio.start, size, value);
            return true;
        }
    }

    return false;
}

void Emulator::RegisterPeripheral(std::shared_ptr<Peripheral> peripheral)
{
    peripheral->SetEmulator(this);

    auto region = FindMMIORegionForPeripheral(peripheral);
    if (!region) {
        EMU_FATAL("Trying to register peripheral in unmapped memory");
    }

    for (auto& otherPeriph : region->peripherals) {
        if (peripheral->GetStart() >= otherPeriph->GetStart() && peripheral->GetEnd() <= otherPeriph->GetEnd()) {
            EMU_FATAL("Trying to register overlapping peripheral");
        }
    }

    region->peripherals.push_back(peripheral);
}

void Emulator::RegisterUnmanagedMMIO(uint32_t address, uint32_t size,
    UnicornEngine::MMIOReadCallbackFn readCallback,
    UnicornEngine::MMIOWriteCallbackFn writeCallback)
{
    // Register with engine
    mEngine.MapMMIO(address, size, readCallback, writeCallback);

    // Need to also register internally for DMA
    auto& mmio = mUnmanagedMMIO.emplace_back();
    mmio.start = address;
    mmio.end = address + size;
    mmio.readCallback = std::move(readCallback);
    mmio.writeCallback = std::move(writeCallback);
}

uintptr_t Emulator::AddBreakpoint(uint32_t address, uint32_t size, std::function<void(void)> callback)
{
    return mEngine.AddBreakpoint(address, size, std::move(callback));
}

void Emulator::RemoveBreakpoint(uintptr_t handle)
{
    mEngine.RemoveBreakpoint(handle);
}

void Emulator::AddWatchpoint(uint32_t address, uint32_t size, WatchpointType type)
{
    mEngine.AddWatchpoint(address, size, type,
        [this, address, type](){
            mGDBStub.OnWatchpoint(address, type);
        }
    );
}

void Emulator::RemoveWatchpoint(uint32_t address, uint32_t size, WatchpointType type)
{
    mEngine.RemoveWatchpoint(address, size, type);
}

void Emulator::SetExceptionPending(arm::Exception exception)
{
    mNVIC->SetExceptionPending(exception);
}

void Emulator::PrintContext(emu::LogType type)
{
    EMU_LOG(type, "R0 = 0x{:08x}", GetRegister(arm::Register::R0));
    EMU_LOG(type, "R1 = 0x{:08x}", GetRegister(arm::Register::R1));
    EMU_LOG(type, "R2 = 0x{:08x}", GetRegister(arm::Register::R2));
    EMU_LOG(type, "R3 = 0x{:08x}", GetRegister(arm::Register::R3));
    EMU_LOG(type, "R4 = 0x{:08x}", GetRegister(arm::Register::R4));
    EMU_LOG(type, "R5 = 0x{:08x}", GetRegister(arm::Register::R5));
    EMU_LOG(type, "R6 = 0x{:08x}", GetRegister(arm::Register::R6));
    EMU_LOG(type, "R7 = 0x{:08x}", GetRegister(arm::Register::R7));
    EMU_LOG(type, "R8 = 0x{:08x}", GetRegister(arm::Register::R8));
    EMU_LOG(type, "R9 = 0x{:08x}", GetRegister(arm::Register::R9));
    EMU_LOG(type, "R10 = 0x{:08x}", GetRegister(arm::Register::R10));
    EMU_LOG(type, "R11 = 0x{:08x}", GetRegister(arm::Register::R11));
    EMU_LOG(type, "R12 = 0x{:08x}", GetRegister(arm::Register::R12));

    EMU_LOG(type, "SP = 0x{:08x}", GetRegister(arm::Register::SP));
    EMU_LOG(type, "LR = 0x{:08x}", GetRegister(arm::Register::LR));
    EMU_LOG(type, "PC = 0x{:08x}", GetRegister(arm::Register::PC));
    EMU_LOG(type, "XPSR = 0x{:08x}", GetRegister(arm::Register::XPSR));
}

std::optional<capstone::Instruction> Emulator::Disassemble(uint32_t address)
{
    std::array<uint8_t, 4> opcode;
    ReadMemory(address, opcode);

    return capstone::Disassemble(address, opcode);
}

void Emulator::EnableGDBStub()
{
    mGDBStub.Init(this);
}

void Emulator::EmulationThread(std::stop_token stoken)
{
    while (!stoken.stop_requested()) {
        std::unique_lock lk(mRunningMutex);
        if (!mRunningCond.wait(lk, stoken, [&]{ return !!mIsRunning; })) {
            break;
        }
        // TODO are we allowed do this?
        lk.unlock();

        mEngine.Run();
    }
}

void Emulator::PeriodicUpdate()
{
    for (auto region : mMMIORegions) {
        for (auto peripheral : region->peripherals) {
            peripheral->Update();
        }
    }
}

std::shared_ptr<Emulator::MMIORegion> Emulator::FindMMIORegionForPeripheral(std::shared_ptr<Peripheral> peripheral)
{
    for (auto region : mMMIORegions) {
        if (peripheral->GetStart() >= region->start && peripheral->GetEnd() < region->end) {
            return region;
        }
    }

    return nullptr;
}

std::shared_ptr<Peripheral> Emulator::FindPeripheral(std::shared_ptr<MMIORegion> region, uint32_t address)
{
    for (auto& peripheral : region->peripherals) {
        if (address >= peripheral->GetStart() && address < peripheral->GetEnd()) {
            return peripheral;
        }
    }

    return nullptr;
}

uint32_t Emulator::OnMMIORegionRead(std::shared_ptr<MMIORegion> region, uint32_t offset, uint32_t size)
{
    const uint32_t address = region->start + offset;

    auto peripheral = FindPeripheral(region, address);
    if (peripheral) {
        return peripheral->Read(address - peripheral->GetStart(), size);
    }

    EMU_LOG_WARN("Unknown MMIO Read 0x{:08x}, size = {}", address, size);
    return 0;
}

void Emulator::OnMMIORegionWrite(std::shared_ptr<MMIORegion> region, uint32_t offset, uint32_t size, uint32_t value)
{
    const uint32_t address = region->start + offset;

    auto peripheral = FindPeripheral(region, address);
    if (peripheral) {
        return peripheral->Write(address - peripheral->GetStart(), size, value);
    }

    EMU_LOG_WARN("Unknown MMIO Write 0x{:08x}, size = {}, value = 0x{:08x}", address, size, value);
    return;
}

void Emulator::OnUnmappedMemory(MemoryFlags type, uint32_t address, uint32_t size, uint32_t value)
{
    switch (type) {
    case MemoryFlags::Read:
        EMU_LOG_WARN("Unmapped read at 0x{:08x}, size = {}", address, size);
        break;
    case MemoryFlags::Write:
        EMU_LOG_WARN("Unmapped write to 0x{:08x}, size = {}, value = 0x{:08x}", address, size, value);
        break;
    case MemoryFlags::Exec:
        // Unmapped fetch is always an error
        EMU_LOG_ERROR("Unmapped fetch at 0x{:08x}, size = {}", address, size);
        mGDBStub.Break();
        break;
    default:
        break;
    }

    // Break on unmapped read/write
    // mGDBStub.Break();
}

void Emulator::OnCode(uint32_t address, uint32_t size)
{
    if (mTraceEnabled) {
        auto instr = Disassemble(address);
        if (instr) {
            // Use std::println directly here to print to stdout without log level
            std::println("{:08x}: {} {}", instr->address, instr->mnemonic, instr->op);
        }
    }

    // This can block if the client/breakpoint requests to stop
    mGDBStub.OnCode(address);
}

void Emulator::OnException(arm::Exception exception)
{
    switch (exception) {
        case arm::Exception::SVCall:
            // Use "HandleException" to immediately process the interrupt
            mNVIC->HandleException(exception);
            return;
        default:
            break;
    }

    EMU_LOG_WARN("Unhandled interrupt {}", static_cast<int>(exception));

    // Raise crash for fuzzing
    if (afl::ForkserverRunning()) {
        afl::RaiseCrash();
    }

    mGDBStub.Break();
}

} // namespace emu
