// Compiled for Vita against the real SDK. Execution requires Vita OS/hardware.
#include "blockingconcurrentqueue.h"
extern "C" int duck_vita_queue_probe() {
    duck_vita::Semaphore semaphore;
    if (semaphore.try_wait() || semaphore.timed_wait(0)) return 1;
    semaphore.signal(2);
    if (!semaphore.wait() || !semaphore.timed_wait(1000)) return 2;
    moodycamel::BlockingConcurrentQueue<int> queue;
    if (!queue.enqueue(42)) return 3;
    int result = 0;
    if (!queue.wait_dequeue_timed(result, 1000) || result != 42) return 4;
    return 0;
}
