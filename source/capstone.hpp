#pragma once

#include "common.hpp"

namespace capstone
{

struct Instruction
{
    uint32_t address;
    uint16_t size;
    std::string mnemonic;
    std::string op;
};

std::optional<Instruction> Disassemble(uint32_t address, const std::span<const uint8_t>& bytes);

std::vector<Instruction> DisassembleAll(uint32_t address, const std::span<const uint8_t>& bytes);

} // namespace capstone

