#include "overlay.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace nightreign {

static void Log(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm parts;
    localtime_s(&parts, &now_c);

    std::ofstream logFile("E:\\Code\\Mods\\Vision Of Nightreign\\build\\bin\\Release\\Nightreign_Hook.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[" << std::put_time(&parts, "%H:%M:%S") << "] [DX12 Hook] " << buf << "\n";
    }
}

static float g_Alpha = 0.55f;
static bool  g_ShowSettings = false;
static float g_UIScale = 1.0f;
static float g_FontScale = 1.0f;
static float g_PosX = 50.0f;
static float g_PosY = 50.0f;
static bool  g_AlwaysShowStagger = false;
static bool  g_AlwaysShowEffects = false;
static bool  g_ShowNumericalValues = true;
static int   g_MenuKeyVSC = VK_OEM_3;

static std::string GetConfigPath() {
    char path[MAX_PATH];
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetConfigPath, &hm);
    GetModuleFileNameA(hm, path, sizeof(path));
    std::filesystem::path p(path);
    return p.parent_path().string() + "\\VisionOfNightreign.ini";
}

static void LoadSettings() {
    std::string path = GetConfigPath();
    if (!std::filesystem::exists(path)) return;
    auto readInt = [&](const char* key, int def) { return GetPrivateProfileIntA("Settings", key, def, path.c_str()); };
    auto readFloat = [&](const char* key, const char* def) {
        char buf[64]; GetPrivateProfileStringA("Settings", key, def, buf, 64, path.c_str());
        try { return std::stof(buf); } catch (...) { return std::stof(def); }
    };
    g_Alpha = readFloat("Opacity", "0.55");
    g_UIScale = readFloat("UIScale", "1.0");
    g_FontScale = readFloat("FontScale", "1.0");
    g_PosX = readFloat("PosX", "50.0");
    g_PosY = readFloat("PosY", "50.0");
    g_AlwaysShowStagger = readInt("AlwaysShowStagger", 0) != 0;
    g_AlwaysShowEffects = readInt("AlwaysShowEffects", 0) != 0;
    g_ShowNumericalValues = readInt("ShowNumericalValues", 1) != 0;
    g_MenuKeyVSC = readInt("MenuKeyCode", VK_OEM_3);
}

static void SaveSettings() {
    std::string path = GetConfigPath();
    auto writeStr = [&](const char* key, std::string val) { WritePrivateProfileStringA("Settings", key, val.c_str(), path.c_str()); };
    writeStr("Opacity", std::to_string(g_Alpha));
    writeStr("UIScale", std::to_string(g_UIScale));
    writeStr("FontScale", std::to_string(g_FontScale));
    writeStr("PosX", std::to_string(g_PosX));
    writeStr("PosY", std::to_string(g_PosY));
    writeStr("AlwaysShowStagger", std::to_string(g_AlwaysShowStagger ? 1 : 0));
    writeStr("AlwaysShowEffects", std::to_string(g_AlwaysShowEffects ? 1 : 0));
    writeStr("ShowNumericalValues", std::to_string(g_ShowNumericalValues ? 1 : 0));
    writeStr("MenuKeyCode", std::to_string(g_MenuKeyVSC));
}

static ID3D12CommandQueue*          g_LastCommandQueue = nullptr;
static ID3D12CommandQueue*          g_CommandQueue = nullptr;
static IDXGISwapChain3*             g_SwapChain = nullptr;
static ID3D12Device*                g_Device = nullptr;
static ID3D12DescriptorHeap*        g_RTVHeap = nullptr;
static ID3D12DescriptorHeap*        g_SRVHeap = nullptr;
static ID3D12CommandAllocator*      g_CommandAllocators[8] = { nullptr };
static ID3D12GraphicsCommandList*   g_CommandList = nullptr;
static ID3D12Resource*              g_RenderTargets[8] = { nullptr };

