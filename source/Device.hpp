#pragma once

#include "Connector.hpp"

namespace emu
{

class Device : public IConnectable
{
public:
    Device();
    virtual ~Device();

    virtual void Init();

    virtual void DrawNode() { }
};

} // namespace emu
