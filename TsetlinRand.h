#include <cstddef>
#include <cstdint>
#include <limits>

#include "TsetlinConfig.h"

// https://en.wikipedia.org/wiki/Xorshift#Xorshift.2B
#if TINT_BIT_NUM == 32
class TsetlinRandGen {
    constexpr static float max_16 = 65536;

   public:
    tint state;

    inline TsetlinRandGen(tint seed) { state = seed; }

    inline tint
    rand_tint() {
        tint x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    // Return a 0 or a 1 with probability p.
    inline bool
    rand_bernoulli(float p) {
        return (rand_tint() / (float)UINT32_MAX) <= p;
    }

    inline tint
    biased_bits(float p) {
        tint answer = 0;
        // #pragma clang rool unroll(enable)
        for (size_t i = 0; i < 8; i++) {
            tint ra = rand_tint();
            tint rb = rand_tint();
            uint16_t r1 = ((ra & 0x0000ffff) >> 0);
            uint16_t r2 = ((ra & 0xffff0000) >> 16);
            uint16_t r3 = ((rb & 0x0000ffff) >> 0);
            uint16_t r4 = ((rb & 0xffff0000) >> 16);
            float d1 = r1 / max_16;
            float d2 = r2 / max_16;
            float d3 = r3 / max_16;
            float d4 = r4 / max_16;
            bool t1 = d1 <= p;
            bool t2 = d2 <= p;
            bool t3 = d3 <= p;
            bool t4 = d4 <= p;
            char a1 = (t1 << 0);
            char a2 = (t2 << 1);
            char a3 = (t3 << 2);
            char a4 = (t4 << 3);
            answer <<= 4;
            answer |= (a1 | a2 | a3 | a4);
        }
        return answer;
    }
};
#else
// TINT_BIT_NUM == 64
class TsetlinRandGen {
    constexpr static float max_16 = 65536;

   public:
    tint state;

    inline TsetlinRandGen(tint seed) { state = seed; }

    inline tint
    rand_tint() {
        tint x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x;
    }

    // Return a 0 or a 1 with probability p.
    inline bool
    rand_bernoulli(float p) {
        return (rand_tint() / (float)UINT64_MAX) <= p;
    }

    inline tint
    biased_bits(float p) {
        tint answer = 0;
        // #pragma clang rool unroll(enable)
        for (size_t i = 0; i < 16; i++) {
            constexpr static float max_16 = 65536;
            tint r = rand_tint();
            uint16_t r1 = ((r & 0x000000000000ffff) >> 0);
            uint16_t r2 = ((r & 0x00000000ffff0000) >> 16);
            uint16_t r3 = ((r & 0x0000ffff00000000) >> 32);
            uint16_t r4 = ((r & 0xffff000000000000) >> 48);
            float d1 = r1 / max_16;
            float d2 = r2 / max_16;
            float d3 = r3 / max_16;
            float d4 = r4 / max_16;
            bool t1 = d1 <= p;
            bool t2 = d2 <= p;
            bool t3 = d3 <= p;
            bool t4 = d4 <= p;
            char a1 = (t1 << 0);
            char a2 = (t2 << 1);
            char a3 = (t3 << 2);
            char a4 = (t4 << 3);
            answer <<= 4;
            answer |= (a1 | a2 | a3 | a4);
        }
        return answer;
    }
};
#endif