static ID3D12Fence*                 g_Fence = nullptr;
static HANDLE                       g_FenceEvent = nullptr;
static UINT64                       g_FenceValue = 0;
static UINT64                       g_FrameFenceValues[8] = { 0 };

static UINT                         g_FrameCount = 0;
static bool                         g_ImGuiInitialized = false;
static EnemyStatus                  g_LatestStatus{};

static HWND                         g_GameHwnd = nullptr;
static WNDPROC                      oWndProc = nullptr;

typedef void(__stdcall* ExecuteFn)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);

static ExecuteFn oExecute = nullptr;
static PresentFn oPresent = nullptr;

LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_ShowSettings) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_MOUSEWHEEL) {
            return true; 
        }
    }
    return CallWindowProc(oWndProc, hwnd, msg, wParam, lParam);
}

void __stdcall hkExecute(ID3D12CommandQueue* queue, UINT numLists, ID3D12CommandList* const* lists)
{
    if (queue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
        g_LastCommandQueue = queue;
    }
    oExecute(queue, numLists, lists);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT sync, UINT flags)
{
    if (!g_ImGuiInitialized && g_LastCommandQueue)
    {
        g_CommandQueue = g_LastCommandQueue;
        Log("Game called Present. Starting ImGui Init...");

        if (FAILED(swapChain->QueryInterface(IID_PPV_ARGS(&g_SwapChain)))) {
            return oPresent(swapChain, sync, flags);
        }

        g_SwapChain->GetDevice(IID_PPV_ARGS(&g_Device));

        DXGI_SWAP_CHAIN_DESC desc{};
        g_SwapChain->GetDesc(&desc);
        g_FrameCount = desc.BufferCount;
        if (g_FrameCount > 8) g_FrameCount = 8;
        
        g_GameHwnd = desc.OutputWindow;

        if (g_GameHwnd) {
            oWndProc = (WNDPROC)SetWindowLongPtr(g_GameHwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
        }

        g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence));
        g_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 1;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        g_Device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_SRVHeap));

        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = g_FrameCount;
        rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        g_Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_RTVHeap));

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        UINT rtvSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        DXGI_FORMAT safeFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_RENDER_TARGET_VIEW_DESC safeRTVDesc{};
        safeRTVDesc.Format = safeFormat;
        safeRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        for (UINT i = 0; i < g_FrameCount; i++)
        {
            g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&g_RenderTargets[i]));
            g_Device->CreateRenderTargetView(g_RenderTargets[i], &safeRTVDesc, rtvHandle);
            rtvHandle.ptr += rtvSize;
            g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocators[i]));
        }

        g_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_CommandAllocators[0], nullptr, IID_PPV_ARGS(&g_CommandList));
        g_CommandList->Close();

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        
        io.Fonts->Build();

        ImGui_ImplWin32_Init(g_GameHwnd);
        ImGui_ImplDX12_Init(g_Device, g_FrameCount, safeFormat, g_SRVHeap,
            g_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
            g_SRVHeap->GetGPUDescriptorHandleForHeapStart());

        g_ImGuiInitialized = true;
        Log("ImGui Init SUCCESS! UI Restored.");
    }

    if (g_ImGuiInitialized && g_CommandQueue)
    {
        UINT frameIndex = g_SwapChain->GetCurrentBackBufferIndex();

        if (g_Fence->GetCompletedValue() < g_FrameFenceValues[frameIndex]) {
            g_Fence->SetEventOnCompletion(g_FrameFenceValues[frameIndex], g_FenceEvent);
            WaitForSingleObject(g_FenceEvent, INFINITE);
        }

        g_CommandAllocators[frameIndex]->Reset();
        g_CommandList->Reset(g_CommandAllocators[frameIndex], nullptr);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_RenderTargets[frameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        g_CommandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += frameIndex * g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        g_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        ID3D12DescriptorHeap* heaps[] = { g_SRVHeap };
        g_CommandList->SetDescriptorHeaps(1, heaps);

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Overlay::Render(g_LatestStatus);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_CommandList);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_CommandList->ResourceBarrier(1, &barrier);

        g_CommandList->Close();

        ID3D12CommandList* cmdLists[] = { g_CommandList };
        g_CommandQueue->ExecuteCommandLists(1, cmdLists);

        g_FenceValue++;
        g_CommandQueue->Signal(g_Fence, g_FenceValue);
        g_FrameFenceValues[frameIndex] = g_FenceValue;
    }

    return oPresent(swapChain, sync, flags);
}

