#pragma once

#include "Window.hpp"

class DisassemblyWindow : public emu::Window
{
public:
    DisassemblyWindow();
    virtual ~DisassemblyWindow();

    virtual void Draw() override;
};
