#ifndef NIGHTREIGN_UI_RENDERER_H
#define NIGHTREIGN_UI_RENDERER_H

#include "structs/chr_ins.h"

namespace nightreign {

class UIRenderer {
public:
    static void Initialize();
    static void Render();
    static void Shutdown();

private:
    static void RenderEnemyStatus(ChrIns* enemy);
    static void DrawBar(const char* label, float current, float max, float width = 300.0f);
    
    static bool enabled_;
};

} // namespace nightreign

#endif