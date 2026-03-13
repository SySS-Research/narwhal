#include "EmulationStateWindow.hpp"
#include "Application.hpp"

#include <imgui.h>

EmulationStateWindow::EmulationStateWindow()
 : Window("Emulation State")
{
}

EmulationStateWindow::~EmulationStateWindow()
{
}

void EmulationStateWindow::Draw()
{
    emu::Emulator* emulator = emu::Application::Get()->GetEmulator();

    if (ImGui::Button(emulator->IsRunning() ? "Stop" : "Start")) {
        if (emulator->IsRunning()) {
            emulator->Stop();
        } else {
            emulator->Start();
        }
    }

    ImGui::Text("R0 = 0x%08x", emulator->GetRegister(arm::Register::R0));
    ImGui::Text("R1 = 0x%08x", emulator->GetRegister(arm::Register::R1));
    ImGui::Text("R2 = 0x%08x", emulator->GetRegister(arm::Register::R2));
    ImGui::Text("R3 = 0x%08x", emulator->GetRegister(arm::Register::R3));
    ImGui::Text("R4 = 0x%08x", emulator->GetRegister(arm::Register::R4));
    ImGui::Text("R5 = 0x%08x", emulator->GetRegister(arm::Register::R5));
    ImGui::Text("R6 = 0x%08x", emulator->GetRegister(arm::Register::R6));
    ImGui::Text("R7 = 0x%08x", emulator->GetRegister(arm::Register::R7));
    ImGui::Text("R8 = 0x%08x", emulator->GetRegister(arm::Register::R8));
    ImGui::Text("R9 = 0x%08x", emulator->GetRegister(arm::Register::R9));
    ImGui::Text("R10 = 0x%08x", emulator->GetRegister(arm::Register::R10));
    ImGui::Text("R11 = 0x%08x", emulator->GetRegister(arm::Register::R11));
    ImGui::Text("R12 = 0x%08x", emulator->GetRegister(arm::Register::R12));

    ImGui::Separator();

    ImGui::Text("SP = 0x%08x", emulator->GetRegister(arm::Register::SP));
    ImGui::Text("LR = 0x%08x", emulator->GetRegister(arm::Register::LR));
    ImGui::Text("PC = 0x%08x", emulator->GetRegister(arm::Register::PC));
    ImGui::Text("XPSR = 0x%08x", emulator->GetRegister(arm::Register::XPSR));
}
