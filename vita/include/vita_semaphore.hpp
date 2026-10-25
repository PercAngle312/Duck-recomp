// Duck Recomp Vita platform adapter. Does not replace the upstream queue.
#pragma once
#include <psp2/kernel/threadmgr/semaphore.h>
#include <psp2/kernel/error.h>
#include <cstdint>
#include <cstdlib>
#include <climits>

namespace duck_vita {
class Semaphore {
    SceUID id_;
    static bool success_or_timeout(int result) {
        if (result >= 0) return true;
        if (result == static_cast<int>(SCE_KERNEL_ERROR_WAIT_TIMEOUT)) return false;
        // A deleted/invalid semaphore must not become an endless queue retry.
        std::abort();
    }
public:
    explicit Semaphore(int initial = 0) {
        if (initial < 0) std::abort();
        id_ = sceKernelCreateSema("DuckQueue", 0, initial, INT_MAX, nullptr);
        if (id_ < 0) std::abort();
    }
    ~Semaphore() { if (sceKernelDeleteSema(id_) < 0) std::abort(); }
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    bool wait() {
        return success_or_timeout(sceKernelWaitSema(id_, 1, nullptr));
    }
    bool try_wait() {
        int result = sceKernelPollSema(id_, 1);
        if (result == static_cast<int>(SCE_KERNEL_ERROR_SEMA_ZERO)) return false;
        return success_or_timeout(result);
    }
    bool timed_wait(std::uint64_t usecs) {
        if (!usecs) return try_wait();
        // The queue accepts 64-bit microseconds; Vita accepts 32-bit relative
        // intervals. Chunk long waits rather than truncating or waiting forever.
        while (usecs) {
            SceUInt timeout = usecs > UINT32_MAX ? UINT32_MAX : static_cast<SceUInt>(usecs);
            const SceUInt interval = timeout;
            if (success_or_timeout(sceKernelWaitSema(id_, 1, &timeout))) return true;
            usecs -= interval;
        }
        return false;
    }
    void signal(int count = 1) {
        if (count < 0) std::abort();
        if (count && sceKernelSignalSema(id_, count) < 0) std::abort();
    }
};
}
