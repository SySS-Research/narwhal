#include "DisassemblyWindow.hpp"
#include "Application.hpp"

#include <imgui.h>

DisassemblyWindow::DisassemblyWindow()
 : Window("Disassembly View")
{
}

DisassemblyWindow::~DisassemblyWindow()
{
}

void DisassemblyWindow::Draw()
{
    emu::Emulator* emulator = emu::Application::Get()->GetEmulator();

    uint32_t currentPC = emulator->GetRegister(arm::Register::PC);

    // Read some instructions around the PC
    uint32_t startPC = currentPC - 0x10;
    std::vector<uint8_t> opcodes(0x20);
    if (!emulator->ReadMemory(startPC, opcodes)) {
        ImGui::Text("Failed to read memory at 0x%08x\n", startPC);
        return;     
    }

    auto instructions = capstone::DisassembleAll(startPC, opcodes);
    if (instructions.empty()) {
        // We might have hit a 4-byte instruction try again at offset 2
        startPC += 2;
        instructions = capstone::DisassembleAll(startPC, std::span(opcodes).subspan(2));
        if (instructions.empty()) {
            ImGui::Text("Failed to disassemble at 0x%08x\n", startPC);
            return;
        }
    }

    // Find current instruction
    auto currentIt = std::find_if(instructions.begin(), instructions.end(), [currentPC](auto& insn){
        return insn.address == currentPC;
    });
    if (currentIt == instructions.end()) {
        ImGui::Text("Cannot find current PC in instructions");
        return;
    }

    // Print the previous 3 instructions
    size_t preceeding = std::min<size_t>(currentIt - instructions.begin(), 3);
    for (auto it = currentIt - preceeding; it != currentIt; it++) {
        ImGui::Text("%08x: %s %s", it->address, it->mnemonic.c_str(), it->op.c_str());
    }

    ImGui::Text("->%08x: %s %s", currentIt->address, currentIt->mnemonic.c_str(), currentIt->op.c_str());

    // Print the following 3 instructions
    size_t following = std::min<size_t>(instructions.end() - currentIt - 1, 3);
    for (auto it = currentIt + 1; it != currentIt + following + 1; it++) {
        ImGui::Text("%08x: %s %s", it->address, it->mnemonic.c_str(), it->op.c_str());
    }
}
