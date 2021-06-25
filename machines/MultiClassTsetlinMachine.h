#include "TsetlinMachine.h"

#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

#ifndef ARGMAX_TIEBREAK
#define AM_LAST 0
#define AM_FIRST 1
#define AM_RANDOM 2
#define ARGMAX_TIEBREAK AM_LAST
#endif  // ARGMAX_TIEBREAK

template <size_t num_classes, size_t input_bits, size_t num_clauses,
          size_t summation_target, float S, typename TsetlinAutomaton = char,
          size_t num_states = 128>
class MultiClassTsetlinMachine {
    TsetlinMachine<input_bits, num_clauses, summation_target, S,
                   TsetlinAutomaton, num_states>
        machines[num_classes];
};

#endif