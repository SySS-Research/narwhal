#pragma once

#include <cstdint>
#include <span>

namespace afl
{

bool StartForkserver();

bool ForkserverRunning();

std::span<const uint8_t> GetFuzzData();

void UpdateCodeCoverage(uint32_t address);

void UpdateCmpCov(uint32_t address, uint32_t arg1, uint32_t arg2, uint32_t size);

void StartLoop();

void EndLoop();

// TODO
void RaiseCrash();

} // namespace afl
