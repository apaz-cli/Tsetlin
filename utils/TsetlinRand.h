#include <cstdint>
#include <limits>

#ifndef TSETLIN_RAND_INCLUDE
#define TSETLIN_RAND_INCLUDE

#include "TsetlinBitset.h"

class TsetlinRandGen {
    constexpr static float max_16 = 0xffff;
    constexpr static float max_16_inv = 1. / max_16;
    constexpr static double max_32 = (double)0xffffffff;
    constexpr static double max_32_inv = 1. / max_32;
    constexpr static double max_64 = (double)0xffffffffffffffff;
    constexpr static double max_64_inv = 1. / max_64;

   public:
    uint64_t state;

    inline TsetlinRandGen(uint64_t seed = 0xabcdef0123456789) { state = seed; }

    // https://en.wikipedia.org/wiki/Xorshift

    uint32_t
    rand_32() noexcept {
        uint32_t x = (uint32_t)state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = state ^ x;
        return x;
    }

    uint64_t
    rand_64() noexcept {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x;
    }

    // Return a 0 or a 1 with probability p.
    bool
    rand_bernoulli(float p) noexcept {
        return (this->rand_64() * max_64_inv) <= p;
    }

    // Returns bit with probability:
    //   p in 0x01 position,
    // 1-p in 0x10 position
    // 0 otherwise.
    // Places t1 in bit 0x1
    // Places t2 in bit 0x2
    uint8_t
    sample_feedback(float p, bool polarity) noexcept {
        p = std::abs(polarity - p);
        // float loses precision, but that's okay.
        uint64_t r = rand_64();
        uint64_t s1 = r & 0x00000000ffffffff;
        uint64_t s2 = r & 0xffffffff00000000;
        uint32_t r1 = s1 >> 0;
        uint32_t r2 = s2 >> 32;
        double d1 = r1 * max_32_inv;
        double d2 = r2 * max_32_inv;

        // abs(0 - x)=x, abs(1-x)=1-x
        // Therefore, 1 polarity (positive clause) inverts p,
        // and 0 keeps it the same.

        uint8_t t1 = d1 <= p;
        uint8_t t2 = d2 <= (1 - p);

        uint8_t a1 = (t1 << 0);
        uint8_t a2 = (t2 << 1);
        uint8_t samples = (a1 | a2);
        return samples;
    }

    uint8_t
    biased_bits_4(float p) noexcept {
        uint64_t r = rand_64();
        uint64_t s1 = r & 0x000000000000ffff;
        uint64_t s2 = r & 0x00000000ffff0000;
        uint64_t s3 = r & 0x0000ffff00000000;
        uint64_t s4 = r & 0xffff000000000000;
        uint16_t r1 = s1 >> 0;
        uint16_t r2 = s2 >> 16;
        uint16_t r3 = s3 >> 32;
        uint16_t r4 = s4 >> 48;
        float d1 = r1 * max_16_inv;
        float d2 = r2 * max_16_inv;
        float d3 = r3 * max_16_inv;
        float d4 = r4 * max_16_inv;
        uint8_t t1 = d1 <= p;
        uint8_t t2 = d2 <= p;
        uint8_t t3 = d3 <= p;
        uint8_t t4 = d4 <= p;
        uint8_t a1 = (t1 << 0);
        uint8_t a2 = (t2 << 1);
        uint8_t a3 = (t3 << 2);
        uint8_t a4 = (t4 << 3);
        uint8_t four = (a1 | a2 | a3 | a4);
        return four;
    }

