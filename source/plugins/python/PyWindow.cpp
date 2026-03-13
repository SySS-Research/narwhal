#include "PyWindow.hpp"

#include <pybind11/pybind11.h>

namespace emu
{

void PyWindow::Init()
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_NAME(void, Window, "init", Init);
}

void PyWindow::Draw()
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_PURE_NAME(void, Window, "draw", Draw);
}

} // namespace emu
