#include "SerialConsoleWindow.hpp"

#include <imgui.h>

SerialConsoleWindow::SerialConsoleWindow(SerialConsole* console)
 : Window("Serial Console"),
 mConsole(console),
 mAutoScroll(true)
{
}

SerialConsoleWindow::~SerialConsoleWindow()
{
}

void SerialConsoleWindow::Draw()
{
    ImGui::Checkbox("Auto-scroll", &mAutoScroll);

    if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::TextUnformatted(mConsole->GetBuffer().c_str());
        ImGui::PopStyleVar();

        if (mAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }

    ImGui::EndChild();
}
