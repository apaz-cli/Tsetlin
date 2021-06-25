#include <cstddef>
#include <iostream>
#include <ostream>

#include "../utils/TsetlinBitset.h"

int
main() {
    static constexpr size_t size = 64;
    TBitset<size> tb1, tb2, x;
    for (size_t i = 0; i < size; i++) {
        tb1[i] = (bool)(i % 2);
        tb2[i] = tb1[i];
    }

    tb2 = ~tb1;

    std::cout << "tb1 (01s)" << std::endl;
    std::cout << tb1 << std::endl;
    std::cout << "tb2 (10s, ~tb1)" << std::endl;
    std::cout << tb2 << std::endl;

    std::cout << "tb1^tb2 (1s)" << std::endl;
    x = (tb1 ^ tb2);
    std::cout << x << std::endl;

    std::cout << "tb1&tb2 (0s)" << std::endl;
    x = (tb1 & tb2);
    std::cout << x << std::endl;

    std::cout << "tb1|tb2 (1s)" << std::endl;
    x = (tb1 | tb2);
    std::cout << x << std::endl;
}