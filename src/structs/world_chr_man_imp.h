#ifndef NIGHTREIGN_WORLD_CHR_MAN_IMP_H
#define NIGHTREIGN_WORLD_CHR_MAN_IMP_H

#include <cstdint>

namespace nightreign {

struct WorldChrManImp {
    char pad_0x0000[0x10EF8];  // Padding to player array (adjust if needed)
    // Add other fields as discovered
};

} // namespace nightreign

#endif