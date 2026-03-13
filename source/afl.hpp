#pragma once

#include <cstdint>
#include <span>

namespace afl
{

bool StartForkserver();

bool ForkserverRunning();

std::span<uint8_t> GetFuzzData();

void UpdateCodeCoverage(uint32_t address);

// TODO
void RaiseCrash();

} // namespace afl
