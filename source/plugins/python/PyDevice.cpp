#include "PyDevice.hpp"

#include <pybind11/pybind11.h>

namespace emu
{

void PyDevice::Init()
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_NAME(void, Device, "init", Init);
}

void PyDevice::DrawNode()
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_NAME(void, Device, "draw_node", DrawNode);
}

} // namespace emu
