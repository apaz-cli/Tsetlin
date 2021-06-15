// #include "TsetlinParams.h"
#include <array>
#include <cstddef>
#include <iostream>
#include <ostream>

#include "TsetlinConfig.h"

#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

/////////////////////
// Tsetlin Machine //
/////////////////////

template <size_t input_buf_len, size_t summation_target, size_t num_clauses = 64,
          typename TsetlinAutomaton = char, TsetlinAutomaton num_states = 127>
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

   public:
    const void
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

    constexpr TsetlinMachine() {
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                automata_states[y][x] = -(rand() & 1);
    }

    const inline bool
    clause_forward(size_t clause_num, std::array<tint, input_buf_len> &input,
                   std::array<tint, input_buf_len> &input_conjugate) {
        // Get the group of automata for this clause
        TsetlinAutomaton *clause_automata = automata_states[clause_num];

        tint automaton_include[input_buf_len * 2];

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

    const inline void
    literals_clauses_forward(
        std::array<tint, input_buf_len> &input,
        std::array<tint, clause_output_buf_len> &clause_output_buf) {
        // Conjugate the input to form the literals
        std::array<tint, input_buf_len> input_conjugate;
        for (size_t i = 0; i < input_buf_len; i++)
            input_conjugate[i] = ~input[i];

        #pragma clang loop unroll(enable)
        // For each bit, pack it into the buffer
        for (size_t i = 0; i < num_clauses; i++)
            clause_output_buf[i / TINT_BIT_NUM] =
                clause_forward(i, input, input_conjugate) << (i % TINT_BIT_NUM);
    }

    const static inline int
    summation_forward(std::array<tint, clause_output_buf_len> &clause_outputs) {
        constexpr size_t halfway = clause_output_buf_len / 2;
        int pos = 0, neg = 0;
        #pragma clang loop unroll(enable)
        for (size_t i = 0; i < halfway; i++)
            pos += std::__popcount(clause_outputs[i]);
        #pragma clang loop unroll(enable)
        for (size_t i = halfway; i < clause_output_buf_len; i++)
            neg += std::__popcount(clause_outputs[i]);
        return pos - neg;
    }

    const inline int
    literals_clause_summation_forward(std::array<tint, input_buf_len> &input) {
        // Compute clause outputs
        std::array<tint, clause_output_buf_len> clause_outputs;
        clause_outputs.fill(0);
        literals_clauses_forward(input, clause_outputs);

        // Sum positive and negative clauses
        return summation_forward(clause_outputs);
    }

    const static inline bool
    threshold_forward(int sum) {
        return sum >= 0;
    }

    const inline bool
    forward(std::array<tint, input_buf_len> &input) {
        return threshold_forward(literals_clause_summation_forward(input));
    }

    const inline bool
    forward_backward(std::array<tint, input_buf_len> &input) {}
};

#endif  // TSETLIN_MACHINE_INCLUDE