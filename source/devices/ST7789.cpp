#include "ST7789.hpp"
#include "Application.hpp"
#include "Registry.hpp"
#include "windows/ST7789Window.hpp"

ST7789::ST7789() : Device(),
 mSelectConnectorView("select"),
 mCommandConnectorView("command"),
 mDataConnectorView("data"),
 mCurrentCommand(),
 mWriteOffset(),
 mPixels()
{
    // w x h RGB
    mPixels.resize(GetWidth() * GetHeight() * 3);
    std::fill(mPixels.begin(), mPixels.end(), 0xFF);

    GetInputConnectors().Add(mSelectConnectorView);
    GetInputConnectors().Add(mCommandConnectorView);
    GetInputConnectors().Add(mDataConnectorView);

    mSelectConnectorView.OnCall([this](bool selected) {
        if (!selected) {
            // Command complete
            if (mCurrentCommand == 0x2c) {
                mWriteOffset = 0;
            }
        }
    });

    mCommandConnectorView.OnCall([this](uint8_t command) {
        mCurrentCommand = command;
    });

    mDataConnectorView.OnCall([this](uint32_t data, size_t size) {
        if (mCurrentCommand == 0x2c && size == 2) {
            if (mWriteOffset < mPixels.size()) {
                mPixels[mWriteOffset++] = static_cast<uint8_t>(data >> 8);
                mPixels[mWriteOffset++] = static_cast<uint8_t>(data & 0xFF);
            }
        }
    });
}

ST7789::~ST7789()
{
}

void ST7789::Init()
{
    // TODO this is ... not great
    emu::Registry::RegisterWindow<ST7789Window>(this);
}

void ST7789::DrawNode()
{
    auto width = GetWidth();
    auto height = GetHeight();

    auto ui = emu::Application::Get()->GetUI();

    auto texture = ui->CreateRGBTexture(GetPixels().data(), width, height, true);
    ImageRotated(reinterpret_cast<ImTextureID>(texture), ImVec2(width, height), 270);
}
