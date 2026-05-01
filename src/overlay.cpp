#include "overlay.h"
#include <iostream>
#include <cstdio>
#include <dwmapi.h>
#include <algorithm>

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
float Overlay::uiScale_ = 1.0f;

ID3D11Device*           Overlay::device_ = nullptr;
ID3D11DeviceContext*    Overlay::context_ = nullptr;
IDXGISwapChain*         Overlay::swapChain_ = nullptr;
ID3D11RenderTargetView* Overlay::renderTarget_ = nullptr;

bool Overlay::CreateDeviceD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
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

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels, 1, D3D11_SDK_VERSION, &sd, &swapChain_, &device_, &featureLevel, &context_);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        device_->CreateRenderTargetView(backBuffer, nullptr, &renderTarget_);
        backBuffer->Release();
    }
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
    const char* titles[] = { "ELDEN RING: Nightreign", "ELDEN RING", "Nightreign", nullptr };
    for (int i = 0; titles[i]; i++) {
        gameWindow_ = FindWindowA(nullptr, titles[i]);
        if (gameWindow_) break;
    }
    if (!gameWindow_) gameWindow_ = GetForegroundWindow();

    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, "VisionOfNightreignOverlay", nullptr };
    RegisterClassExA(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    overlayWindow_ = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, "VisionOfNightreignOverlay", "Vision of Nightreign", WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!overlayWindow_) return false;

    SetLayeredWindowAttributes(overlayWindow_, RGB(0, 0, 0), 255, LWA_COLORKEY);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(overlayWindow_, &margins);

    ShowWindow(overlayWindow_, SW_SHOWNOACTIVATE);

    if (!CreateDeviceD3D(overlayWindow_)) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(overlayWindow_);
    ImGui_ImplDX11_Init(device_, context_);

    imguiInitialized_ = true;
    running_ = true;
    return true;
}

void Overlay::UpdateAndRender(const EnemyStatus& status)
{
    if (!running_ || !imguiInitialized_) return;

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

    ImGuiIO& io = ImGui::GetIO();
    float scale = (io.DisplaySize.y / 1080.0f) * uiScale_;
    if (scale < 0.1f) scale = 0.1f;

    if (status.valid || showSettings_) {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

        float windowWidth = 320.0f * scale;
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 0.0f), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, showSettings_ ? 0.7f : 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, showSettings_ ? ImVec4(1, 1, 1, 0.5f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * scale, 8.0f * scale));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar;
        if (!showSettings_) {
            flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | 
                     ImGuiWindowFlags_NoBackground;
        }

        if (ImGui::Begin("Vision of Nightreign ##Main", nullptr, flags)) {
            if (status.valid) {
                float hpRatio = (status.maxHp > 0) ? (float)status.hp / (float)status.maxHp : 0.0f;
                hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_HP);
                ImGui::ProgressBar(hpRatio, ImVec2(-1, 16.0f * scale), "");
                ImGui::PopStyleColor();

                ImVec2 barMin = ImGui::GetItemRectMin();
                ImVec2 barMax = ImGui::GetItemRectMax();
                char hpText[64];
                sprintf_s(hpText, "%d / %d", status.hp, status.maxHp);
                ImVec2 textSize = ImGui::CalcTextSize(hpText);
                ImGui::GetWindowDrawList()->AddText(ImVec2(barMin.x + (barMax.x - barMin.x - textSize.x) * 0.5f, barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f), IM_COL32(255, 255, 255, 255), hpText);

                if (status.maxStagger > 0.001f) {
                    float sRatio = std::clamp(status.stagger / status.maxStagger, 0.0f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_STAGGER);
                    ImGui::ProgressBar(sRatio, ImVec2(-1, 6.0f * scale), "");
                    ImGui::PopStyleColor();
                }

                bool anyEff = false;
                for (int i = 0; i < 7; i++) if (status.effects[i].current > 0) anyEff = true;
                if (anyEff) {
                    ImGui::Spacing();
                    const ImVec4 colors[7] = { COLOR_POISON, COLOR_ROT, COLOR_BLEED, COLOR_BLIGHT, COLOR_FROST, COLOR_SLEEP, COLOR_MADNESS };
                    for (int i = 0; i < 7; i++) {
                        if (status.effects[i].current > 0 && status.effects[i].max > 0) {
                            ImGui::TextColored(colors[i], "%s", status.effects[i].name);
                            
                            char valText[32];
                            sprintf_s(valText, "%d/%d", status.effects[i].current, status.effects[i].max);
                            float valTextWidth = ImGui::CalcTextSize(valText).x;
                            
                            ImGui::SameLine(windowWidth - valTextWidth - (20.0f * scale)); 
                            ImGui::TextColored(colors[i], "%s", valText);
                            
                            float eRatio = std::clamp((float)status.effects[i].current / (float)status.effects[i].max, 0.0f, 1.0f);
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colors[i]);
                            ImGui::ProgressBar(eRatio, ImVec2(-1, 5.0f * scale), "");
                            ImGui::PopStyleColor();
                        }
                    }
                }
            } else {
                ImGui::TextDisabled("Vision of Nightreign\n(Waiting for target...)");
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    if (showSettings_) {
        ImGui::SetNextWindowPos(ImVec2(20, io.DisplaySize.y - 180), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 0));
        if (ImGui::Begin("Overlay Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SliderFloat("Opacity", &alpha_, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("UI Scale", &uiScale_, 0.5f, 2.5f, "%.2f");
            
            if (ImGui::Button("Reset UI", ImVec2(-1, 0))) {
                alpha_ = 0.55f;
                uiScale_ = 1.0f;
                ImGui::SetWindowPos("Vision of Nightreign ##Main", ImVec2(50, 50));
            }
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Controls:");
            ImGui::TextDisabled("- Press ~ to Lock/Unlock UI");
            ImGui::TextDisabled("- When Unlocked: Drag top bar to move");
        }
        ImGui::End();
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
    UnregisterClassA("VisionOfNightreignOverlay", GetModuleHandle(nullptr));
}

} // namespace nightreign