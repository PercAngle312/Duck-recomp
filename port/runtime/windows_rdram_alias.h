#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <memoryapi.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>

namespace duck::win_rdram {
inline constexpr SIZE_T kSize = 8ULL * 1024ULL * 1024ULL;
inline constexpr SIZE_T kAliasOffset = 0x20000000ULL;
inline constexpr SIZE_T kReserveSize = 4096ULL * 1024ULL * 1024ULL;
struct Allocation { uint8_t* base{}; HANDLE backing{}; bool first_mapped{}; bool second_mapped{}; int splits{3}; };
inline bool split(uint8_t* base, SIZE_T head) {
    return VirtualFree(base, head, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER) != 0;
}
inline bool release_placeholders(uint8_t* base, int& splits) {
    if (!base) return true;
    if (splits >= 3) {
        if (!VirtualFree(base + kAliasOffset, kReserveSize - kAliasOffset,
                         MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) return false;
        splits = 2;
    }
    if (splits >= 2) {
        if (!VirtualFree(base + kSize, kReserveSize - kSize,
                         MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) return false;
        splits = 1;
    }
    if (splits >= 1) {
        if (!VirtualFree(base, kReserveSize,
                         MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) return false;
        splits = 0;
    }
    return VirtualFree(base, 0, MEM_RELEASE) != 0;
}
inline bool destroy(Allocation& a);
inline bool create(Allocation& out) {
    if (out.base || out.backing) return false;
    auto* base = static_cast<uint8_t*>(VirtualAlloc2(nullptr, nullptr, kReserveSize,
        MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));
    if (!base) return false;
    out.base = base;
    out.splits = 0;
    if (!split(base, kSize)) goto fail;
    ++out.splits;
    if (!split(base + kSize, kAliasOffset - kSize)) goto fail;
    ++out.splits;
    if (!split(base + kAliasOffset, kSize)) goto fail;
    ++out.splits;
    out.backing = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                    static_cast<DWORD>(kSize), nullptr);
    if (!out.backing) goto fail;
    {
        auto* first = MapViewOfFile3(out.backing, GetCurrentProcess(), base, 0, kSize,
                                     MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
        if (first != base) {
            if (first && !UnmapViewOfFile2(GetCurrentProcess(), first, MEM_PRESERVE_PLACEHOLDER))
                std::fprintf(stderr, "[memory] unexpected view could not be unmapped\n");
            goto fail;
        }
        out.first_mapped = true;
        auto* second = MapViewOfFile3(out.backing, GetCurrentProcess(), base + kAliasOffset, 0, kSize,
                                      MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
        if (second != base + kAliasOffset) {
            if (second && !UnmapViewOfFile2(GetCurrentProcess(), second, MEM_PRESERVE_PLACEHOLDER))
                std::fprintf(stderr, "[memory] unexpected view could not be unmapped\n");
            goto fail;
        }
        out.second_mapped = true;
        return true;
    }
fail:
    std::fprintf(stderr, "[memory] Windows RDRAM mapping failed: %lu\n", GetLastError());
    if (!destroy(out))
        std::fprintf(stderr, "[memory] rollback incomplete; call destroy() again\n");
    return false;
}
inline bool destroy(Allocation& a) {
    if (!a.base) return !a.backing;
    if (a.first_mapped) {
        if (!UnmapViewOfFile2(GetCurrentProcess(), a.base, MEM_PRESERVE_PLACEHOLDER)) return false;
        a.first_mapped = false;
    }
    if (a.second_mapped) {
        if (!UnmapViewOfFile2(GetCurrentProcess(), a.base + kAliasOffset, MEM_PRESERVE_PLACEHOLDER)) return false;
        a.second_mapped = false;
    }
    if (!release_placeholders(a.base, a.splits)) return false;
    if (a.backing) CloseHandle(a.backing);
    a = {};
    return true;
}
}

#endif // _WIN32
