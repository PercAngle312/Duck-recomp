#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <memoryapi.h>
#include <cstdint>
#include <cstdio>
int main() {
    constexpr SIZE_T size = 8ull * 1024 * 1024;
    constexpr SIZE_T offset = 0x20000000ull;
    constexpr SIZE_T reserve = 0x100000000ull;
    auto* base = static_cast<std::uint8_t*>(VirtualAlloc2(nullptr, nullptr, reserve,
        MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));
    if (!base) { std::printf("reserve failed: %lu\n", GetLastError()); return 1; }
    if (!VirtualFree(base, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER) ||
        !VirtualFree(base + size, offset - size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER) ||
        !VirtualFree(base + offset, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
        std::printf("split failed: %lu\n", GetLastError()); return 2;
    }
    HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        static_cast<DWORD>(size), nullptr);
    if (!section) { std::printf("section failed: %lu\n", GetLastError()); return 3; }
    void* a = MapViewOfFile3(section, GetCurrentProcess(), base, 0, size,
        MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
    void* b = MapViewOfFile3(section, GetCurrentProcess(), base + offset, 0, size,
        MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
    if (a != base || b != base + offset) {
        std::printf("map failed: %lu\n", GetLastError()); return 4;
    }
    const SIZE_T offsets[] = {0, 4095, 4096, size - 1};
    for (SIZE_T i : offsets) {
        base[i] = 0xA7;
        if (base[offset + i] != 0xA7) return 5;
        base[offset + i] = 0x39;
        if (base[i] != 0x39) return 6;
    }
    if (!UnmapViewOfFile2(GetCurrentProcess(), b, MEM_PRESERVE_PLACEHOLDER) ||
        !UnmapViewOfFile2(GetCurrentProcess(), a, MEM_PRESERVE_PLACEHOLDER)) return 7;
    CloseHandle(section);
    if (!VirtualFree(base + offset, reserve - offset, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS) ||
        !VirtualFree(base + size, reserve - size, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS) ||
        !VirtualFree(base, reserve, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS) ||
        !VirtualFree(base, 0, MEM_RELEASE)) return 8;
    std::puts("PASS: dual RDRAM aliases coherent and released");
    return 0;
}
