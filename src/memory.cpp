#include "memory.h"
#include <psapi.h>
#include <vector>
#include <iostream>

namespace nightreign {

uintptr_t Memory::GetModuleBase(const char* moduleName) {
    HMODULE hModule = moduleName ? GetModuleHandleA(moduleName) : GetModuleHandle(nullptr);
    return reinterpret_cast<uintptr_t>(hModule);
}

uintptr_t Memory::PatternScan(const char* pattern, const char* mask, const char* moduleName) {
    HMODULE hModule = moduleName ? GetModuleHandleA(moduleName) : GetModuleHandle(nullptr);
    if (!hModule) return 0;

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO))) {
        return 0;
    }

    auto startAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size_t moduleSize = modInfo.SizeOfImage;
    size_t patternLength = strlen(mask);

    for (size_t i = 0; i < moduleSize - patternLength; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLength; ++j) {
            if (mask[j] != '?' && pattern[j] != *reinterpret_cast<char*>(startAddress + i + j)) {
                found = false;
                break;
            }
        }
        if (found) {
            return startAddress + i;
        }
    }

    return 0;
}

uintptr_t Memory::ResolveRIP(uintptr_t address, int instructionLength) {
    int32_t offset = *reinterpret_cast<int32_t*>(address + 3);
    return address + offset + instructionLength;
}

void Memory::DumpMemory(uintptr_t address, size_t size) {
    std::cout << "Memory dump at 0x" << std::hex << address << std::dec << ":\n";
    auto ptr = reinterpret_cast<unsigned char*>(address);
    for (size_t i = 0; i < size; ++i) {
        if (i % 16 == 0) std::cout << "\n  ";
        printf("%02X ", ptr[i]);
    }
    std::cout << "\n";
}

} // namespace nightreign