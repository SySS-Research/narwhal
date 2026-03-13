#pragma once

#include "Window.hpp"
#include "devices/ST7789.hpp"

class ST7789Window : public emu::Window
{
public:
    ST7789Window(ST7789* display);
    virtual ~ST7789Window();

    virtual void Draw() override;

private:
    ST7789* mDisplay;
};
