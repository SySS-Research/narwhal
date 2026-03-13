#pragma once

#include "Peripheral.hpp"

#include <pybind11/trampoline_self_life_support.h>

namespace emu
{

class __attribute__((visibility("hidden")))
    PyPeripheral : public Peripheral, public pybind11::trampoline_self_life_support
{
public:
    using Peripheral::Peripheral; // Inherit constructors

    void Init() override;
    void Update() override;
    uint32_t Read(uint32_t offset, uint32_t size) override;
    void Write(uint32_t offset, uint32_t size, uint32_t value) override;

private:

};

} // namespace emu
