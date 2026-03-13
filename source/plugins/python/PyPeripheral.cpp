#include "PyPeripheral.hpp"

#include <pybind11/pybind11.h>

namespace emu
{

void PyPeripheral::Init()
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_NAME(void, Peripheral, "init", Init);
}

void PyPeripheral::Update()
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_NAME(void, Peripheral, "init", Update);
}

uint32_t PyPeripheral::Read(uint32_t offset, uint32_t size)
{
    pybind11::gil_scoped_acquire g{};

    PYBIND11_OVERRIDE_PURE_NAME(uint32_t, Peripheral, "read", Read, offset, size);
}

void PyPeripheral::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    PYBIND11_OVERRIDE_PURE_NAME(void, Peripheral, "write", Read, offset, size, value);
}

} // namespace emu
