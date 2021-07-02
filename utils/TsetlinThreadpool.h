#ifndef TTHREADPOOL
#define TTHREADPOOL

#include <cstddef>
#include <mutex>
#include <thread>

template <size_t num_threads, typename output_type>
class TThreadpool {
    std::thread threads[num_threads];
    std::mutex task_mutex = std::mutex();

    std::function tasks[num_threads];
    output_type output_space[num_threads];

    TThreadpool() {
        auto await_work_fn = [this]() { 
            task_mutex.lock();
            auto work
            task_mutex.unlock();
         };

        for (size_t i = 0; i < num_threads; i++)
            threads[i] = std::thread(await_work_fn);
    }
};

#endif  // TTHREADPOOL