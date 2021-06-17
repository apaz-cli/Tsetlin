// #include "TsetlinParams.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <random>
#include <algorithm>


#include "TsetlinConfig.h"
#include "TsetlinRand.h"

#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

/////////////////////
// Tsetlin Machine //
/////////////////////

template <typename TsetlinAutomaton, size_t num_clauses, size_t num_automata>
struct TsetlinMachineCheckpoint {
    TsetlinAutomaton automata_states[num_clauses][num_automata];
    TsetlinRandGen rgen;
    static_assert(num_clauses % 2,
                  "The number of clauses must be divisible by 2.");
};

template <size_t input_buf_len, size_t num_clauses, size_t summation_target,
          typename TsetlinAutomaton = char, size_t num_states = 128>
class TsetlinMachine {
   private:
    // Calculated Parameters
    static_assert(!(num_clauses % 2),
                  "The number of clauses must be divisible by 2.");
    static_assert(num_clauses, "The number of clauses must not be zero.");
    static_assert(input_buf_len, "The input buffer size must not be zero.");

    static constexpr size_t input_bits = input_buf_len * TINT_BIT_NUM;
    static constexpr size_t num_automata = input_bits * 2;
    static constexpr size_t clause_output_buf_len =
        (num_clauses / TINT_BIT_NUM) + ((num_clauses % TINT_BIT_NUM) != 0);

    TsetlinAutomaton automata_states[num_clauses][num_automata];
    TsetlinRandGen rgen;

   public:
    void
    print_constexprs() {
        std::cout << "TINT_BIT_NUM: " << TINT_BIT_NUM << std::endl;
        std::cout << "input_buf_len: " << input_buf_len << std::endl;
        std::cout << "num_clauses: " << num_clauses << std::endl;
        std::cout << "input_bits: " << input_bits << std::endl;
        std::cout << "num_automata: " << num_automata << std::endl;
        std::cout << "clause_output_buf_len: " << clause_output_buf_len
                  << std::endl;
        std::flush(std::cout);
    }

    inline TsetlinMachine(tint seed = 0) {
        rgen = TsetlinRandGen(seed);
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                automata_states[y][x] = -(rand() & 1);
    }

    inline void
    literals_forward(std::array<tint, input_buf_len> &input,
                     std::array<tint, input_buf_len> &input_conjugate) {
        for (size_t i = 0; i < input_buf_len; i++)
            input_conjugate[i] = ~input[i];
    }

    inline bool
    clause_forward(size_t clause_num, std::array<tint, input_buf_len> &input,
                   std::array<tint, input_buf_len> &input_conjugate,
                   std::array<tint, num_automata> &automaton_include) {
        // Get the group of automata for this clause
        TsetlinAutomaton *clause_automata = automata_states[clause_num];

        // Evaluate the automata and pack into 32 or 64-bit buffers.
        // The number of bits in each buffer is TINT_BIT_NUM.
        // The results are an "inclusion mask" for our input vectors.
        for (size_t buf_idx = 0; buf_idx < input_buf_len * 2; buf_idx++) {
            // For each buffer of the results, pack its bits
            automaton_include[buf_idx] = 0;

            // Force this inner loop to unroll.
            #pragma clang loop unroll(enable)
            for (size_t i = 0; i < TINT_BIT_NUM; i++) {
                // For each automaton, pack a bit into the results.
                automaton_include[buf_idx] |=
                    ((tint)(clause_automata[buf_idx * TINT_BIT_NUM + i] >= 0)
                     << i);
            }
        }

        //            inp
        //          1     0
        //       X-----X-----X
        //     1 X  1  |  0  X
        // inc   X-----X-----X
        //     0 X  1  |  1  X
        //       X-----X-----X

        // Compute the truth table, 32 or 64 bits at a time.

        tint ret = TINT_MAX;  // All 1s
        for (size_t i = 0; i < input_buf_len; i++) {
            tint inc = automaton_include[i];
            tint inp = input[i];
            // Returning from inside the loop slaughters the compiler.
            ret &= (~inc | inp);
        }

        // If we've already ANDed a zero, return.
        if (ret != TINT_MAX) return 0;

        // Now do the same with the conjugate
        for (size_t i = 0; i < input_buf_len; i++) {
            tint inc = automaton_include[input_buf_len + i];
            tint inp = input_conjugate[i];
            ret &= (~inc | inp);
        }

        if (ret != TINT_MAX) return 0;

        return 1;
    }

