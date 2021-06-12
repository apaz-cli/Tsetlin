#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>

#include "TsetlinParams.h"

// A basic Tsetlin machine.
// Takes a binary string as input and returns a single bit answer.
struct TsetlinMachine {
    TsetlinAutomaton automata_states[TSETLIN_CLAUSE_NUM][TSETLIN_NUM_AUTOMATA];
};
typedef struct TsetlinMachine TsetlinMachine;

// An ensemble of Tsetlin machines.
// Also takes a binary string as input and returns a single bit answer.
struct TsetlinEnsemble {
    TsetlinMachine machines[TSETLIN_ENSEMBLE_SIZE];
};
typedef struct TsetlinEnsemble TsetlinEnsemble;

// A Tsetlin machine based Classifier.
// Takes a binary string as input and returns a class (0 through TSETLIN_CLASS_NUM-1).
struct MultiClassTsetlinMachine {
    TsetlinMachine machines[TSETLIN_CLASS_NUM];
};
typedef struct MultiClassTsetlinMachine MultiClassTsetlinMachine;


/////////////////////////////////////
// Tsetlin Machine Building Blocks //
/////////////////////////////////////

///////////////////////
// Forward Functions //
///////////////////////

static inline void
make_literals(tint* input, tint* input_conjugate_space) {
    for (size_t i = 0; i < TSETLIN_INPUT_BUF_LEN; i++)
        input_conjugate_space[i] = ~input[i];
}

