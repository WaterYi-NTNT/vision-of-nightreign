#ifndef NIGHTREIGN_CHR_RESIST_MODULE_H
#define NIGHTREIGN_CHR_RESIST_MODULE_H

#include <cstdint>

namespace nightreign {

struct ChrResistModule {
    char pad_0x0000[0x10];     // 0x0000
    float currentResist[10];   // 0x0010 - array of current resist values
    float maxResist[10];       // 0x001C + offsets - array of max resist values

    float GetCurrentResist(int index) const {
        if (index < 0 || index >= 10) return 0.0f;
        return currentResist[index];
    }
    
    float GetMaxResist(int index) const {
        if (index < 0 || index >= 10) return 0.0f;
        return *reinterpret_cast<const float*>(
            reinterpret_cast<const char*>(&currentResist[index]) + 0x0C
        );
    }
    
    float GetBuildup(int index) const {
        float max = GetMaxResist(index);
        float current = GetCurrentResist(index);
        return max > 0 ? max - current : 0.0f;
    }
    
    float GetPercent(int index) const {
        float max = GetMaxResist(index);
        return max > 0 ? (GetBuildup(index) / max) * 100.0f : 0.0f;
    }
};

} // namespace nightreign

#endif