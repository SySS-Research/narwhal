#pragma once

#include "Emulator.hpp"

namespace FileLoader {

bool LoadBin(emu::Emulator* emulator, uint32_t address, const std::string& path, size_t offset = 0);

bool LoadElf(emu::Emulator* emulator, const std::string& path);

bool LoadSREC(emu::Emulator* emulator, const std::string& path, uint32_t& outStartAddress);

void RegisterLoaders();

} // namespace FileLoader