bool Overlay::Initialize()
{
    LoadSettings();

    std::ofstream logFile("E:\\Code\\Mods\\Vision Of Nightreign\\build\\bin\\Release\\Nightreign_Hook.log", std::ios::trunc);
    logFile.close();
    Log("--- Overlay Hook Starting (Dummy Window Method) ---");

    if (MH_Initialize() != MH_OK) return false;

    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX12DummyWindow", NULL };
    RegisterClassExA(&wc);
    HWND dummyHwnd = CreateWindowA("DX12DummyWindow", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

    IDXGIFactory4* factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;

    IDXGIAdapter1* adapter;
    factory->EnumAdapters1(0, &adapter);

    ID3D12Device* device;
    if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) return false;

    D3D12_COMMAND_QUEUE_DESC qdesc{};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12CommandQueue* queue;
    device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&queue));

    DXGI_SWAP_CHAIN_DESC1 scdesc{};
    scdesc.BufferCount = 2;
    scdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scdesc.SampleDesc.Count = 1;

    IDXGISwapChain1* swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(queue, dummyHwnd, &scdesc, nullptr, nullptr, &swapChain1))) return false;

    void** vtableQueue = *reinterpret_cast<void***>(queue);
    void** vtableSwap  = *reinterpret_cast<void***>(swapChain1);

    if (MH_CreateHook(vtableQueue[10], hkExecute, reinterpret_cast<void**>(&oExecute)) == MH_OK) {
        MH_EnableHook(vtableQueue[10]);
    }
    if (MH_CreateHook(vtableSwap[8], hkPresent, reinterpret_cast<void**>(&oPresent)) == MH_OK) {
        MH_EnableHook(vtableSwap[8]);
    }

    swapChain1->Release();
    queue->Release();
    device->Release();
    adapter->Release();
    factory->Release();
    DestroyWindow(dummyHwnd);
    UnregisterClassA("DX12DummyWindow", wc.hInstance);

    return true;
}

void Overlay::UpdateAndRender(const EnemyStatus& status)
{
    g_LatestStatus = status;
}

