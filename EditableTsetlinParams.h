// Note: Automata is the plural of Automaton.

/////////////////////////////
// Base Machine Parameters //
/////////////////////////////

// Fall back to a 32 bit implementation if BITS_32 is 1.
// Otherwise, use the 64 bit implementation.
// This is wrapped with ifndef so it can be declared from 
// the command line with -DBITS_32 0.
#ifndef BITS_32
#define BITS_32 0
#endif


// The input size of the tsetlin machine in bits.
// This must be a multiple of 64 because the input is 
// packed into a buffer of uint64.
#define TSETLIN_INPUT_BIT_SIZE 128

// The number of clauses in each TsetlinMachine that
// have positive polarity.
#define TSETLIN_POS_CLAUSE_NUM 32

// The number of clauses in each TsetlinMachine that
// have negative polarity.
#define TSETLIN_NEG_CLAUSE_NUM 32

// The number of states per TsetlinAutomaton.
// The TsetlinAutomaton's type only describes its state.
// Make sure it is signed and maxvalue(TsetlinAutomaton) >= TSETLIN_N_STATES.
#define TSETLIN_N_STATES 8
#define TsetlinAutomaton char

////////////////////////////////
// Derived Machine Parameters //
////////////////////////////////

// The number of TsetlinMachines that vote to produce the final
// result for a TsetlinEnsemble.
#define TSETLIN_ENSEMBLE_SIZE 32

// The number of TsetlinMachines that compete to produce the final 
// result for a MultiClassTsetlinMachine.
// This number must be greater than two. If you want two, you're 
// doing binary classification and you should use an ensemble instead.
#define TSETLIN_CLASS_NUM 32

// How to break ties on argmax.
// AM_LAST is computationally cheapest.
// AM_RANDOM is computationally most expensive.
#define AM_LAST 0
#define AM_FIRST 1
#define AM_RANDOM 2
#define TSETLIN_ARGMAX_TIEBREAK AM_LAST
