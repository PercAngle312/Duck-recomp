// Host API model: verifies adapter/queue behavior, not Vita OS behavior.
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <cstdio>
#include "blockingconcurrentqueue.h"

struct State { std::mutex mutex; std::condition_variable cv; int count; int max; };
static std::mutex registry_mutex;
static std::unordered_map<int, std::shared_ptr<State>> registry;
static int next_id = 1;
static bool virtual_time = false;
static std::vector<unsigned> intervals;
static std::atomic<int> wait_calls{0};
static std::shared_ptr<State> state(int id) {
    std::lock_guard lock(registry_mutex); return registry.at(id);
}
extern "C" int sceKernelCreateSema(const char*, unsigned, int initial, int max, void*) {
    auto s = std::make_shared<State>(); s->count = initial; s->max = max;
    std::lock_guard lock(registry_mutex); int id = next_id++; registry[id] = s; return id;
}
extern "C" int sceKernelDeleteSema(int id) {
    std::lock_guard lock(registry_mutex); return registry.erase(id) == 1 ? 0 : -1;
}
extern "C" int sceKernelPollSema(int id, int amount) {
    auto s = state(id); std::lock_guard lock(s->mutex);
    if (s->count < amount) return static_cast<int>(SCE_KERNEL_ERROR_SEMA_ZERO);
    s->count -= amount; return 0;
}
extern "C" int sceKernelWaitSema(int id, int amount, unsigned* timeout) {
    ++wait_calls;
    if (virtual_time) {
        assert(timeout); intervals.push_back(*timeout); *timeout = 0;
        return static_cast<int>(SCE_KERNEL_ERROR_WAIT_TIMEOUT);
    }
    auto s = state(id); std::unique_lock lock(s->mutex);
    if (timeout) {
        if (!s->cv.wait_for(lock, std::chrono::microseconds(*timeout), [&]{return s->count >= amount;}))
            return static_cast<int>(SCE_KERNEL_ERROR_WAIT_TIMEOUT);
    } else s->cv.wait(lock, [&]{return s->count >= amount;});
    s->count -= amount; return 0;
}
extern "C" int sceKernelSignalSema(int id, int amount) {
    auto s = state(id); std::lock_guard lock(s->mutex);
    assert(amount > 0 && s->count <= s->max - amount);
    s->count += amount; s->cv.notify_all(); return 0;
}
int main() {
    {
        duck_vita::Semaphore s(2);
        assert(s.try_wait()); assert(s.wait()); assert(!s.try_wait());
        assert(!s.timed_wait(0)); assert(!s.timed_wait(100));
        s.signal(0); assert(!s.try_wait());
        s.signal(3); for (int i=0;i<3;++i) assert(s.try_wait());
        virtual_time = true;
        assert(!s.timed_wait(std::uint64_t(UINT32_MAX) * 2 + 17));
        assert((intervals == std::vector<unsigned>{UINT32_MAX,UINT32_MAX,17}));
        virtual_time = false;
        std::thread producer([&]{std::this_thread::sleep_for(std::chrono::milliseconds(5));s.signal();});
        assert(s.timed_wait(1000000)); producer.join();
    }
    {
        // Force the real lightweight queue down its blocking OS adapter path.
        moodycamel::LightweightSemaphore s(0, 0);
        int before = wait_calls;
        assert(!s.wait(100)); assert(wait_calls > before);
        std::thread producer([&]{std::this_thread::sleep_for(std::chrono::milliseconds(5));s.signal(2);});
        assert(s.wait(1000000)); assert(s.wait(1000000)); producer.join();
    }
    {
        moodycamel::BlockingConcurrentQueue<int> queue;
        int value = -1; assert(!queue.wait_dequeue_timed(value, 100));
        constexpr int total = 20000;
        std::vector<unsigned char> seen(total);
        std::vector<std::thread> producers;
        for (int p=0;p<4;++p) producers.emplace_back([&,p]{
            for (int i=p;i<total;i+=4) assert(queue.enqueue(i));
        });
        for (int i=0;i<total;++i) {
            assert(queue.wait_dequeue_timed(value, 5000000));
            assert(value>=0 && value<total && !seen[value]); seen[value]=1;
        }
        for (auto& thread:producers) thread.join();
        assert(!queue.wait_dequeue_timed(value,100));
    }
    assert(registry.empty());
    std::puts("PASS: adapter polling/counts/timeouts/64-bit intervals/wakeup; blocking queue 20,000 unique items.");
}
