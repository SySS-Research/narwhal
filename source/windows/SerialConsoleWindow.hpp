#pragma once

#include "Window.hpp"
#include "devices/SerialConsole.hpp"

class SerialConsoleWindow : public emu::Window
{
public:
    SerialConsoleWindow(SerialConsole* console);
    virtual ~SerialConsoleWindow();

    virtual void Draw() override;

private:
    SerialConsole* mConsole;
    bool mAutoScroll;
};
