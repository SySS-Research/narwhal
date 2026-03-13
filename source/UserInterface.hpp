#pragma once

#include "Emulator.hpp"
#include "Window.hpp"

#include <SDL3/SDL.h>

// TODO
#include <imgui.h>
void ImageRotated(ImTextureID user_texture_id, const ImVec2& size, int angle, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1,1));

namespace emu
{

class UserInterface
{
public:
    UserInterface(Emulator* emulator);
    ~UserInterface();

    void Run();

    SDL_Texture* CreateRGBTexture(const void* pixels, int width, int height, bool autoFree = false);
    void DestroyRGBTexture(SDL_Texture* texture);

private:
    void Draw();
    void MainMenuBar();

    Emulator* mEmulator;
    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    bool mIsRunning;

    bool mShowDemo;

    std::list<std::shared_ptr<Window>> mWindows;
    std::list<SDL_Texture*> mTextureFreeList;
};

} // namespace emu
