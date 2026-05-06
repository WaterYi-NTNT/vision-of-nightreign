#ifndef NIGHTREIGN_OVERLAY_H
#define NIGHTREIGN_OVERLAY_H

#include <Windows.h>
#include <d3d11.h>
#include <cstdint>

namespace nightreign {

struct StatusEffect {
    const char* name = "";
    int32_t current = 0;
    int32_t max = 0;
};

struct EnemyStatus {
    bool valid = false;
    int32_t hp = 0;
    int32_t maxHp = 0;
    float stagger = 0.0f;
    float maxStagger = 0.0f;
    StatusEffect effects[7];
};

class Overlay {
public:
    static bool Initialize();
    static void UpdateAndRender(const EnemyStatus& status);
    static void Shutdown();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static bool CreateDeviceD3D(HWND hwnd);
    static void CleanupDeviceD3D();
    static void Render(const EnemyStatus& status);

    static HWND overlayWindow_;
    static HWND gameWindow_;
    static bool running_;
    static bool imguiInitialized_;

    static float alpha_;
    static float lastAlpha_;
    static bool showSettings_;
    static bool lastShowSettings_;
    static float uiScale_;

    static ID3D11Device*           device_;
    static ID3D11DeviceContext*    context_;
    static IDXGISwapChain*         swapChain_;
    static ID3D11RenderTargetView* renderTarget_;
};

}

#endif