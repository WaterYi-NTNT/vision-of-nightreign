#include "overlay.h"
#include <iostream>
#include <cstdio>
#include <dwmapi.h>
#include <algorithm>
#include <filesystem>
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
float Overlay::lastAlpha_ = -1.0f;
bool Overlay::showSettings_ = false;
bool Overlay::lastShowSettings_ = false;
float Overlay::uiScale_ = 1.0f;
float Overlay::fontScale_ = 1.0f;
float Overlay::posX_ = 50.0f;
float Overlay::posY_ = 50.0f;
bool Overlay::alwaysShowStagger_ = false;
bool Overlay::alwaysShowEffects_ = false;
bool Overlay::showNumericalValues_ = true;
int Overlay::menuKeyVSC_ = VK_OEM_3;

ID3D11Device*           Overlay::device_ = nullptr;
ID3D11DeviceContext*    Overlay::context_ = nullptr;
IDXGISwapChain*         Overlay::swapChain_ = nullptr;
ID3D11RenderTargetView* Overlay::renderTarget_ = nullptr;

struct EnumData { DWORD pid; HWND hwnd; };
BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lp) {
    EnumData* data = (EnumData*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == data->pid && GetParent(hwnd) == NULL && IsWindowVisible(hwnd)) {
        data->hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

std::string Overlay::GetConfigPath() {
    char path[MAX_PATH];
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&Initialize, &hm);
    GetModuleFileNameA(hm, path, sizeof(path));
    std::filesystem::path p(path);
    return p.parent_path().string() + "\\VisionOfNightreign.ini";
}

void Overlay::LoadSettings() {
    std::string path = GetConfigPath();
    if (!std::filesystem::exists(path)) return;
    auto readInt = [&](const char* key, int def) { return GetPrivateProfileIntA("Settings", key, def, path.c_str()); };
    auto readFloat = [&](const char* key, const char* def) {
        char buf[64]; GetPrivateProfileStringA("Settings", key, def, buf, 64, path.c_str());
        try { return std::stof(buf); } catch (...) { return std::stof(def); }
    };
    alpha_ = readFloat("Opacity", "0.55");
    uiScale_ = readFloat("UIScale", "1.0");
    fontScale_ = readFloat("FontScale", "1.0");
    posX_ = readFloat("PosX", "50.0");
    posY_ = readFloat("PosY", "50.0");
    alwaysShowStagger_ = readInt("AlwaysShowStagger", 0) != 0;
    alwaysShowEffects_ = readInt("AlwaysShowEffects", 0) != 0;
    showNumericalValues_ = readInt("ShowNumericalValues", 1) != 0;
    menuKeyVSC_ = readInt("MenuKeyCode", VK_OEM_3);
}

void Overlay::SaveSettings() {
    std::string path = GetConfigPath();
    auto writeStr = [&](const char* key, std::string val) { WritePrivateProfileStringA("Settings", key, val.c_str(), path.c_str()); };
    writeStr("Opacity", std::to_string(alpha_));
    writeStr("UIScale", std::to_string(uiScale_));
    writeStr("FontScale", std::to_string(fontScale_));
    writeStr("PosX", std::to_string(posX_));
    writeStr("PosY", std::to_string(posY_));
    writeStr("AlwaysShowStagger", std::to_string(alwaysShowStagger_ ? 1 : 0));
    writeStr("AlwaysShowEffects", std::to_string(alwaysShowEffects_ ? 1 : 0));
    writeStr("ShowNumericalValues", std::to_string(showNumericalValues_ ? 1 : 0));
    writeStr("MenuKeyCode", std::to_string(menuKeyVSC_));
}

bool Overlay::CreateDeviceD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels, 1, D3D11_SDK_VERSION, &sd, &swapChain_, &device_, &featureLevel, &context_);
    if (FAILED(hr)) return false;
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) { device_->CreateRenderTargetView(backBuffer, nullptr, &renderTarget_); backBuffer->Release(); }
    return true;
}

void Overlay::CleanupDeviceD3D() {
    if (renderTarget_) renderTarget_->Release();
    if (swapChain_) swapChain_->Release();
    if (context_) context_->Release();
    if (device_) device_->Release();
}

LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Overlay::Initialize() {
    LoadSettings();
    EnumData ed = { GetCurrentProcessId(), NULL };
    EnumWindows(EnumProc, (LPARAM)&ed);
    gameWindow_ = ed.hwnd;
    
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, "VisionOfNightreignOverlay", nullptr };
    RegisterClassExA(&wc);
    overlayWindow_ = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, "VisionOfNightreignOverlay", "Vision of Nightreign", WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!overlayWindow_) return false;
    SetLayeredWindowAttributes(overlayWindow_, RGB(0, 0, 0), 255, LWA_COLORKEY);
    MARGINS margins = { -1 }; DwmExtendFrameIntoClientArea(overlayWindow_, &margins);
    ShowWindow(overlayWindow_, SW_SHOWNOACTIVATE);
    if (!CreateDeviceD3D(overlayWindow_)) return false;
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplWin32_Init(overlayWindow_); ImGui_ImplDX11_Init(device_, context_);
    imguiInitialized_ = true; running_ = true;
    return true;
}

