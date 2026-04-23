#ifndef NIGHTREIGN_CHR_SUPER_ARMOR_MODULE_H
#define NIGHTREIGN_CHR_SUPER_ARMOR_MODULE_H

#include <cstdint>

namespace nightreign {

struct ChrSuperArmorModule {
    char pad_0x0000[0x10];   // 0x0000
    float stagger;           // 0x0010
    float maxStagger;        // 0x0014
};

} // namespace nightreign

#endif