    uint8_t
    biased_bits_8(float p) noexcept {
        uint64_t r1 = rand_64();
        uint64_t r2 = rand_64();
        uint64_t s1 = r1 & 0x000000000000ffff;
        uint64_t s2 = r1 & 0x00000000ffff0000;
        uint64_t s3 = r1 & 0x0000ffff00000000;
        uint64_t s4 = r1 & 0xffff000000000000;
        uint64_t s5 = r2 & 0x000000000000ffff;
        uint64_t s6 = r2 & 0x00000000ffff0000;
        uint64_t s7 = r2 & 0x0000ffff00000000;
        uint64_t s8 = r2 & 0xffff000000000000;
        uint16_t c1 = s1 >> 0;
        uint16_t c2 = s2 >> 16;
        uint16_t c3 = s3 >> 32;
        uint16_t c4 = s4 >> 48;
        uint16_t c5 = s5 >> 0;
        uint16_t c6 = s6 >> 16;
        uint16_t c7 = s7 >> 32;
        uint16_t c8 = s8 >> 48;
        float d1 = c1 * max_16_inv;
        float d2 = c2 * max_16_inv;
        float d3 = c3 * max_16_inv;
        float d4 = c4 * max_16_inv;
        float d5 = c5 * max_16_inv;
        float d6 = c6 * max_16_inv;
        float d7 = c7 * max_16_inv;
        float d8 = c8 * max_16_inv;
        uint8_t t1 = d1 <= p;
        uint8_t t2 = d2 <= p;
        uint8_t t3 = d3 <= p;
        uint8_t t4 = d4 <= p;
        uint8_t t5 = d5 <= p;
        uint8_t t6 = d6 <= p;
        uint8_t t7 = d7 <= p;
        uint8_t t8 = d8 <= p;
        uint8_t a1 = (t1 << 0);
        uint8_t a2 = (t2 << 1);
        uint8_t a3 = (t3 << 2);
        uint8_t a4 = (t4 << 3);
        uint8_t a5 = (t5 << 4);
        uint8_t a6 = (t6 << 5);
        uint8_t a7 = (t7 << 6);
        uint8_t a8 = (t8 << 7);
        uint8_t eight = (a1 | a2 | a3 | a4 | a5 | a6 | a7 | a8);
        return eight;
    }

    uint32_t
    biased_bits_32(float p) noexcept {
        // Clang inlines all these calls.
        uint32_t a1 = biased_bits_8(p);
        uint32_t a2 = biased_bits_8(p);
        uint32_t a3 = biased_bits_8(p);
        uint32_t a4 = biased_bits_8(p);
        uint32_t e1 = (a1 << 0);
        uint32_t e2 = (a2 << 8);
        uint32_t e3 = (a3 << 16);
        uint32_t e4 = (a4 << 24);
        uint32_t thirty_two = (e1 | e2 | e3 | e4);
        return thirty_two;
    }

    uint64_t
    biased_bits_64(float p) noexcept {
        uint64_t a1 = biased_bits_8(p);
        uint64_t a2 = biased_bits_8(p);
        uint64_t a3 = biased_bits_8(p);
        uint64_t a4 = biased_bits_8(p);
        uint64_t a5 = biased_bits_8(p);
        uint64_t a6 = biased_bits_8(p);
        uint64_t a7 = biased_bits_8(p);
        uint64_t a8 = biased_bits_8(p);
        uint64_t e1 = (a1 << 0);
        uint64_t e2 = (a2 << 8);
        uint64_t e3 = (a3 << 16);
        uint64_t e4 = (a4 << 24);
        uint64_t e5 = (a5 << 32);
        uint64_t e6 = (a6 << 40);
        uint64_t e7 = (a7 << 48);
        uint64_t e8 = (a8 << 56);
        uint64_t sixty_four = (e1 | e2 | e3 | e4 | e5 | e6 | e7 | e8);
        return sixty_four;
    }

    // Destroys the zeroes at the end. Don't use .count() after.
    // That's okay though, because this is only used for sampling
    // feedback.
    template <size_t bits>
    void
    biased_bits(TBitset<bits>& to_pack, float p) noexcept {
        static constexpr bool _32_bit = sizeof(tint) == sizeof(uint32_t);
        static constexpr size_t backing_size =
            (bits / TINT_BIT_NUM) + ((bits % TINT_BIT_NUM) != 0);

        std::array<uint64_t, backing_size>& backing = to_pack.get_backing();
        for (size_t i = 0; i < backing_size; i++)
            backing[i] = _32_bit ? biased_bits_32(p) : biased_bits_64(p);
    }
};

#endif  // TSETLIN_RAND_INCLUDE