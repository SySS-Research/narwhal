#pragma once

#include <string>

namespace emu
{

class Window
{
public:
    Window(const std::string& name);
    virtual ~Window();

    virtual void Init() {}
    virtual void Draw() = 0;

    const std::string& GetName() const { return mName; }
    bool IsVisible() const { return mVisible; }
    void SetVisible(bool visible) { mVisible = visible; }

protected:
    std::string mName;
    bool mVisible;
};

} // namespace emu
