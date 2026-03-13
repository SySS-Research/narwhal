#include "ST7789Window.hpp"
#include "Application.hpp"

ST7789Window::ST7789Window(ST7789* display) : Window("ST7789"),
 mDisplay(display)
{
}

ST7789Window::~ST7789Window()
{
}

void ST7789Window::Draw()
{
    auto width = mDisplay->GetWidth();
    auto height = mDisplay->GetHeight();

    auto ui = emu::Application::Get()->GetUI();

    auto texture = ui->CreateRGBTexture(mDisplay->GetPixels().data(), width, height, true);
    ImageRotated(reinterpret_cast<ImTextureID>(texture), ImVec2(width, height), 270);
}
