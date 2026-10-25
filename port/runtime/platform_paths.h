#pragma once
#include <filesystem>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace duck::platform {
inline std::filesystem::path executable_directory(std::error_code& ec) {
#ifdef _WIN32
    std::wstring path(512, L'\0');
    for (;;) {
        DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (!count) {
            ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
            return {};
        }
        if (count < path.size()) {
            path.resize(count);
            return std::filesystem::path(path).parent_path();
        }
        if (path.size() >= 32768) {
            ec = std::make_error_code(std::errc::filename_too_long);
            return {};
        }
        path.resize(path.size() * 2);
    }
#else
    auto executable = std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path{} : executable.parent_path();
#endif
}
inline std::filesystem::path storage_base(const char* override_dir, std::error_code& ec) {
    if (override_dir && *override_dir) return std::filesystem::absolute(override_dir, ec);
    auto dir = executable_directory(ec);
    return ec ? std::filesystem::path{} : dir / "private";
}
}
