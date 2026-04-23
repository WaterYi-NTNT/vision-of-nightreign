#ifndef NIGHTREIGN_CHR_STAT_MODULE_H
#define NIGHTREIGN_CHR_STAT_MODULE_H

#include <cstdint>

namespace nightreign {

struct ChrStatModule {
    char pad_0x0000[0x140];  // 0x0000
    int32_t hp;              // 0x0140
    int32_t maxHp;           // 0x0144
    // Add other stats as needed
};

} // namespace nightreign

#endif