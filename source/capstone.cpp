#include "capstone.hpp"

#include <capstone/capstone.h>

std::optional<capstone::Instruction> capstone::Disassemble(uint32_t address, const std::span<const uint8_t>& bytes)
{
    csh handle;
    cs_insn* insn;
    size_t count;

    if (cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &handle) != CS_ERR_OK) {
        return std::nullopt;
    }

    count = cs_disasm(handle, bytes.data(), bytes.size(), address, 1, &insn);
    if (count > 0) {
        Instruction instruction{};
        instruction.address = insn->address;
        instruction.size = insn->size;
        instruction.mnemonic = insn->mnemonic;
        instruction.op = insn->op_str;

        cs_free(insn, count);
        cs_close(&handle);

        return instruction;
    }

    cs_close(&handle);
    return std::nullopt;
}

std::vector<capstone::Instruction> capstone::DisassembleAll(uint32_t address, const std::span<const uint8_t>& bytes)
{
    csh handle;
    cs_insn* insn;
    size_t count;
    std::vector<Instruction> instructions;

    if (cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &handle) != CS_ERR_OK) {
        return instructions;
    }

    count = cs_disasm(handle, bytes.data(), bytes.size(), address, 0, &insn);
    if (count > 0) {
        instructions.reserve(count);
        for (size_t i = 0; i < count; i++) {
            Instruction& instruction = instructions.emplace_back();
            instruction.address = insn[i].address;
            instruction.size = insn[i].size;
            instruction.mnemonic = insn[i].mnemonic;
            instruction.op = insn[i].op_str;
        }

        cs_free(insn, count);
        cs_close(&handle);

        return instructions;
    }

    cs_close(&handle);
    return instructions;
}
