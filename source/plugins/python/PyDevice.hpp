#pragma once

#include "Device.hpp"

#include <pybind11/trampoline_self_life_support.h>

namespace emu
{

class __attribute__((visibility("hidden")))
    PyDevice : public Device, public pybind11::trampoline_self_life_support
{
public:
    using Device::Device; // Inherit constructors

    void Init() override;

    void DrawNode() override;

private:

};

} // namespace emu