// Computes the output of a single clause.
// Input: input vector, clause number
// Output: 1 or 0
static inline bool
clause_forward(TsetlinMachine* tm, size_t clause_num, tint* input, tint* input_conjugate) {
    
    // Get the group of automata for this clause
    TsetlinAutomaton* clause_automata = tm->automata_states[clause_num];

    // Note that TSETLIN_INPUT_BUF_LEN*2 = TSETLIN_NUM_AUTOMATA/TINT_BITNUM.
    // There are twice as many automata per clause as there are bits in the input.
    // They match up 1-1.
    tint automaton_results[TSETLIN_INPUT_BUF_LEN*2];

    // Evaluate the automata and pack into 32 or 64-bit buffers.
    // The number of bits in each buffer is TINT_BITNUM.
    // The results are an "inclusion mask" for our input vectors.
    for (size_t buf_idx = 0; buf_idx < TSETLIN_INPUT_BUF_LEN*2; buf_idx++) {

        // For each buffer of the results, pack its bits
        automaton_results[buf_idx] = 0;

        // Force this inner loop to unroll.
        #pragma clang loop unroll(enable)
        for (size_t i = 0; i < TINT_BITNUM; i++) {
            // For each automaton, pack a bit into the results.
            automaton_results[buf_idx] |= 
                ((clause_automata[buf_idx*TINT_BITNUM+i] >= 0) << i);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Now "AND" all of the bits of the input and the conjugate together for //
    // which the corresponding automaton is in the including state.          //
    // However, doing this directly is inefficient. Instead, refer to the    //
    // truth table below.                                                    //
    //                                                                       //
    //            inp                                                        //
    //          1     0                                                      //
    //       X-----X-----X                                                   //
    //     1 X  1  |  0  X                                                   //
    // inc   X-----X-----X                                                   //
    //     0 X  1  |  1  X                                                   //
    //       X-----X-----X                                                   //
    //                                                                       //
    // We want to evaluate this truth table for each bit of inp and inc,     //
    // then return 1 if all the bits of inp and inc were 1, or return 0 if   //
    // even a single bit was 0.                                              //
    //                                                                       //
    // This can be accomplished with the bit manipulation:                   //
    //                                                                       //
    // if ((inc ^ inp) & inc) return 0;                                      //
    // else return 1;                                                        //
    //                                                                       //
    // Truth table for ((inc ^ inp) & inc):                                  //
    // let    inc: 1010                                                      //
    // let    inp: 1100                                                      //
    // calc   xor: 0110                                                      //
    // recall inc: 1010                                                      //
    // calc   and: 0010                                                      //
    //                                                                       //
    // In this way if even a single bit of ((inc ^ inp) & inc) is set,       //
    // we know the result of ANDing will be zero, so we return zero.         //
    ///////////////////////////////////////////////////////////////////////////

    tint ret = 0;
    for (size_t i = 0; i < TSETLIN_INPUT_BUF_LEN; i++) {
        tint inc  = automaton_results[i];
        tint inp  = input[i];
        // Returning from inside the loop slaughters the compiler.
        ret &= (inc ^ inp) & inc;
    }

    // If we've already ANDed a zero, the whole thing 
    // will be zero, so return.
    if (ret) return 0;

    // Now the conjugate
    for (size_t i = 0; i < TSETLIN_INPUT_BUF_LEN; i++) {
        tint inc  = automaton_results[TSETLIN_INPUT_BUF_LEN+i];
        tint inp  = input_conjugate[i];
        ret &= (inc ^ inp) & inc;
    }

    // return 0 if ret has any bits set, else 1.
    // Are clauses without literals handled correctly?
    return !ret;
}

static inline int
summation_forward(bool* pos_clause_outputs, bool* neg_clause_outputs) {
    int pos = 0, neg = 0;

    // I'm going to trust clang to make the right call
    // about unrolling these loops.
    for (uint i = 0; i < TSETLIN_POS_CLAUSE_NUM; i++) {
        pos += pos_clause_outputs[i];
    }

    for (uint i = 0; i < TSETLIN_NEG_CLAUSE_NUM; i++) {
        neg += neg_clause_outputs[i];
    }

    return pos - neg;
}

static inline uint
argmax_forward(int* sums, const uint n) {
#if TSETLIN_ARGMAX_TIEBREAK == AM_LAST
    uint argmax = 0;
    int highest = sums[argmax];
    
    // Trust in clang to unroll
    for (uint i = 1; i < n; i++) {
        if (sums[i] >= highest) {
            argmax = i;
            highest = sums[i];
        }
    }

    return argmax;
#elif TSETLIN_ARGMAX_TIEBREAK == AM_FIRST
    uint argmax = n-1;
    int highest = sums[argmax];

    for (int i = n-2; i >= 0; i--) {
        if (sums[i] >= highest) {
            argmax = i;
            highest = sums[i];
        }
    }

    return argmax;
#elif TSETLIN_ARGMAX_TIEBREAK == AM_RANDOM

    uint is[n];
    int ses[n];
    
    is[0] = 0;
    ses[0] = sums[0];
    uint stack_height = 1;

    // Build a stack of potential argmaxes.
    #pragma clang loop_unroll(enable)
    for (uint i = 1; i < n; i++) {
        if (sums[i] >= ses[stack_height-1]) {
            is[stack_height] = i;
            ses[stack_height] = sums[i];
            stack_height += 1;
        }
    }

    // Find the number of ties
    uint num_ties = 0;
    int largest = ses[stack_height-1];
    #pragma clang loop_unroll(enable)
    for (int i = stack_height-1; i >= 0; i--) {
        if (ses[i] != largest) break;
        num_ties++;
    }

    // Select a random tie to win
    uint startidx = stack_height - num_ties;
    return num_ties == 1 ?
               is[startidx] :
               is[startidx+(rand()%num_ties)];
#endif
}

////////////////////////
// Backward Functions //
////////////////////////

// Apply on False Positive Output
static inline void
clause_apply_type1_backward() {}

static inline void
clause_apply_type2_backward() {}



/////////////////////////////
// Tsetlin Machine Methods //
/////////////////////////////

static inline void
TsetlinMachine_init(TsetlinMachine* tm) {
    // Initialize automata state randomly to -1 or 0.
    srand(time(NULL));
    for (size_t i = 0; i < TSETLIN_CLAUSE_NUM; i++)
        for(size_t j = 0; j++ < TSETLIN_NUM_AUTOMATA; j++)
            tm->automata_states[i][j] = -(rand() & 1);
}

static inline bool
TsetlinMachine_forward(TsetlinMachine* tm, tint* input) {

    // Conjugate the input
    tint input_conjugate[TSETLIN_INPUT_BUF_LEN];
    make_literals(input, input_conjugate);

    // Compute positive and negative polarity clauses
    bool pos_clause_outputs[TSETLIN_POS_CLAUSE_NUM];
    bool neg_clause_outputs[TSETLIN_NEG_CLAUSE_NUM];
    for (size_t i = 0; i < TSETLIN_POS_CLAUSE_NUM; i++)
        pos_clause_outputs[i] = clause_forward(tm, i, input, input_conjugate);
    for (size_t i = TSETLIN_POS_CLAUSE_NUM; i < TSETLIN_CLAUSE_NUM; i++)
        pos_clause_outputs[i] = clause_forward(tm, i, input, input_conjugate);
    
    // Sum positive and negative clauses
    int sum = summation_forward(pos_clause_outputs, neg_clause_outputs);

    // Threshold
    return sum >= 0;
}

static inline void
TsetlinMachine_forward_backward(TsetlinMachine* tm, tint* input) {

}


//////////////////////////////
// Tsetlin Ensemble Methods //
//////////////////////////////

static inline void
TsetlinEnsemble_init(TsetlinEnsemble* en) {
    for (size_t i = 0; i < TSETLIN_ENSEMBLE_SIZE; i++) 
        TsetlinMachine_init(en->machines + i);
}

static inline void
TsetlinEnsemble_forward(TsetlinEnsemble* en, tint* input) {

}

static inline void
TsetlinEnsemble_forward_backward(TsetlinEnsemble* en, tint* input) {

}
