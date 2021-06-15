#include <stdio.h>

#include "TsetlinMachine.h"

int main() {
    std::array<tint, 30> input;
    auto tm = TsetlinMachine<30, 128>();
    tm.print_constexprs();
    tm.forward(input);
}
