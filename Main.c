#include "TsetlinMachine.h"
#include <stdio.h>

MultiClassTsetlinMachine mctm;

int main() {
    printf("TSETLIN_NUM_AUTOMATA: %i\n", TSETLIN_NUM_AUTOMATA);
    printf("TSETLIN_NUM_AUTOMATA/TINT_BITNUM: %i\n", TSETLIN_NUM_AUTOMATA/TINT_BITNUM);
    printf("TSETLIN_INPUT_BIT_SIZE: %i\n", TSETLIN_INPUT_BIT_SIZE);
    printf("TSETLIN_INPUT_BUF_LEN: %i\n", TSETLIN_INPUT_BUF_LEN);
    printf("TSETLIN_CLAUSE_NUM: %i\n", TSETLIN_CLAUSE_NUM);

    srand(time(NULL));
    MultiClassTsetlinMachine_init(&mctm);
}