    inline void
    clauses_forward_bool(std::array<tint, input_buf_len> &input,
                    std::array<tint, input_buf_len> &input_conjugate,
                    std::array<bool, num_clauses> &clause_outputs) {

        std::array<tint, num_automata> automata_include;
        for (size_t i = 0; i < num_clauses; i++)
            clause_outputs[i] = clause_forward(i, input, input_conjugate, automata_include);
    }

    static inline int
    summation_forward_bool(std::array<bool, num_clauses> &clause_outputs) {
        constexpr size_t halfway = num_clauses / 2;
        int pos = 0, neg = 0;
        for (size_t i = 0; i < halfway; i++) pos += clause_outputs[i];
        for (size_t i = 0; i < halfway; i++) neg += clause_outputs[i];
        return pos - neg;
    }

    static inline bool
    threshold_forward(int sum) {
        return sum >= 0;
    }

    inline bool
    forward(std::array<tint, input_buf_len> &input) {
        // Literals forward
        std::array<tint, input_buf_len> input_conjugate;
        literals_forward(input, input_conjugate);

        // Clauses forward
        std::array<bool, num_clauses> clause_outputs;
        clauses_forward_bool(input, input_conjugate, clause_outputs);

        // Summation forward
        int sum = summation_forward_bool(clause_outputs);

        // Threshold
        return threshold_forward(sum);
    }

    ///////////////
    // Backwards //
    ///////////////

    static inline int
    clip(int x) {
        constexpr int iT = (int)summation_target;
        return std::max(-iT, std::min(x, iT));
    }

    inline bool
    forward_backward(std::array<tint, input_buf_len> &input,
                     bool desired_output) {
        /////////////
        // Forward //
        /////////////

        // Literals forward
        std::array<tint, input_buf_len> input_conjugate;
        literals_forward(input, input_conjugate);

        // Clauses forward
        std::array<bool, num_clauses> clause_outputs;
        clauses_forward_bool(input, input_conjugate, clause_outputs);

        // Summation forward
        int sum = summation_forward_bool(clause_outputs);

        // Threshold
        bool output = threshold_forward(sum);

        //////////////
        // Backward //
        //////////////

        TsetlinAutomaton feedback[num_clauses][num_automata];
        static constexpr size_t _T = summation_target;
        static constexpr float _2T = 2.0 * _T;
        float pos_prob = (_T + clip(sum)) / _2T;
        float neg_prob = (_T - clip(sum)) / _2T;

        static constexpr size_t halfway = num_clauses / 2;
        for (size_t j = 0; j < halfway; j++) {
            if (output) {
                // type 1 to positive clause
                if (rgen.rand_bernoulli(pos_prob)) {

                }
            } else {
                // type 2 to positive clause
                if (rgen.rand_bernoulli(neg_prob)) {
                    
                }
            }
        }

        for (size_t j = halfway; j < num_clauses; j++) {
            if (output) {
                // type 2 to negative clause
                if (rgen.rand_bernoulli(neg_prob)) {

                }
            } else {
                // type 1 to negative clause
                if (rgen.rand_bernoulli(pos_prob)) {

                }
            }
        }

        return output;
    }

    // This is how I would train a Tsetlin Machine
    inline bool
    apaz_forward_backward(std::array<tint, input_buf_len> &input,
                     bool desired_output) {
        /////////////
        // Forward //
        /////////////

        // Literals forward
        std::array<tint, input_buf_len> input_conjugate;
        literals_forward(input, input_conjugate);

        // Clauses forward
        std::array<bool, num_clauses> clause_outputs;
        clauses_forward_bool(input, input_conjugate, clause_outputs);

        // Summation forward
        int sum = summation_forward_bool(clause_outputs);

        // Threshold
        bool output = threshold_forward(sum);

        //////////////
        // Backward //
        //////////////

        static constexpr size_t _T = summation_target;
        static constexpr float _2T = 2.0 * _T;
        float pos_prob = (_T + clip(sum)) / _2T;
        float neg_prob = (_T - clip(sum)) / _2T;

        

    }

    /////////////
    // UTILITY //
    /////////////

    inline void
    checkpoint(TsetlinMachineCheckpoint<TsetlinAutomaton, num_clauses,
                                        num_automata> &checkpoint) {
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                checkpoint.automata_states[y][x] = automata_states[y][x];
        checkpoint.rgen = rgen;
    }

    inline void
    resumeCheckpoint(TsetlinMachineCheckpoint<TsetlinAutomaton, num_clauses,
                                              num_automata> &checkpoint) {
        rgen = checkpoint.rgen;
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                automata_states[y][x] = checkpoint.automata_states[y][x];
    }
};

#endif  // TSETLIN_MACHINE_INCLUDE