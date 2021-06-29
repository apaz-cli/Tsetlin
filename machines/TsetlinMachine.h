#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cstddef>
#include <iostream>
#include <limits>
#include <ostream>
#include <random>

#include "../utils/NDArray.h"
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
    static constexpr TsetlinAutomaton TA_max = (num_states / 2) - 1;
    static constexpr TsetlinAutomaton TA_min = -(num_states / 2);

    // Assert parameters
    static_assert(!(num_clauses % 2),
                  "The number of clauses must be divisible by 2.");
    static_assert(!(num_states % 2),
                  "The number of states must be divisible by 2.");
    static_assert(
        std::numeric_limits<TsetlinAutomaton>::min() <= TA_min,
        "The number of states in a Tsetlin machine must fall within limits.");
    static_assert(
        std::numeric_limits<TsetlinAutomaton>::max() <= TA_max,
        "The number of states in a Tsetlin machine must fall within limits.");
    static_assert(num_clauses, "The number of clauses must not be zero.");
    static_assert(input_bits, "The number of input bits must not be zero.");

    ///////////
    // State //
    ///////////

    // Random generator. Seeded in constructor.
    TsetlinRandGen rgen;

    // Clause Layout:
    // pos, neg, pos, neg, ... (polarity = i % 2 ? 'pos', 'neg')
    // Automaton Layout in each clause:
    // inp, ~inp, inp, ~inp, ... (Where each bundle of these is TINT_SIZE long.)
    static constexpr size_t automata_states_len =
        num_clauses * automata_per_clause;
    TsetlinAutomaton automata_states[automata_states_len];

    inline TsetlinAutomaton *
    automataForClause(size_t clause_num) {
        return automata_states + (automata_per_clause * clause_num);
    }

    /////////////
    // Methods //
    /////////////

   public:
    TsetlinMachine(uint64_t seed = 0) : rgen(TsetlinRandGen(seed)) {
        for (size_t x = 0; x < automata_states_len; x++)
            automata_states[x] = -(rgen.rand() & 1);
    }

    bool
    clause_forward(TsetlinAutomaton *clause_automata,
                   TBitset<input_bits> &input) const noexcept {
        // Evaluate the automata and pack into bitset.
        // The results are an "inclusion mask" for the clause.
        TBitset<automata_per_clause> automata_results;
        for (size_t i = 0; i < automata_per_clause; i++)
            automata_results[i] = (clause_automata[i] >= 0) ? 1 : 0;

        // Get the bits
        const std::array<tint, (input_bits / TINT_BIT_NUM) +
                                   ((input_bits % TINT_BIT_NUM) != 0)>
            &input_b = input.get_backing();
        const std::array<tint, (automata_per_clause / TINT_BIT_NUM) +
                                   ((automata_per_clause % TINT_BIT_NUM) != 0)>
            &inc_b = automata_results.get_backing();

        //            inp
        //          1     0
        //       X-----X-----X
        //     1 X  1  |  0  X
        // inc   X-----X-----X
        //     0 X  1  |  1  X
        //       X-----X-----X

        // Compute the table for the inp, and also for ~inp and its
        // corresponding automata.

        tint ret = TINT_MAX;  // 32 or 64 1s, depending on archetecture.
        for (size_t i = 0, j = 0; i < input.backing_size(); i++, j++) {
            tint inc_pos = inc_b[j];
            tint inc_neg = inc_b[++j];
            tint inp_pos = input_b[i];
            tint inp_neg = ~input_b[i];
            tint table_inp = (~inc_pos | inp_pos);
            tint table_inp_conj = (~inc_neg | inp_neg);
            ret &= table_inp & table_inp_conj;
        }

        // If a 0 never appeared in the clause, return 1. Otherwise, return 0.
        // I forget if returning the comparison directly is undefined behavior
        // or not, so I'mma just play it safe and it'll get optimized away.
        return (ret == TINT_MAX) ? 1 : 0;
    }

    void
    clauses_forward(TBitset<input_bits> &input, TBitset<num_clauses> &output) {
        for (size_t i = 0; i < num_clauses; i++)
            output[i] = clause_forward(automataForClause(i), input);
    }

    static int
    summation_forward(TBitset<num_clauses> &clause_outputs) {
        static constexpr size_t size = num_clauses;
        static constexpr size_t halfway = size / 2;  // num_clauses % 2 == 0
        static constexpr size_t backing_size =
            (num_clauses / TINT_BIT_NUM) + ((num_clauses % TINT_BIT_NUM) != 0);
        static constexpr size_t middle_idx = backing_size / 2;

        static constexpr size_t split_idx = halfway % TINT_BIT_NUM;
        static constexpr bool clean =
            (backing_size % 2 == 0) && (split_idx == 0);

        static constexpr size_t r1_begin = 0;
        static constexpr size_t r1_end = middle_idx;
        static constexpr size_t r2_begin = middle_idx + !clean;
        static constexpr size_t r2_end = backing_size;

        const std::array<tint, backing_size> &backing =
            clause_outputs.get_backing();
        int sum = 0;

        if constexpr (clean) {
            // Case: The boundary between positive and negative clause outputs
            // is the boundary of a buffer.
            for (size_t i = r1_begin; i < r1_end; i++)
                sum += std::popcount<size_t>(backing[i]);
            for (size_t i = r2_begin; i < r2_end; i++)
                sum -= std::popcount<size_t>(backing[i]);
            return sum;
        } else {
            // Case: The boundary between positive and negative clause outputs
            // is inside a buffer.
            int sum = 0;
            for (size_t i = r1_begin; i < r1_end; i++)
                sum += std::popcount<size_t>(backing[i]);

            // Deal with the middle
            for (size_t j = 0; j < split_idx; j++)
                sum += (backing[middle_idx] & (1 << j)) ? 1 : 0;
            for (size_t j = 0; j < split_idx; j++)
                sum -= (backing[middle_idx] & (1 << j)) ? 1 : 0;

            for (size_t i = r2_begin; i < r2_end; i++)
                sum -= std::popcount<size_t>(backing[i]);

            return sum;
        }
    }

    static bool
    threshold_forward(int sum) {
        return sum >= 0;
    }

    bool
    ctm_forward(TBitset<input_bits> &input) {
        // Clauses forward
        TBitset<num_clauses> clause_outputs;
        clauses_forward(input, clause_outputs);

        // Summation forward
        int sum = summation_forward(clause_outputs);

        // Threshold
        return threshold_forward(sum);
    }

    bool
    operator()(TBitset<input_bits> &input) {
        return ctm_forward(input);
    }

    ///////////////
    // Backwards //
    ///////////////

    static int
    clip(int x) {
        constexpr int iT = (int)summation_target;
        return std::max(-iT, std::min(x, iT));
    }

    static TsetlinAutomaton
    bound_TA(long long x) {
        return std::max<TsetlinAutomaton>(
            TA_min, std::min<TsetlinAutomaton>(x, TA_max));
    }

    static float
    t1feedback_table(bool clause, bool literal, bool include) {
        static constexpr float type1rewards[] = {((S - 1) / S), (1 / S)};

        bool NA = !(clause && !literal && include);
        return type1rewards[(clause && literal)] * NA;
    }

    static bool
    t2feedback_table(bool clause, bool literal, bool include) {
        return (clause && literal && !include);
    }

    TsetlinAutomaton
    calculate_feedback(TsetlinAutomaton prev_state, bool t1, bool t2,
                       bool clause_output, bool literal, bool include) {
        long long feedback = 0;
        if (t1) {
            feedback += rgen.rand_bernoulli(
                t1feedback_table(clause_output, literal, include));
        }
        if (t2) {
            feedback += t2feedback_table(clause_output, literal, include);
        }

        return bound_TA(prev_state + feedback);
    }

    void
    ctm_backward(TBitset<input_bits> &input,
                 TBitset<num_clauses> clause_outputs, int sum,
                 bool desired_output) {
        // Calculate probability of feedback
        static constexpr size_t _T = summation_target;
        static constexpr float _2T = 2.0 * _T;
        float type1prob = (_T - clip(sum)) / _2T;
        float type2prob = (_T + clip(sum)) / _2T;

        // For each clause
        for (size_t cl_num = 0; cl_num < num_clauses; cl_num++) {
            bool clause_out = clause_outputs[cl_num];
            TsetlinAutomaton *automata_for_clause = automataForClause(cl_num);

            // For each automaton in the clause, apply t1 and t2 feedback
            bool t1 = rgen.rand_bernoulli(type1prob);
            bool t2 = rgen.rand_bernoulli(type2prob);
            for (size_t i = 0, j = 0; i < input_bits; i += 1, j += 2) {
                // Update the the automata in the clause corresponding to the
                // input.

                // Inputs and their conjugates are updated at the same time.
                // Variables eding in _  correspond to conjugates.
                // clang-format off
                TsetlinAutomaton *current_state_pos = automata_for_clause + j;
                TsetlinAutomaton *current_state_pos_ = automata_for_clause + j + 1;
                TsetlinAutomaton current_state = *current_state_pos;
                TsetlinAutomaton current_state_ = *current_state_pos_;
                // clang-format on

                TsetlinAutomaton new_state =
                    calculate_feedback(current_state, t1, t2, clause_out,
                                       input[i], current_state >= 0 ? 1 : 0);
                TsetlinAutomaton new_state_ =
                    calculate_feedback(current_state_, t1, t2, clause_out,
                                       ~input[i], current_state_ >= 0 ? 1 : 0);

                *current_state_pos = new_state;
                *current_state_pos_ = new_state_;
            }
        }
    }

    bool
    ctm_forward_backward(TBitset<input_bits> &input, bool desired_output) {
        /////////////
        // Forward //
        /////////////

        // Clauses forward
        TBitset<num_clauses> clause_outputs;
        clauses_forward(input, clause_outputs);

        // Summation forward
        int sum = summation_forward(clause_outputs);

        // Threshold
        bool output = threshold_forward(sum);

        // Update TM teams
        ctm_backward(input, clause_outputs, sum, desired_output);

        return output;
    }

    /////////////
    // UTILITY //
    /////////////
};

#endif  // TSETLIN_MACHINE_INCLUDE