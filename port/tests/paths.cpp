#include "../runtime/platform_paths.h"
#include <cstdio>
int main() {
    std::error_code ec;
    auto exe = duck::platform::executable_directory(ec);
    if (ec || exe.empty() || !exe.is_absolute()) return 1;
    auto custom = duck::platform::storage_base("duck-save-test", ec);
    if (ec || custom != std::filesystem::absolute("duck-save-test")) return 2;
    auto def = duck::platform::storage_base(nullptr, ec);
    if (ec || def != exe / "private") return 3;
    auto empty = duck::platform::storage_base("", ec);
    if (ec || empty != def) return 4;
    std::puts("PASS portable file paths");
    return 0;
}
