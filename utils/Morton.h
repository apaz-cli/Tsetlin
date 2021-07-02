#ifndef MORTON_INCLUDE
#define MORTON_INCLUDE

// AES intrinsics
#include <immintrin.h>
#include <wmmintrin.h>

#include <cstdint>

// Interleave

static inline uint64_t
carryless_square(uint32_t x) {
    uint64_t val[2] = {x, 0};
    __m128i *a = (__m128i *)val;
    *a = _mm_clmulepi64_si128(*a, *a, 0);
    return val[0];
}

static inline void
xy32_to_morton64(uint32_t x, uint32_t y, uint64_t *m) {
    *m = carryless_square(x) | (carryless_square(y) << 1);
}

static inline void
xy32_to_morton64_2(uint32_t x, uint32_t y, uint32_t *m1, uint32_t *m2) {
    uint64_t i = carryless_square(x) | (carryless_square(y) << 1);
    *m1 = (uint32_t)((i & 0x00000000FFFFFFFF) >> 0);
    *m2 = (uint32_t)((i & 0xFFFFFFFF00000000) >> 32);
}

static inline void
xy64_to_morton128(uint64_t x, uint64_t y, uint64_t *m1, uint64_t *m2) {
    uint64_t x1 = (x & 0x00000000FFFFFFFF) >> 0;
    uint64_t y1 = (y & 0x00000000FFFFFFFF) >> 0;
    uint64_t x2 = (x & 0xFFFFFFFF00000000) >> 32;
    uint64_t y2 = (y & 0xFFFFFFFF00000000) >> 32;

    xy32_to_morton64(x1, y1, m1);
    xy32_to_morton64(x2, y2, m2);
}

// Un-Interleave

static inline void
morton64_to_xy32(uint64_t m, uint32_t *x, uint32_t *y) {
    static_assert(sizeof(uint64_t) == sizeof(unsigned long long),
                  "Size mismatch.");
    *x = _pext_u64(m, 0x5555555555555555);
    *y = _pext_u64(m, 0xaaaaaaaaaaaaaaaa);
}

static inline void
morton64_to_xy32_2(uint32_t m1, uint32_t m2, uint32_t *x, uint32_t *y) {
    uint64_t m = (uint64_t)m1 | ((uint64_t)m2 << 32);
    morton64_to_xy32(m, x, y);
}

static inline void
morton128_to_xy64(uint64_t m1, uint64_t m2, uint64_t *x, uint64_t *y) {
    uint32_t x1, y1, x2, y2;
    morton64_to_xy32(m1, &x1, &y1);
    morton64_to_xy32(m2, &x2, &y2);

    *x = (uint64_t)x1 | ((uint64_t)x2 << 32);
    *y = (uint64_t)y1 | ((uint64_t)y2 << 32);
}

// Useful debugging statements
/*
std::cout << "x:  " << std::bitset<64>(x) << std::endl;
std::cout << "x2: " << std::bitset<64>(x2) << std::endl;
std::cout << "x1: " << std::bitset<64>(x1) << std::endl;
std::cout << "y:  " << std::bitset<64>(y) << std::endl;
std::cout << "y2: " << std::bitset<64>(y2) << std::endl;
std::cout << "y1: " << std::bitset<64>(y1) << std::endl;
std::cout << "m1: " << std::bitset<64>(*m1) << std::endl;
std::cout << "m2: " << std::bitset<64>(*m2) << std::endl;
*/

#endif  // MORTON_INCLUDE