#ifndef NIGHTREIGN_MEMORY_H
#define NIGHTREIGN_MEMORY_H

#include <Windows.h>
#include <string>
#include <cstdint>

namespace nightreign {

class Memory {
public:
    static uintptr_t PatternScan(const char* pattern, const char* mask, const char* moduleName = nullptr);
    
    static uintptr_t ResolveRIP(uintptr_t address, int instructionLength = 7);
    
    static uintptr_t GetModuleBase(const char* moduleName = nullptr);
    
    static void DumpMemory(uintptr_t address, size_t size);
};

} // namespace nightreign

#endif