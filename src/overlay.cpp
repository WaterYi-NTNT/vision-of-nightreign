#include "overlay.h"
#include <iostream>
#include <cstdio>
#include <dwmapi.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace nightreign {

HWND Overlay::overlayWindow_ = nullptr;
HWND Overlay::gameWindow_ = nullptr;
bool Overlay::running_ = false;
bool Overlay::imguiInitialized_ = false;
float Overlay::alpha_ = 0.55f;
bool Overlay::showSettings_ = false;

ID3D11Device*           Overlay::device_ = nullptr;
ID3D11DeviceContext*    Overlay::context_ = nullptr;
IDXGISwapChain*         Overlay::swapChain_ = nullptr;
ID3D11RenderTargetView* Overlay::renderTarget_ = nullptr;

bool Overlay::CreateDeviceD3D(HWND hwnd)
{
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = screenW;
    sd.BufferDesc.Height = screenH;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createFlags, featureLevels, 1,
        D3D11_SDK_VERSION, &sd,
        &swapChain_, &device_, &featureLevel, &context_
    );

    if (FAILED(hr)) {
        std::cerr << "[Overlay] D3D11CreateDeviceAndSwapChain failed: 0x"
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device_->CreateRenderTargetView(backBuffer, nullptr, &renderTarget_);
    backBuffer->Release();

    return true;
}

void Overlay::CleanupDeviceD3D()
{
    if (renderTarget_) { renderTarget_->Release(); renderTarget_ = nullptr; }
    if (swapChain_)    { swapChain_->Release();    swapChain_ = nullptr; }
    if (context_)      { context_->Release();      context_ = nullptr; }
    if (device_)       { device_->Release();       device_ = nullptr; }
}


LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Overlay::Initialize()
{
    std::cout << "[Overlay] Initializing..." << std::endl;

    const char* titles[] = {
        "ELDEN RING: Nightreign",
        "ELDEN RING",
        "Nightreign",
        "nightreign",
        nullptr
    };

    for (int i = 0; titles[i]; i++) {
        gameWindow_ = FindWindowA(nullptr, titles[i]);
        if (gameWindow_) {
            std::cout << "[Overlay] Found game window: " << titles[i] << std::endl;
            break;
        }
    }

    if (!gameWindow_) {
        gameWindow_ = GetForegroundWindow();
        std::cout << "[Overlay] Using foreground window as fallback" << std::endl;
    }

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "VisionOfNightreignOverlay";

    RegisterClassExA(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    overlayWindow_ = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "VisionOfNightreignOverlay",
        "Vision of Nightreign",
        WS_POPUP,
        0, 0, screenW, screenH,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (!overlayWindow_) {
        std::cerr << "[Overlay] CreateWindowEx failed!" << std::endl;
        return false;
    }

    SetLayeredWindowAttributes(overlayWindow_, RGB(0, 0, 0), 0, LWA_COLORKEY);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(overlayWindow_, &margins);

    ShowWindow(overlayWindow_, SW_SHOWNOACTIVATE);
    UpdateWindow(overlayWindow_);

    if (!CreateDeviceD3D(overlayWindow_)) {
        std::cerr << "[Overlay] DX11 device creation failed!" << std::endl;
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(overlayWindow_);
    ImGui_ImplDX11_Init(device_, context_);

    imguiInitialized_ = true;
    running_ = true;

    std::cout << "[Overlay] Initialized successfully!" << std::endl;
    return true;
}

void Overlay::UpdateAndRender(const EnemyStatus& status)
{
    if (!running_ || !imguiInitialized_)
        return;

    MSG msg;
    while (PeekMessage(&msg, overlayWindow_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (GetAsyncKeyState(VK_OEM_3) & 1) {
        showSettings_ = !showSettings_;
    }

    LONG_PTR exStyle = GetWindowLongPtr(overlayWindow_, GWL_EXSTYLE);
    if (showSettings_) {
        exStyle &= ~WS_EX_TRANSPARENT;
    } else {
        exStyle |= WS_EX_TRANSPARENT;
    }
    SetWindowLongPtr(overlayWindow_, GWL_EXSTYLE, exStyle);

    BYTE alphaValue = (BYTE)(alpha_ * 255.0f);
    SetLayeredWindowAttributes(overlayWindow_, RGB(0, 0, 0), alphaValue, LWA_COLORKEY | LWA_ALPHA);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(overlayWindow_, HWND_TOPMOST,
                 0, 0, screenW, screenH,
                 SWP_NOACTIVATE);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Render(status);

    ImGui::Render();

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->OMSetRenderTargets(1, &renderTarget_, nullptr);
    context_->ClearRenderTargetView(renderTarget_, clearColor);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    swapChain_->Present(1, 0);
}


void Overlay::Render(const EnemyStatus& status)
{
    static const ImVec4 COLOR_POISON   = ImVec4(0.45f, 0.65f, 0.15f, 1.0f);
    static const ImVec4 COLOR_ROT      = ImVec4(0.75f, 0.25f, 0.10f, 1.0f);
    static const ImVec4 COLOR_BLEED    = ImVec4(0.85f, 0.10f, 0.10f, 1.0f);
    static const ImVec4 COLOR_BLIGHT   = ImVec4(0.55f, 0.55f, 0.45f, 1.0f);
    static const ImVec4 COLOR_FROST    = ImVec4(0.40f, 0.70f, 0.90f, 1.0f);
    static const ImVec4 COLOR_SLEEP    = ImVec4(0.50f, 0.40f, 0.75f, 1.0f);
    static const ImVec4 COLOR_MADNESS  = ImVec4(0.95f, 0.70f, 0.10f, 1.0f);
    static const ImVec4 COLOR_HP       = ImVec4(0.75f, 0.15f, 0.10f, 1.0f);
    static const ImVec4 COLOR_STAGGER  = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    static const ImVec4 COLOR_BAR_BG   = ImVec4(0.12f, 0.12f, 0.12f, 0.80f);

    ImGuiIO& io = ImGui::GetIO();
    float scale = io.DisplaySize.y / 1080.0f;
    if (scale < 0.5f) scale = 1.0f;

    if (status.valid) {
        float winW = 280.0f * scale;
        float winX = 15.0f * scale;
        float winY = 190.0f * scale;

        ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(winW, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.65f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoInputs;

        if (ImGui::Begin("##BossOverlay", nullptr, flags)) {
            {
                float hpRatio = (status.maxHp > 0) ? (float)status.hp / (float)status.maxHp : 0.0f;
                if (hpRatio < 0.0f) hpRatio = 0.0f;
                if (hpRatio > 1.0f) hpRatio = 1.0f;

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_HP);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, COLOR_BAR_BG);
                ImGui::ProgressBar(hpRatio, ImVec2(-1, 10.0f * scale), "");
                ImGui::PopStyleColor(2);

                ImVec2 barMin = ImGui::GetItemRectMin();
                ImVec2 barMax = ImGui::GetItemRectMax();

                char hpText[64];
                sprintf_s(hpText, "%d / %d", status.hp, status.maxHp);

                ImVec2 textSize = ImGui::CalcTextSize(hpText);
                float textX = barMin.x + (barMax.x - barMin.x - textSize.x) * 0.5f;
                float textY = barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f;

                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(textX, textY),
                    IM_COL32(255, 255, 255, 220),
                    hpText
                );
            }

            if (status.maxStagger > 0.001f) {
                float ratio = status.stagger / status.maxStagger;
                if (ratio < 0.0f) ratio = 0.0f;
                if (ratio > 1.0f) ratio = 1.0f;

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_STAGGER);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, COLOR_BAR_BG);
                ImGui::ProgressBar(ratio, ImVec2(-1, 5.0f * scale), "");
                ImGui::PopStyleColor(2);
            }

            bool anyActive = false;
            for (int i = 0; i < 7; i++) {
                if (status.effects[i].current > 0 && status.effects[i].max > 0) {
                    anyActive = true;
                    break;
                }
            }

            if (anyActive) {
                ImGui::Spacing();

                const ImVec4 colors[7] = {
                    COLOR_POISON, COLOR_ROT, COLOR_BLEED, COLOR_BLIGHT,
                    COLOR_FROST, COLOR_SLEEP, COLOR_MADNESS
                };

                for (int i = 0; i < 7; i++) {
                    int cur = status.effects[i].current;
                    int max = status.effects[i].max;

                    if (cur > 0 && max > 0) {
                        ImGui::TextColored(colors[i], "%s", status.effects[i].name);
                        ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f * scale);
                        ImGui::TextColored(colors[i], "%d/%d", cur, max);

                        float ratio = (float)cur / (float)max;
                        if (ratio < 0.0f) ratio = 0.0f;
                        if (ratio > 1.0f) ratio = 1.0f;

                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colors[i]);
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, COLOR_BAR_BG);
                        ImGui::ProgressBar(ratio, ImVec2(-1, 4.0f * scale), "");
                        ImGui::PopStyleColor(2);
                    }
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor(2);
    }

    if (showSettings_) {
        ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 100), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(260, 0));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));

        ImGuiWindowFlags settingsFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("##Settings", nullptr, settingsFlags)) {
            ImGui::Text("~ - Toggle Settings");
            ImGui::Separator();
            ImGui::SliderFloat("Opacity", &alpha_, 0.3f, 1.0f, "%.2f");
        }
        ImGui::End();

        ImGui::PopStyleColor();
    }
}


void Overlay::Shutdown()
{
    if (imguiInitialized_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized_ = false;
    }

    CleanupDeviceD3D();

    if (overlayWindow_) {
        DestroyWindow(overlayWindow_);
        overlayWindow_ = nullptr;
    }

    UnregisterClassA("NightreignOverlay", GetModuleHandle(nullptr));
    running_ = false;
}

} // namespace nightreign