#ifndef NIGHTREIGN_CHR_MODULE_BAG_H
#define NIGHTREIGN_CHR_MODULE_BAG_H

#include "chr_stat_module.h"
#include "chr_resist_module.h"
#include "chr_super_armor_module.h"

namespace nightreign {

struct ChrModuleBag {
    ChrStatModule* statModule;              // 0x0000
    char pad_0x0008[24];                    // 0x0008
    ChrResistModule* resistModule;          // 0x0020
    char pad_0x0028[24];                    // 0x0028
    ChrSuperArmorModule* superArmorModule;  // 0x0040
};

} // namespace nightreign

#endif