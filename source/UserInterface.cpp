#define IMGUI_DEFINE_MATH_OPERATORS
#include "UserInterface.hpp"
#include "Registry.hpp"
#include "common.hpp"

#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imnodes.h>

// https://github.com/ocornut/imgui/pull/5386
void ImageRotated(ImTextureID user_texture_id, const ImVec2& size, int angle, const ImVec2& uv0, const ImVec2& uv1)
{
    IM_ASSERT(angle % 90 == 0);
    ImVec2 _uv0, _uv1, _uv2, _uv3;
    switch(angle % 360)
    {
    case 0:
        ImGui::Image(user_texture_id, size, uv0, uv1);
        return;
    case 180:
        ImGui::Image(user_texture_id, size, uv1, uv0);
        return;
    case 90:
        _uv3 = uv0;
        _uv1 = uv1;
        _uv0 = ImVec2(uv1.x, uv0.y);
        _uv2 = ImVec2(uv0.x, uv1.y);
        break;
    case 270:
        _uv1 = uv0;
        _uv3 = uv1;
        _uv0 = ImVec2(uv0.x, uv1.y);
        _uv2 = ImVec2(uv1.x, uv0.y);
        break;
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;
    ImVec2 _size(size.y, size.x);
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + _size);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, 0))
        return;


    ImVec2 x1 = ImVec2(bb.Max.x, bb.Min.y);
    ImVec2 x3 = ImVec2(bb.Min.x, bb.Max.y);
    window->DrawList->AddImageQuad(user_texture_id, bb.Min, x1, bb.Max, x3, _uv0, _uv1, _uv2, _uv3);
}

namespace emu
{

UserInterface::UserInterface(Emulator* emulator)
 : mEmulator(emulator),
 mWindow(nullptr),
 mRenderer(nullptr),
 mIsRunning(true),
 mShowDemo(false),
 mWindows(Registry::GetWindows()), // TODO who owns the windows?
 mTextureFreeList()
{
    // TODO force wayland if available, since it's not used by default on niri
    // https://jackjamison.net/blog/sdl-native-wayland/
    for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
        if (std::string(SDL_GetVideoDriver(i)) == "wayland") {
            SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
            break;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        EMU_FATAL("Failed to init SDL: {}", SDL_GetError());
    }

    float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (!SDL_CreateWindowAndRenderer("Emu",
        static_cast<int>(1280 * scale), static_cast<int>(720 * scale),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &mWindow, &mRenderer)) {
        EMU_FATAL("Failed to init SDL");
    }

    SDL_SetRenderVSync(mRenderer, 1);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;

}

UserInterface::~UserInterface()
{
    SDL_DestroyRenderer(mRenderer);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();
}

void UserInterface::Run()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL3_InitForSDLRenderer(mWindow, mRenderer);
    ImGui_ImplSDLRenderer3_Init(mRenderer);

    // TODO where
    for (auto w : mWindows) {
        w->Init();
    }

    while (mIsRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                mIsRunning = false;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        // Draw UI
        Draw();

        SDL_SetRenderScale(mRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        SDL_SetRenderDrawColorFloat(mRenderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);

        SDL_RenderClear(mRenderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);
        SDL_RenderPresent(mRenderer);

        for (auto t : mTextureFreeList) {
            SDL_DestroyTexture(t);
        }
        mTextureFreeList.clear();
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImNodes::DestroyContext();
    ImGui::DestroyContext();
}

SDL_Texture* UserInterface::CreateRGBTexture(const void* pixels, int width, int height, bool autoFree)
{
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_BGR24, (void*)(pixels), width * 3);
    if (!surface){
        EMU_LOG_WARN("Failed to create surface");
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(mRenderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        EMU_LOG_WARN("Failed to create texture");
        return nullptr;
    }

    if (autoFree) {
        mTextureFreeList.push_back(texture);
    }

    return texture;
}

void UserInterface::DestroyRGBTexture(SDL_Texture* texture)
{
    SDL_DestroyTexture(texture);
}

void UserInterface::Draw()
{
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();

    MainMenuBar();

    if (mShowDemo) {
        ImGui::ShowDemoWindow(&mShowDemo);
    }

    for (auto w : mWindows) {
        if (w->IsVisible()) {
            if (ImGui::Begin(w->GetName().c_str())) {
                w->Draw();
            }

            ImGui::End();
        }
    }

    ImGui::Render();
}

void UserInterface::MainMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                mIsRunning = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &mShowDemo);

            for (auto w : mWindows) {
                bool tmp = w->IsVisible();
                ImGui::MenuItem(w->GetName().c_str(), nullptr, &tmp);
                w->SetVisible(tmp);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

} // namespace emu
