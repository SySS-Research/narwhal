#include "Window.hpp"

namespace emu
{

Window::Window(const std::string& name)
 : mName(name),
 mVisible(true)
{
}

Window::~Window()
{
}

} // namespace emu
