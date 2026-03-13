#pragma once

#include "common.hpp"
#include "Connector.hpp"

namespace emu
{

class Emulator;

class Peripheral : public IConnectable
{
public:
    Peripheral();
    virtual ~Peripheral();

    virtual void Init() { }
    virtual void Update() { }
    virtual uint32_t Read(uint32_t offset, uint32_t size) = 0;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) = 0;

    void SetBounds(uint32_t start, uint32_t end);

    uint32_t GetStart() const { return mStart; }
    uint32_t GetEnd() const { return mEnd; }
    uint32_t GetSize() const { return mEnd - mStart + 1; }

    void SetEmulator(Emulator* emulator)
    {
        mEmulator = emulator;
    }

protected:
    Emulator* mEmulator;

    uint32_t mStart;
    uint32_t mEnd;
};

} // namespace emu
