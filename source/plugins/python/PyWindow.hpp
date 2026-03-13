#pragma once

#include "Window.hpp"

#include <pybind11/trampoline_self_life_support.h>

namespace emu
{

class __attribute__((visibility("hidden")))
    PyWindow : public Window, public pybind11::trampoline_self_life_support
{
public:
    using Window::Window; // Inherit constructors

    void Init() override;
    void Draw() override;

private:

};

} // namespace emu