void Overlay::Render(const EnemyStatus& status)
{
    static bool wasKeyPressed = false;
    bool isKeyPressed = (GetAsyncKeyState(g_MenuKeyVSC) & 0x8000) || (GetAsyncKeyState(VK_INSERT) & 0x8000);
    if (isKeyPressed && !wasKeyPressed) {
        g_ShowSettings = !g_ShowSettings;
        if (!g_ShowSettings) SaveSettings();
    }
    wasKeyPressed = isKeyPressed;

    ImGui::GetStyle().Alpha = g_Alpha;

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

    float baseScale = (ImGui::GetIO().DisplaySize.y / 1080.0f) * g_UIScale;
    if (baseScale < 0.1f) baseScale = 0.1f;

    if (status.valid || g_ShowSettings) {
        float windowWidth = 320.0f * baseScale;
        
        ImGuiCond posCond = g_ShowSettings ? ImGuiCond_Appearing : ImGuiCond_Always;
        ImGui::SetNextWindowPos(ImVec2(g_PosX, g_PosY), posCond);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 0.0f), ImGuiCond_Always);
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, g_ShowSettings ? 0.7f : 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, g_ShowSettings ? ImVec4(1, 1, 1, 0.5f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * baseScale, 10.0f * baseScale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * baseScale, 4.0f * baseScale));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
        if (!g_ShowSettings) {
            flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;
        }
        
        if (ImGui::Begin("Vision of Nightreign ##Main", nullptr, flags)) {
            ImGui::SetWindowFontScale(g_FontScale);
            
            g_PosX = ImGui::GetWindowPos().x;
            g_PosY = ImGui::GetWindowPos().y;

            if (status.valid) {
                ImGui::TextColored(COLOR_HP, "HP");
                if (g_ShowNumericalValues) {
                    char hpBuf[64]; sprintf_s(hpBuf, "%d / %d", status.hp, status.maxHp);
                    float valWidth = ImGui::CalcTextSize(hpBuf).x;
                    ImGui::SameLine(windowWidth - valWidth - (24.0f * baseScale));
                    ImGui::TextColored(COLOR_HP, "%s", hpBuf);
                }
                float hpRatio = std::clamp((float)status.hp / (status.maxHp > 0 ? status.maxHp : 1), 0.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_HP);
                ImGui::ProgressBar(hpRatio, ImVec2(-1, 12.0f * baseScale), ""); 
                ImGui::PopStyleColor();

                if (g_AlwaysShowStagger || status.stagger > 0) {
                    ImGui::Spacing();
                    ImGui::TextColored(COLOR_STAGGER, "Poise");
                    if (g_ShowNumericalValues) {
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

                if (anyEff || g_AlwaysShowEffects) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    for (int i = 0; i < 7; i++) {
                        if (g_AlwaysShowEffects || status.effects[i].current > 0) {
                            ImVec4 eColor = statusColors[i];
                            
                            ImGui::TextColored(eColor, "%s", status.effects[i].name);
                            
                            if (g_ShowNumericalValues) {
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
        ImGui::End(); 
        ImGui::PopStyleVar(3); 
        ImGui::PopStyleColor(2);
    }

    if (g_ShowSettings) {
        ImGui::SetNextWindowPos(ImVec2(20, ImGui::GetIO().DisplaySize.y - 280), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(320, 0));
        if (ImGui::Begin("Overlay Settings", &g_ShowSettings, ImGuiWindowFlags_NoResize)) {
            ImGui::SliderFloat("Opacity", &g_Alpha, 0.1f, 1.0f);
            ImGui::SliderFloat("UI Scale", &g_UIScale, 0.5f, 2.5f);
            ImGui::SliderFloat("Font Scale", &g_FontScale, 0.5f, 2.0f);
            
            ImGui::Separator();
            ImGui::Checkbox("Always Show Poise", &g_AlwaysShowStagger);
            ImGui::Checkbox("Always Show Status", &g_AlwaysShowEffects);
            ImGui::Checkbox("Show Numerical Values", &g_ShowNumericalValues);

            ImGui::Separator();
            if (ImGui::Button("Reload INI")) LoadSettings();
            ImGui::SameLine();
            if (ImGui::Button("Reset All")) { 
                g_Alpha = 0.55f; g_UIScale = 1.0f; g_FontScale = 1.0f; 
                g_PosX = 50.0f; g_PosY = 50.0f; 
                g_AlwaysShowStagger = false; g_AlwaysShowEffects = false; g_ShowNumericalValues = true;
            }
            ImGui::TextDisabled("Toggle Menu: [~] or [Insert]");
        }
        ImGui::End();
    }
}

void Overlay::Shutdown()
{
    SaveSettings();
    
    if (oWndProc && g_GameHwnd) {
        SetWindowLongPtr(g_GameHwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    }

    if (g_ImGuiInitialized) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (g_FenceEvent) {
            CloseHandle(g_FenceEvent);
            g_FenceEvent = nullptr;
        }
        if (g_Fence) {
            g_Fence->Release();
            g_Fence = nullptr;
        }
    }
    MH_Uninitialize();
}

}