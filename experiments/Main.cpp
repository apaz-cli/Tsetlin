#include <stdio.h>
#include <cstddef>
#include <bitset>
#include <memory>
#include "../utils/TsetlinBitset.h"
#include "../machines/TsetlinMachine.h"

int main() {
    //std::array<tint, 30> input;
    //auto tm = TsetlinMachine<30, 64, 10>();
    //tm.print_constexprs();
    //tm.forward(input);

    
    
    TsetlinRandGen rng = TsetlinRandGen(0xf0f0f0f0);
    for (size_t i = 0; i < 1000000; i++) {
        // tint sum = rng.biased_bits(.9);
        std::cout << rng.rand_bernoulli(.1);
    }

    
}
