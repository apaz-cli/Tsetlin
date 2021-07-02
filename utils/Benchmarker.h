#include <chrono>
#include <iostream>

#ifndef TIMER_DISABLE
#define TIMER_DISABLE 0
#endif

#define TIMER_START \
    auto benchmarker_start_time = std::chrono::high_resolution_clock::now();
#define TIMER_END \
    auto benchmarker_end_time = std::chrono::high_resolution_clock::now();
#define _TIMER_PRINT

#define _TIMER_SHOW_BEGIN     \
    do {                      \
        if (!TIMER_DISABLE) { \
        auto diff = std::chrono::duration_cast <
#define _TIMER_SHOW_END                                        \
    > (benchmarker_end_time - benchmarker_start_time).count(); \
    std::cout << '\n' << diff << std::endl;                    \
    std::flush(std::cout);                                     \
    }                                                          \
    }                                                          \
    while (0)                                                  \
        ;

#define TIMER_SHOW_MS \
    _TIMER_SHOW_BEGIN std::chrono::milliseconds _TIMER_SHOW_END
#define TIMER_SHOW_SEC _TIMER_SHOW_BEGIN std::chrono::seconds _TIMER_SHOW_END
#define TIMER_SHOW_MIN _TIMER_SHOW_BEGIN std::chrono::minutes _TIMER_SHOW_END
#define TIMER_SHOW_MIC _TIMER_SHOW_BEGIN std::chrono::microseconds _TIMER_SHOW_END

static inline void
example_usage() {
    TIMER_START
    // Code to benchmark
    TIMER_END
    TIMER_SHOW_MIN
}
