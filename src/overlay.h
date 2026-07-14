#ifndef NIGHTREIGN_OVERLAY_H
#define NIGHTREIGN_OVERLAY_H

#include <Windows.h>
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

    static void Render(const EnemyStatus& status);

private:
    Overlay() = delete;
};

}

#endif