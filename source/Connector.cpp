#include "Connector.hpp"

namespace emu
{

IConnectable::IConnectable()
 : mInputConnectors(),
 mOutputConnectors(),
 mIdentifier(),
 mName()
{
}

IConnectable::~IConnectable()
{
}

} // namespace emu
