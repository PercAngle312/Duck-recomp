#include "../runtime/windows_rdram_alias.h"
#include <cstdio>
int main() {
    duck::win_rdram::Allocation a;
    if (!duck::win_rdram::create(a)) {
        std::fprintf(stderr, "mapper create failed %lu\n", GetLastError());
        if (a.base && !duck::win_rdram::destroy(a)) return 2;
        return 1;
    }
    volatile unsigned char* p = a.base;
    p[0] = 0x5A;
    if (p[duck::win_rdram::kAliasOffset] != 0x5A) return 3;
    p[duck::win_rdram::kAliasOffset + duck::win_rdram::kSize - 1] = 0xA5;
    if (p[duck::win_rdram::kSize - 1] != 0xA5) return 4;
    if (!duck::win_rdram::destroy(a)) {
        std::fprintf(stderr, "mapper destroy failed %lu\n", GetLastError());
        return 5;
    }
    if (a.base || a.backing) return 6;
    std::puts("PASS Phase 11 mapper on native Windows");
    return 0;
}
