#include <inttypes.h>

#include "EditableTsetlinParams.h"

#ifndef BITS_32
#define BITS_32 0
#endif
#if BITS_32 == 0
#define TINT_BITNUM 64
#define tint uint64_t
#else
#define TINT_BITNUM 32
#define tint uint32_t
#endif

// Note: Automata is the plural of Automaton.

///////////////////////////
// Calculated Parameters //
///////////////////////////

// The size of the array which is the input to the Tsetlin Machine.
// The input is expected in the form of bits packed into 
// an array `tint input[TSETLIN_INPUT_BUF_LEN];`.
#define TSETLIN_INPUT_BUF_LEN (TSETLIN_INPUT_BIT_SIZE/TINT_BITNUM)

// The number of TsetlinAutomata per clause per TsetlinMachine.
#define TSETLIN_NUM_AUTOMATA (TSETLIN_INPUT_BIT_SIZE * 2)

// The number of clauses in each TsetlinMachine that
// have positive polarity.
#define TSETLIN_POS_CLAUSE_NUM TSETLIN_CLAUSE_NUM/2

// The number of clauses in each TsetlinMachine that
// have negative polarity.
#define TSETLIN_NEG_CLAUSE_NUM TSETLIN_CLAUSE_NUM/2

