

#ifndef MULTICLASS_TSETLIN_MACHINE_INCLUDE
#define MULTICLASS_TSETLIN_MACHINE_INCLUDE

#include "TsetlinMachine.h"

#ifndef ARGMAX_TIEBREAK
#define AM_LAST 0
#define AM_FIRST 1
#define AM_RANDOM 2
#define ARGMAX_TIEBREAK AM_LAST
#endif  // ARGMAX_TIEBREAK

template <size_t num_classes, typename config>
class MultiClassTsetlinMachine {
    TsetlinMachine<config>
        machines[num_classes];
};

#endif