#include "ui_renderer.h"
#include "game_handler.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace nightreign {

bool UIRenderer::enabled_ = true;

void UIRenderer::Initialize() {
}

void UIRenderer::Render() {
    if (!enabled_) return;
    
    auto enemy = GameHandler::GetPlayerTarget();
    if (!enemy) return;
    
    RenderEnemyStatus(enemy);
}

void UIRenderer::RenderEnemyStatus(ChrIns* enemy) {
    if (!enemy || !enemy->moduleBag) return;
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Enemy Status", &enabled_, ImGuiWindowFlags_NoCollapse)) {
        auto moduleBag = enemy->moduleBag;
        
        if (moduleBag->statModule) {
            float hp = static_cast<float>(moduleBag->statModule->hp);
            float maxHp = static_cast<float>(moduleBag->statModule->maxHp);
            DrawBar("HP", hp, maxHp);
        }
        
        if (moduleBag->superArmorModule) {
            float stagger = moduleBag->superArmorModule->stagger;
            float maxStagger = moduleBag->superArmorModule->maxStagger;
            DrawBar("Stagger", stagger, maxStagger);
        }
        
        if (moduleBag->resistModule) {
            ImGui::Separator();
            ImGui::Text("Status Effects:");
            
            for (int i = 0; i < 7; ++i) {
                float buildup = moduleBag->resistModule->GetBuildup(i);
                float max = moduleBag->resistModule->GetMaxResist(i);
                
                if (max > 0.1f) {
                    char label[32];
                    snprintf(label, sizeof(label), "Effect %d", i);
                    DrawBar(label, buildup, max, 250.0f);
                }
            }
        }
    }
    ImGui::End();
}

void UIRenderer::DrawBar(const char* label, float current, float max, float width) {
    if (max <= 0) return;
    
    float percent = std::min(current / max, 1.0f);
    
    ImGui::Text("%s: %.0f / %.0f (%.1f%%)", label, current, max, percent * 100.0f);
    
    ImVec4 color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    if (percent < 0.3f) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    } else if (percent < 0.6f) {
        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
    }
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(percent, ImVec2(width, 20.0f));
    ImGui::PopStyleColor();
}

void UIRenderer::Shutdown() {
}

} // namespace nightreign