void Overlay::UpdateAndRender(const EnemyStatus& status) {
    if (!running_ || !imguiInitialized_) return;
    if (!gameWindow_ || !IsWindow(gameWindow_)) {
        EnumData ed = { GetCurrentProcessId(), NULL };
        EnumWindows(EnumProc, (LPARAM)&ed);
        gameWindow_ = ed.hwnd;
    }

    HWND foreground = GetForegroundWindow();
    bool isAppActive = (foreground == gameWindow_ || foreground == overlayWindow_);
    bool isMinimized = gameWindow_ && IsIconic(gameWindow_);

    if (!isAppActive || isMinimized) { 
        ShowWindow(overlayWindow_, SW_HIDE); 
        Sleep(16); 
        return; 
    } else { 
        ShowWindow(overlayWindow_, SW_SHOWNOACTIVATE); 
        SetWindowPos(overlayWindow_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); 
    }
    
    MSG msg; while (PeekMessage(&msg, overlayWindow_, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    if ((GetAsyncKeyState(menuKeyVSC_) & 1) || (GetAsyncKeyState(VK_INSERT) & 1)) {
        showSettings_ = !showSettings_;
        if (!showSettings_) SaveSettings();
    }
    if (showSettings_ != lastShowSettings_) {
        LONG_PTR exStyle = GetWindowLongPtr(overlayWindow_, GWL_EXSTYLE);
        if (showSettings_) exStyle &= ~WS_EX_TRANSPARENT; else exStyle |= WS_EX_TRANSPARENT;
        SetWindowLongPtr(overlayWindow_, GWL_EXSTYLE, exStyle);
        lastShowSettings_ = showSettings_;
    }
    if (std::abs(alpha_ - lastAlpha_) > 0.001f) {
        SetLayeredWindowAttributes(overlayWindow_, RGB(0, 0, 0), (BYTE)(alpha_ * 255.0f), LWA_COLORKEY | LWA_ALPHA);
        lastAlpha_ = alpha_;
    }
    ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
    Render(status);
    ImGui::Render();
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->OMSetRenderTargets(1, &renderTarget_, nullptr);
    context_->ClearRenderTargetView(renderTarget_, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swapChain_->Present(0, 0);
}

void Overlay::Render(const EnemyStatus& status) {
    static const ImVec4 COLOR_POISON   = ImVec4(0.45f, 0.65f, 0.15f, 1.0f);
    static const ImVec4 COLOR_ROT      = ImVec4(0.75f, 0.25f, 0.10f, 1.0f);
    static const ImVec4 COLOR_BLEED    = ImVec4(0.85f, 0.10f, 0.10f, 1.0f);
    static const ImVec4 COLOR_BLIGHT   = ImVec4(0.55f, 0.55f, 0.45f, 1.0f);
    static const ImVec4 COLOR_FROST    = ImVec4(0.40f, 0.70f, 0.90f, 1.0f);
    static const ImVec4 COLOR_SLEEP    = ImVec4(0.50f, 0.40f, 0.75f, 1.0f);
    static const ImVec4 COLOR_MADNESS  = ImVec4(0.95f, 0.70f, 0.10f, 1.0f);
    static const ImVec4 COLOR_HP       = ImVec4(0.75f, 0.15f, 0.10f, 1.0f);
    static const ImVec4 COLOR_STAGGER  = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    const ImVec4 statusColors[7] = { COLOR_POISON, COLOR_ROT, COLOR_BLEED, COLOR_BLIGHT, COLOR_FROST, COLOR_SLEEP, COLOR_MADNESS };

    float baseScale = (ImGui::GetIO().DisplaySize.y / 1080.0f) * uiScale_;
    if (baseScale < 0.1f) baseScale = 0.1f;

    if (status.valid || showSettings_) {
        float windowWidth = 320.0f * baseScale;
        ImGui::SetNextWindowPos(ImVec2(posX_, posY_), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 0.0f), ImGuiCond_Always);
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, showSettings_ ? 0.7f : 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, showSettings_ ? ImVec4(1, 1, 1, 0.5f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * baseScale, 10.0f * baseScale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * baseScale, 4.0f * baseScale));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
        if (!showSettings_) flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;
        
        if (ImGui::Begin("Vision of Nightreign ##Main", nullptr, flags)) {
            ImGui::SetWindowFontScale(fontScale_);
            posX_ = ImGui::GetWindowPos().x;
            posY_ = ImGui::GetWindowPos().y;

            if (status.valid) {
                ImGui::TextColored(COLOR_HP, "HP");
                if (showNumericalValues_) {
                    char hpBuf[64]; sprintf_s(hpBuf, "%d / %d", status.hp, status.maxHp);
                    float valWidth = ImGui::CalcTextSize(hpBuf).x;
                    ImGui::SameLine(windowWidth - valWidth - (24.0f * baseScale));
                    ImGui::TextColored(COLOR_HP, "%s", hpBuf);
                }
                float hpRatio = std::clamp((float)status.hp / (status.maxHp > 0 ? status.maxHp : 1), 0.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_HP);
                ImGui::ProgressBar(hpRatio, ImVec2(-1, 12.0f * baseScale), ""); 
                ImGui::PopStyleColor();

                if (alwaysShowStagger_ || status.stagger > 0) {
                    ImGui::Spacing();
                    ImGui::TextColored(COLOR_STAGGER, "Poise");
                    if (showNumericalValues_) {
                        char sBuf[32]; sprintf_s(sBuf, "%.0f / %.0f", status.stagger, status.maxStagger);
                        float valWidth = ImGui::CalcTextSize(sBuf).x;
                        ImGui::SameLine(windowWidth - valWidth - (24.0f * baseScale));
                        ImGui::TextColored(COLOR_STAGGER, "%s", sBuf);
                    }
                    float sRatio = std::clamp(status.stagger / (status.maxStagger > 0 ? status.maxStagger : 1), 0.0f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_STAGGER);
                    ImGui::ProgressBar(sRatio, ImVec2(-1, 6.0f * baseScale), "");
                    ImGui::PopStyleColor();
                }

                bool anyEff = false;
                for (int i = 0; i < 7; i++) if (status.effects[i].current > 0) anyEff = true;

                if (anyEff || alwaysShowEffects_) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    for (int i = 0; i < 7; i++) {
                        if (alwaysShowEffects_ || status.effects[i].current > 0) {
                            ImVec4 eColor = statusColors[i];
                            
                            ImGui::TextColored(eColor, "%s", status.effects[i].name);
                            
                            if (showNumericalValues_) {
                                char eBuf[32]; sprintf_s(eBuf, "%d / %d", status.effects[i].current, status.effects[i].max);
                                float valWidth = ImGui::CalcTextSize(eBuf).x;
                                ImGui::SameLine(windowWidth - valWidth - (24.0f * baseScale));
                                ImGui::TextColored(eColor, "%s", eBuf);
                            }

                            float eRatio = std::clamp((float)status.effects[i].current / (status.effects[i].max > 0 ? status.effects[i].max : 1), 0.0f, 1.0f);
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, eColor);
                            ImGui::ProgressBar(eRatio, ImVec2(-1, 4.0f * baseScale), "");
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                        }
                    }
                }
            } else {
                ImGui::TextDisabled("Waiting for target...");
            }
        }
        ImGui::End(); ImGui::PopStyleVar(3); ImGui::PopStyleColor(2);
    }

    if (showSettings_) {
        ImGui::SetNextWindowPos(ImVec2(20, ImGui::GetIO().DisplaySize.y - 280), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(320, 0));
        if (ImGui::Begin("Overlay Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            ImGui::SliderFloat("Opacity", &alpha_, 0.1f, 1.0f);
            ImGui::SliderFloat("UI Scale", &uiScale_, 0.5f, 2.5f);
            ImGui::SliderFloat("Font Scale", &fontScale_, 0.5f, 2.0f);
            
            ImGui::Separator();
            ImGui::Checkbox("Always Show Poise", &alwaysShowStagger_);
            ImGui::Checkbox("Always Show Status", &alwaysShowEffects_);
            ImGui::Checkbox("Show Numerical Values", &showNumericalValues_);

            ImGui::Separator();
            if (ImGui::Button("Reload INI")) LoadSettings();
            ImGui::SameLine();
            if (ImGui::Button("Reset All")) { 
                alpha_ = 0.55f; uiScale_ = 1.0f; fontScale_ = 1.0f; 
                posX_ = 50.0f; posY_ = 50.0f; 
                alwaysShowStagger_ = false; alwaysShowEffects_ = false; showNumericalValues_ = true;
            }
            ImGui::TextDisabled("Toggle Menu: [~] or [Insert]");
        }
        ImGui::End();
    }
}

void Overlay::Shutdown() {
    SaveSettings();
    if (imguiInitialized_) { ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); }
    CleanupDeviceD3D();
    if (overlayWindow_) DestroyWindow(overlayWindow_);
}

}