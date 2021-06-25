#include <ostream>
#include <bitset>

#include "../utils/TsetlinRand.h"

int
main() {
    // Endure that there are enough bits of randomness in the generator.
    TsetlinRandGen rg;
    for (size_t i = 0; i < 0x128000000; i++ )
        rg.rand();
    for (;;) std::cout << std::bitset<64>(rg.rand()) << "\n";
    //for (;;) std::cout << std::bitset<32>(rand()) << std::bitset<32>(rand()) << "\n";
}