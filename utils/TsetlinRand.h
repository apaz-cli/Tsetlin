#include <bits/stdint-uintn.h>

#include <cstddef>
#include <cstdint>
#include <limits>

#include "TsetlinBitset.h"

#ifndef TSETLIN_RAND_INCLUDE
#define TSETLIN_RAND_INCLUDE

class TsetlinRandGen {
    constexpr static float max_16 = 65536.0;

   public:
    uint64_t state;

    inline TsetlinRandGen(uint64_t seed = 0xabcdef0123456789) { state = seed; }

    uint64_t
    rand() noexcept {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x;
    }

    uint32_t
    rand_32() noexcept {
        uint32_t x = (uint32_t)state;
        // TODO WHAT WERE THESE SUPPOSED TO BE FOR 32 BIT
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = state ^ x;
        return x;
    }

    // Return a 0 or a 1 with probability p.
    bool
    rand_bernoulli(float p) noexcept {
        return (this->rand() / (float)UINT64_MAX) <= p;
    }

    uint64_t
    biased_bits(float p) noexcept {
        uint64_t answer = 0;
        for (size_t i = 0; i < 16; i++) {
            constexpr static float max_16 = 65536;
            uint64_t r = rand();
            uint64_t s1 = r & 0x000000000000ffff;
            uint64_t s2 = r & 0x00000000ffff0000;
            uint64_t s3 = r & 0x0000ffff00000000;
            uint64_t s4 = r & 0xffff000000000000;
            uint16_t r1 = s1 >> 0;
            uint16_t r2 = s2 >> 16;
            uint16_t r3 = s3 >> 32;
            uint16_t r4 = s4 >> 48;
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

    template <size_t bits>
    void
    biased_bits(TBitset<bits>& to_pack) {
        size_t backing_buf_size = to_pack.backing_size();
        tint* backing = to_pack.get_backing();
        for (size_t i = 0; i < to_pack; i++)
            ;
    }
};

#endif  // TSETLIN_RAND_INCLUDE