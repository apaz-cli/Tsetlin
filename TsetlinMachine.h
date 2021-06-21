#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <random>

#include "TsetlinBitset.h"
#include "TsetlinConfig.h"
#include "TsetlinRand.h"

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

template <size_t input_bits, size_t num_clauses, size_t summation_target,
          typename TsetlinAutomaton = char, size_t num_states = 128>
class TsetlinMachine {
   private:
    // Calculated Parameters
    static_assert(!(num_clauses % 2),
                  "The number of clauses must be divisible by 2.");
    static_assert(num_clauses, "The number of clauses must not be zero.");
    static_assert(input_bits, "The number of input bits must not be zero.");

    static constexpr size_t num_automata = input_bits * 2;

    /////////////
    // Members //
    /////////////
    // TsetlinAutomaton automata_states[num_clauses][num_automata];
    std::array<std::array<TsetlinAutomaton, num_automata>, num_clauses>
        automata_states;
    TsetlinRandGen rgen;

   public:
    void
    print_constexprs() {
        std::cout << "num_clauses: " << num_clauses << std::endl;
        std::cout << "num_automata: " << num_automata << std::endl;
        std::cout << "input_bits: " << input_bits << std::endl;
        std::flush(std::cout);
    }

    TsetlinMachine(uint64_t seed = 0) {
        rgen = TsetlinRandGen(seed);
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                automata_states[y][x] = -(rgen.rand() & 1);
    }

    void
    literals_forward(TBitset<input_bits> &input,
                     TBitset<input_bits> &input_conjugate) {
        input_conjugate = ~input;
    }

    bool
    clause_forward(
        size_t clause_num, TBitset<input_bits> &input,
        TBitset<input_bits> &input_conjugate,
        TBitset<num_automata> &automaton_include_space) const noexcept {
        // Recall num_automata = input_bits * 2

        // Get the group of automata for this clause
        std::array<TsetlinAutomaton, num_automata> &clause_automata =
            automata_states[clause_num];

        // Evaluate the automata and pack into bitset.
        // The results are an "inclusion mask" for our input vectors.
        for (size_t i = 0; i < num_automata; i++)
            automaton_include_space[i] = (clause_automata[i] >= 0) ? 1 : 0;

        // Compute the following truth table, TINT_BIT_NUM bits at a time.
        // This is faster than using the builtin operators.
        auto input_b = input.get_backing();
        auto input_conj_b = input_conjugate.get_backing();
        auto inc_b = automaton_include_space.get_backing();

        //            inp
        //          1     0
        //       X-----X-----X
        //     1 X  1  |  0  X
        // inc   X-----X-----X
        //     0 X  1  |  1  X
        //       X-----X-----X

        tint ret = TINT_MAX;  // All 1s
        for (size_t i = 0; i < input.backing_size(); i++) {
            tint inc = inc_b[i];
            tint inp = input_b[i];
            ret &= (~inc | inp);
        }

        // Returning from inside the loop slaughters the compiler.
        // If we've already ANDed a zero, return.
        if (ret != TINT_MAX) return 0;

        // Now do the same with the conjugate
        for (size_t i = 0; i < input_conjugate.backing_size(); i++) {
            tint inc = inc_b[input.backing_size() + i];
            tint inp = input_conj_b[i];
            ret &= (~inc | inp);
        }

        if (ret != TINT_MAX) return 0;
        return 1;
    }

    void
    clauses_forward(TBitset<input_bits> &input,
                    TBitset<input_bits> &input_conjugate,
                    TBitset<num_automata> &automata_include,
                    TBitset<num_clauses> &output) {
        for (size_t i = 0; i < num_clauses; i++)
            output[i] =
                clause_forward(i, input, input_conjugate, automata_include);
    }

    static int
    summation_forward_bool(std::array<bool, num_clauses> &clause_outputs) {
        constexpr size_t halfway = num_clauses / 2;
        int pos = 0, neg = 0;
        for (size_t i = 0; i < halfway; i++) pos += clause_outputs[i];
        for (size_t i = 0; i < halfway; i++) neg += clause_outputs[i];
        return pos - neg;
    }

    static int
    summation_forward(TBitset<num_clauses> &clause_outputs) {
        // If the split is clean,
        constexpr size_t half_clauses = num_clauses / 2;
        if constexpr (half_clauses % TINT_BIT_NUM == 0) {
        } else {
        }

        return 0;
    }

    static bool
    threshold_forward(int sum) {
        return sum >= 0;
    }

    bool
    forward(TBitset<input_bits> &input) {
        // Literals forward
        std::array<tint, 0> input_conjugate;
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

    static int
    clip(int x) {
        constexpr int iT = (int)summation_target;
        return std::max(-iT, std::min(x, iT));
    }

    bool
    forward_backward(TBitset<input_bits> &input, bool desired_output) {
        /////////////
        // Forward //
        /////////////

        // Literals forward
        TBitset<input_bits> input_conjugate = TBitset<input_bits>();
        literals_forward(input, input_conjugate);

        // Clauses forward
        TBitset<num_clauses> clause_outputs;
        clauses_forward(input, input_conjugate, clause_outputs);

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
    bool
    apaz_forward_backward(TBitset<input_bits> &input, bool desired_output) {
        /////////////
        // Forward //
        /////////////

        // Literals forward
        TBitset<input_bits> input_conjugate = TBitset<input_bits>();
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

    void
    checkpoint(TsetlinMachineCheckpoint<TsetlinAutomaton, num_clauses,
                                        num_automata> &checkpoint) {
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                checkpoint.automata_states[y][x] = automata_states[y][x];
        checkpoint.rgen = rgen;
    }

    void
    resumeCheckpoint(TsetlinMachineCheckpoint<TsetlinAutomaton, num_clauses,
                                              num_automata> &checkpoint) {
        rgen = checkpoint.rgen;
        for (size_t y = 0; y < num_clauses; y++)
            for (size_t x = 0; x < num_automata; x++)
                automata_states[y][x] = checkpoint.automata_states[y][x];
    }
};

#endif  // TSETLIN_MACHINE_INCLUDE