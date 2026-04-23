#include "game_handler.h"
#include "memory.h"
#include <iostream>

namespace nightreign {

uintptr_t GameHandler::worldChrManPtr_ = 0;
uintptr_t GameHandler::getChrInsFromHandleAddr_ = 0;
GameHandler::GetChrInsFromHandleFunc GameHandler::getChrInsFromHandleFunc_ = nullptr;

bool GameHandler::Initialize() {
    std::cout << "=== Nightreign Status Mod Initializing ===" << std::endl;
    
    FindPointers();
    
    if (!worldChrManPtr_) {
        std::cerr << "Failed to find WorldChrMan!" << std::endl;
        return false;
    }
    
    if (!getChrInsFromHandleAddr_) {
        std::cerr << "Failed to find GetChrInsFromHandle!" << std::endl;
        return false;
    }
    
    getChrInsFromHandleFunc_ = reinterpret_cast<GetChrInsFromHandleFunc>(getChrInsFromHandleAddr_);
    
    std::cout << "Initialization complete!" << std::endl;
    std::cout << "  WorldChrMan: 0x" << std::hex << worldChrManPtr_ << std::dec << std::endl;
    std::cout << "  GetChrInsFromHandle: 0x" << std::hex << getChrInsFromHandleAddr_ << std::dec << std::endl;
    
    return true;
}

void GameHandler::FindPointers()
{
    uintptr_t base = Memory::GetModuleBase();
    
    worldChrManPtr_ = base + 0x3C0F0A8; 

    std::cout << "worldchrman address set to: 0x" << std::hex << worldChrManPtr_ << std::dec << std::endl;

    getChrInsFromHandleAddr_ = Memory::PatternScan(
        GET_CHR_FROM_HANDLE_PATTERN,
        GET_CHR_FROM_HANDLE_MASK
    );

    if (getChrInsFromHandleAddr_) {
        std::cout << "Found GetChrInsFromHandle at: 0x" << std::hex << getChrInsFromHandleAddr_ << std::dec << std::endl;
    }
}

WorldChrManImp* GameHandler::GetWorldChrMan()
{
    if (!worldChrManPtr_)
        return nullptr;

    return *reinterpret_cast<WorldChrManImp**>(worldChrManPtr_);
}

ChrIns* GameHandler::GetPlayerChrIns()
{
    auto world = GetWorldChrMan();
    if (!world)
        return nullptr;

    return *reinterpret_cast<ChrIns**>(
        reinterpret_cast<uintptr_t>(world) + 0x174E8
    );
}

ChrIns* GameHandler::GetChrInsFromHandle(uint64_t handle) {
    if (!getChrInsFromHandleFunc_ || handle == 0 || handle == 0xFFFFFFFFFFFFFFFF) {
        return nullptr;
    }
    
    auto worldChrMan = GetWorldChrMan();
    if (!worldChrMan) return nullptr;
    
    return getChrInsFromHandleFunc_(worldChrMan, &handle);
}

ChrIns* GameHandler::GetPlayerTarget() {
    auto player = GetPlayerChrIns();
    if (!player) return nullptr;
    
    uint64_t targetHandle = *reinterpret_cast<uint64_t*>(
        reinterpret_cast<uintptr_t>(player) + 0x768
    );
    if (targetHandle == 0 || targetHandle == 0xFFFFFFFFFFFFFFFF) {
        return nullptr;
    }
    
    return GetChrInsFromHandle(targetHandle);
}

void GameHandler::Shutdown() {
    std::cout << "GameHandler shutdown." << std::endl;
}

} // namespace nightreign