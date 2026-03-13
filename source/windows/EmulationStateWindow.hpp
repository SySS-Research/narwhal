#pragma once

#include "Window.hpp"

class EmulationStateWindow : public emu::Window
{
public:
    EmulationStateWindow();
    virtual ~EmulationStateWindow();

    virtual void Draw() override;
};
