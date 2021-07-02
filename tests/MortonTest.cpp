#include <bitset>
#include <iostream>

#include "../utils/Morton.h"

void
test1() {
    // 32 bit interleave and undo
    uint32_t x = 0xabcdef12;
    uint32_t y = 0x12abcdef;
    std::cout << "x: " << std::bitset<32>(x) << std::endl;
    std::cout << "y: " << std::bitset<32>(y) << std::endl;

    uint64_t m;
    xy32_to_morton64(x, y, &m);
    std::cout << "m: " << std::bitset<64>(m) << std::endl;

    morton64_to_xy32(m, &x, &y);
    std::cout << "x: " << std::bitset<32>(x) << std::endl;
    std::cout << "y: " << std::bitset<32>(y) << std::endl;
}

void
test2() {  // 64 bit interleave and undo
    uint64_t x = 0x55550000aaaaffff;
    uint64_t y = 0xffffaaaa00005555;
    std::cout << "x: " << std::bitset<64>(x) << std::endl;
    std::cout << "y: " << std::bitset<64>(y) << std::endl;

    uint64_t m1, m2;
    xy64_to_morton128(x, y, &m1, &m2);
    std::cout << "m: " << std::bitset<64>(m1) << std::bitset<64>(m2)
              << std::endl;

    morton128_to_xy64(m1, m2, &x, &y);

    std::cout << "x: " << std::bitset<64>(x) << std::endl;
    std::cout << "y: " << std::bitset<64>(y) << std::endl;
}

void
test3() {  // 32 bit interleave and undo with 2

    uint32_t x = 0xabcdef12;
    uint32_t y = 0x12abcdef;
    std::cout << "x: " << std::bitset<32>(x) << std::endl;
    std::cout << "y: " << std::bitset<32>(y) << std::endl;

    uint32_t m1, m2;
    xy32_to_morton64_2(x, y, &m1, &m2);
    std::cout << "m: " << std::bitset<32>(m2) << std::bitset<32>(m1)
              << std::endl;

    morton64_to_xy32_2(m1, m2, &x, &y);
    std::cout << "x: " << std::bitset<32>(x) << std::endl;
    std::cout << "y: " << std::bitset<32>(y) << std::endl;
}

int
main() {
    std::cout << "Test 1: " << std::endl;
    test1();
    std::cout << "\nTest 2:" << std::endl;
    test2();
    std::cout << "\nTest 3:" << std::endl;
    test3();
}