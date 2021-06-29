#include <bitset>
#include <ostream>

#include "../utils/TsetlinRand.h"

int
main() {
    // Endure that there are enough bits of randomness in the generator.
    TsetlinRandGen rg;
    for (size_t i = 0; i < 0x128000000; i++) rg.rand_32();
    for (size_t i = 0; i < 0x128000000; i++) std::cout << std::bitset<TINT_BIT_NUM>(rg.rand()) << '\n';
    // for (;;) std::cout << std::bitset<64>(rg.rand()) << "\n";
    // for (;;) std::cout << std::bitset<32>(rg.rand_32()) << "\n";
    // for (;;) std::cout << std::bitset<32>(rand()) << std::bitset<32>(rand())
    // << "\n";
}