#ifndef NIGHTREIGN_CHR_INS_H
#define NIGHTREIGN_CHR_INS_H

#include <cstdint>
#include "chr_module_bag.h"

namespace nightreign {

struct ChrIns {
    char pad_0x0000[8];          // 0x0000
    uint64_t handle;             // 0x0008
    char pad_0x0010[368];        // 0x0010
    ChrModuleBag* moduleBag;     // 0x0190 (previously found as +0x1B8 but that was from different offset)
    char pad_0x0198[1496];       // 0x0198
    uint64_t targetHandle;       // 0x0768
};

} // namespace nightreign

#endif