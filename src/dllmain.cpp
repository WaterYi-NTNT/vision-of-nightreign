#include <Windows.h>
#include "game_handler.h"
#include "overlay.h"
#include <thread>
#include <iostream>
#include <cstdint>
#include <algorithm>

using namespace nightreign;

#ifdef _DEBUG
    static constexpr bool DEBUG_CONSOLE = true;
#else
    static constexpr bool DEBUG_CONSOLE = false;
#endif

static constexpr uintptr_t CHRINS_HANDLE            = 0x8;
static constexpr uintptr_t CHRINS_STATE_MODULE      = 0x1B8;
static constexpr uintptr_t CHRINS_TARGET_HANDLE     = 0x768;

static constexpr uintptr_t STATE_MODULE_STAT_BASE   = 0x0;
static constexpr uintptr_t STATE_MODULE_RESIST      = 0x20;
static constexpr uintptr_t STATE_MODULE_SUPERARMOR  = 0x40;

static constexpr uintptr_t STAT_HP                  = 0x140;
static constexpr uintptr_t STAT_MAX_HP              = 0x144;

static constexpr uintptr_t SUPERARMOR_STAGGER       = 0x10;
static constexpr uintptr_t SUPERARMOR_MAX_STAGGER   = 0x14;

static const char* EFFECT_NAMES[] = {
    "Poison", "Rot", "Bleed", "Blight", "Frost", "Sleep", "Madness"
};

EnemyStatus ReadEnemyStatus(ChrIns* player)
{
    EnemyStatus status = {};
    status.valid = false;

    uintptr_t playerAddr = reinterpret_cast<uintptr_t>(player);

    uint64_t targetHandle = *reinterpret_cast<uint64_t*>(playerAddr + CHRINS_TARGET_HANDLE);
    if (targetHandle == 0 || targetHandle == 0xFFFFFFFFFFFFFFFF)
        return status;

    auto enemy = GameHandler::GetChrInsFromHandle(targetHandle);
    if (!enemy)
        return status;

    uintptr_t enemyRawAddr = reinterpret_cast<uintptr_t>(enemy);

    uintptr_t enemyChrIns = *reinterpret_cast<uintptr_t*>(enemyRawAddr);
    if (!enemyChrIns)
        return status;

    uint64_t enemyHandle = *reinterpret_cast<uint64_t*>(enemyChrIns + CHRINS_HANDLE);
    if (enemyHandle != targetHandle)
        return status;

    uintptr_t stateModulePtr = *reinterpret_cast<uintptr_t*>(enemyChrIns + CHRINS_STATE_MODULE);
    if (!stateModulePtr)
        return status;

    uintptr_t statBase = *reinterpret_cast<uintptr_t*>(stateModulePtr + STATE_MODULE_STAT_BASE);
    if (!statBase)
        return status;

    status.hp    = *reinterpret_cast<int32_t*>(statBase + STAT_HP);
    status.maxHp = *reinterpret_cast<int32_t*>(statBase + STAT_MAX_HP);

    uintptr_t superArmorModule = *reinterpret_cast<uintptr_t*>(stateModulePtr + STATE_MODULE_SUPERARMOR);
    if (superArmorModule) {
        status.stagger    = *reinterpret_cast<float*>(superArmorModule + SUPERARMOR_STAGGER);
        status.maxStagger = *reinterpret_cast<float*>(superArmorModule + SUPERARMOR_MAX_STAGGER);
    }

    uintptr_t resistModule = *reinterpret_cast<uintptr_t*>(stateModulePtr + STATE_MODULE_RESIST);
    if (resistModule) {
        for (int i = 0; i < 7; i++) {
            status.effects[i].name = EFFECT_NAMES[i];

            int32_t remaining = *reinterpret_cast<int32_t*>(resistModule + 0x10 + i * 4);
            int32_t max       = *reinterpret_cast<int32_t*>(resistModule + 0x2C + i * 4);

            if (max > 0) {
                int32_t buildup = max - remaining;
                buildup = std::clamp(buildup, 0, max);

                status.effects[i].current = buildup;
                status.effects[i].max     = max;
            }
            else {
                status.effects[i].current = 0;
                status.effects[i].max     = 0;
            }
        }
    }

    status.valid = true;
    return status;
}

DWORD WINAPI MainLoop(LPVOID)
{
    if (DEBUG_CONSOLE) {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        std::cout << "=== Vision of Nightreign ===" << std::endl;
        std::cout << "Version 1.0.0" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (!GameHandler::Initialize()) {
        if (DEBUG_CONSOLE) {
            std::cerr << "Failed to initialize GameHandler!" << std::endl;
        }
        return 0;
    }

    if (!Overlay::Initialize()) {
        if (DEBUG_CONSOLE) {
            std::cerr << "Failed to initialize Overlay!" << std::endl;
            std::cerr << "Continuing with console only..." << std::endl;
        }
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps

        auto player = GameHandler::GetPlayerChrIns();

        EnemyStatus status = {};
        if (player) {
            status = ReadEnemyStatus(player);
        }

        Overlay::UpdateAndRender(status);

        if (DEBUG_CONSOLE) {
            static int frameCount = 0;
            if (status.valid && (frameCount++ % 30 == 0)) {
                std::cout << "HP: " << status.hp << " / " << status.maxHp << std::endl;

                if (status.maxStagger > 0)
                    std::cout << "Stagger: " << status.stagger << " / " << status.maxStagger << std::endl;

                for (int i = 0; i < 7; i++) {
                    if (status.effects[i].max > 0) {
                        std::cout << EFFECT_NAMES[i] << ": "
                                  << status.effects[i].current << " / "
                                  << status.effects[i].max << std::endl;
                    }
                }
                std::cout << "---" << std::endl;
            }
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)lpReserved;

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0, MainLoop, nullptr, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
            Overlay::Shutdown();
            GameHandler::Shutdown();
            break;
    }
    return TRUE;
}