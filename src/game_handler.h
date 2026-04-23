#ifndef NIGHTREIGN_GAME_HANDLER_H
#define NIGHTREIGN_GAME_HANDLER_H

#include "structs/world_chr_man_imp.h"
#include "structs/chr_ins.h"
#include <cstdint>

namespace nightreign {

class GameHandler {
public:
    static bool Initialize();
    static void Shutdown();

    static WorldChrManImp* GetWorldChrMan();
    static ChrIns* GetPlayerChrIns();
    
    static ChrIns* GetChrInsFromHandle(uint64_t handle);
    
    static ChrIns* GetPlayerTarget();

private:
    static void FindPointers();

    static constexpr const char* WORLD_CHR_MAN_PATTERN = 
        "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x0F\x48\x39\x88";
    static constexpr const char* WORLD_CHR_MAN_MASK = "xxx????xxxxxxxx";
    
    static constexpr const char* GET_CHR_FROM_HANDLE_PATTERN = 
        "\x48\x89\x5C\x24\x18\x56\x48\x83\xEC\x20\x83\x3A\xFF";
    static constexpr const char* GET_CHR_FROM_HANDLE_MASK = "xxxxxxxxxxxxx";

    using GetChrInsFromHandleFunc = ChrIns*(*)(WorldChrManImp*, uint64_t*);

    static uintptr_t worldChrManPtr_;
    static uintptr_t getChrInsFromHandleAddr_;
    static GetChrInsFromHandleFunc getChrInsFromHandleFunc_;
};

} // namespace nightreign

#endif