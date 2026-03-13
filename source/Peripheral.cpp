#include "Peripheral.hpp"

namespace emu
{

Peripheral::Peripheral()
 : mEmulator(nullptr),
 mStart(~0),
 mEnd(~0)
{
}

Peripheral::~Peripheral()
{
}

void Peripheral::SetBounds(uint32_t start, uint32_t end)
{
    mStart = start;
    mEnd = end;
}


} // namespace emu
