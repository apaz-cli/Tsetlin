#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <random>

#include "../utils/TsetlinBitset.h"
#include "../utils/TsetlinRand.h"

/////////////////////
// Tsetlin Machine //
/////////////////////

template <typename config>
class TsetlinMachine {
   private:
    // Unpack Hyperparameters
    static constexpr size_t input_bits = config::input_bits;
    static constexpr size_t automata_per_clause = input_bits * 2;
    static constexpr size_t num_clauses = config::num_clauses;
    static constexpr size_t clauses_per_polarity = num_clauses / 2;
    static constexpr size_t automata_per_polarity =
        automata_per_clause * clauses_per_polarity;
    static constexpr size_t summation_target = config::summation_target;
    static constexpr float S = config::S;
    static constexpr size_t num_states = config::num_states;
    using TsetlinAutomaton = decltype(config::TsetlinAutomaton);

    // Calculated Parameters
    static_assert(!(num_clauses % 2),
                  "The number of clauses must be divisible by 2.");
    static_assert(num_clauses, "The number of clauses must not be zero.");
    static_assert(input_bits, "The number of input bits must not be zero.");

    ///////////
    // State //
    ///////////

    // TsetlinAutomaton automata_states[num_clauses][num_automata];
    TsetlinRandGen rgen;
    TsetlinAutomaton pos_clause_automata_states[automata_per_polarity];
    TsetlinAutomaton neg_clause_automata_states[automata_per_polarity];

    inline TsetlinAutomaton *
    automataForClause(TsetlinAutomaton *states, size_t clause_num) {
        return states + (automata_per_clause * clause_num);
    }

    ////////////
    // Methods//
    ////////////

   public:
    TsetlinMachine(uint64_t seed = 0)
        : rgen(TsetlinRandGen(seed)) {
        for (size_t x = 0; x < automata_per_polarity; x++)
            pos_clause_automata_states[x] = -(rgen.rand() & 1);
        for (size_t x = 0; x < automata_per_polarity; x++)
            neg_clause_automata_states[x] = -(rgen.rand() & 1);
    }

    void
    literals_forward(TBitset<input_bits> &input,
                     TBitset<input_bits> &input_conjugate) {
        input_conjugate = ~input;
    }

    bool
    clause_forward(
        TsetlinAutomaton *clause_automata, TBitset<input_bits> &input,
        TBitset<input_bits> &input_conjugate,
        TBitset<automata_per_clause> &automata_results) const noexcept {
        // Evaluate the automata and pack into bitset.
        // The results are an "inclusion mask" for our input vectors.
        for (size_t i = 0; i < automata_per_clause; i++)
            automata_results[i] = (clause_automata[i] >= 0) ? 1 : 0;

        // Compute the following truth table, TINT_BIT_NUM bits at a time.
        // This is faster than using the builtin operators.
        // clang-format off
        std::array<tint, input_bits>& input_b = input.get_backing();
        std::array<tint, input_bits>& input_conj_b = input_conjugate.get_backing();
        std::array<tint, automata_per_polarity>& inc_b = automata_results.get_backing();
        // clang-format on

        //            inp
        //          1     0
        //       X-----X-----X
        //     1 X  1  |  0  X
        // inc   X-----X-----X
        //     0 X  1  |  1  X
        //       X-----X-----X

        tint ret = TINT_MAX;  // 32 or 64 1s
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

        return (ret != TINT_MAX) ? 0 : 1;
    }

    void
    clauses_forward(TBitset<input_bits> &input,
                    TBitset<input_bits> &input_conjugate,
                    TBitset<clauses_per_polarity> &pos_output,
                    TBitset<clauses_per_polarity> &neg_output) {
        TBitset<automata_per_clause> automata_results;
        for (size_t i = 0; i < clauses_per_polarity; i++)
            pos_output[i] =
                clause_forward(automataForClause(pos_clause_automata_states, i),
                               input, input_conjugate, automata_results);
        for (size_t i = 0; i < clauses_per_polarity; i++)
            neg_output[i] =
                clause_forward(automataForClause(neg_clause_automata_states, i),
                               input, input_conjugate, automata_results);
    }

    static int
    summation_forward(TBitset<clauses_per_polarity> &pos_clause_outputs,
                      TBitset<clauses_per_polarity> &neg_clause_outputs) {
        return pos_clause_outputs.count() - neg_clause_outputs.count();
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
        TBitset<clauses_per_polarity> pos_clause_outputs;
        TBitset<clauses_per_polarity> neg_clause_outputs;
        clauses_forward(input, input_conjugate, pos_clause_outputs,
                        neg_clause_outputs);

        // Summation forward
        int sum = summation_forward(pos_clause_outputs, neg_clause_outputs);

        // Threshold
        bool output = threshold_forward(sum);

        //////////////
        // Backward //
        //////////////

        TsetlinAutomaton feedback[num_clauses][automata_per_clause];
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

    /////////////
    // UTILITY //
    /////////////
};

#endif  // TSETLIN_MACHINE_INCLUDE