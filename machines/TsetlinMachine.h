#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

#include <bits/stdint-uintn.h>

#include <bitset>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <ostream>

// AES intrinsics

#include "../utils/Benchmarker.h"
#include "../utils/Morton.h"
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
    // pos..., neg... (polarity = clause_idx < clauses_per_polarity)
    // Automaton Layout in each clause:
    // inp, ~inp, inp, ~inp, ... (bits alternate, mask with 0x)

    static constexpr size_t automata_states_len =
        num_clauses * automata_per_clause;
    TsetlinAutomaton automata_states[automata_states_len];

    static inline bool
    eval_automaton(TsetlinAutomaton a) {
        return a >= 0 ? 1 : 0;
    }

    inline TsetlinAutomaton *
    automataForClause(size_t clause_num) {
        return automata_states + (automata_per_clause * clause_num);
    }

    /////////////
    // Methods //
    /////////////

   public:
    TsetlinMachine(uint64_t seed = 0xabcdef0123456789)
        : rgen(TsetlinRandGen(seed)) {
        for (size_t x = 0; x < automata_states_len; x++)
            automata_states[x] = -(TsetlinAutomaton)(rgen.rand_64() % 2);
    }

    bool
    clause_forward(TsetlinAutomaton *clause_automata,
                   TBitset<input_bits> &input) const noexcept {
        static constexpr size_t to_cover =
            (input_bits / TINT_BIT_NUM) + ((input_bits % TINT_BIT_NUM) != 0);
        static constexpr bool not_covering = to_cover % 2;
        static constexpr size_t cover = to_cover - not_covering;
        static constexpr size_t not_covering_begin_idx =
            2 * TINT_BIT_NUM * cover;

        // Get the bits
        tint *input_b = input.buf;

        tint ret = 0;
        for (size_t inp_idx = 0, aut_idx = 0; inp_idx < cover; inp_idx += 1) {
            // Each loop iterates:
            // TINT_BIT_NUM (1 tint) into input_b (inp_idx), and also
            // 2 * TINT_BIT_NUM into automata_states (aut_idx).
            // After we're done,

            // Interleave them
            // tint m1, m2;
            // xy64_to_morton128(input_buf, input_conj, &m1, &m2);

            // Evaluate two tints worth of automata, un-interleaving and packing
            // into a tint. This should get vectorized.
            tint pos_aut = 0, conj_aut = 0;
            for (size_t j = 0; j < TINT_BIT_NUM; j++) {
                pos_aut <<= 1;
                pos_aut |= eval_automaton(clause_automata[aut_idx++]);
                conj_aut <<= 1;
                conj_aut |= eval_automaton(clause_automata[aut_idx++]);
            }

            // Get input and conjugate.
            tint input_buf = input_b[inp_idx];
            tint input_conj = ~input_buf;

            // Compute the inverse of the following truth table.
            //            inp
            //          1     0
            //       X-----X-----X
            //     1 X  1  |  0  X
            // inc   X-----X-----X
            //     0 X  1  |  1  X
            //       X-----X-----X
            tint result1 = (input_conj & pos_aut);
            tint result2 = (input_buf & conj_aut);

            /*
            std::cout << "input_buf:  " << std::bitset<TINT_BIT_NUM>(input_buf)
                      << std::endl;
            std::cout << "pos_aut:    " << std::bitset<TINT_BIT_NUM>(pos_aut)
                      << std::endl;
            std::cout << "result1:    " << std::bitset<TINT_BIT_NUM>(result1)
                      << '\n'
                      << std::endl;

            std::cout << "input_conj: " << std::bitset<TINT_BIT_NUM>(input_conj)
                      << std::endl;
            std::cout << "conj_aut:   " << std::bitset<TINT_BIT_NUM>(conj_aut)
                      << std::endl;
            std::cout << "result2:    " << std::bitset<TINT_BIT_NUM>(result2)
                      << '\n'
                      << std::endl;
                      */

            ret &= result1;
            ret &= result2;
        }

        // We just went two at a time. Now if there's a last one, we have to do
        // it.
        if (not_covering) {
            tint pos_aut = 0, conj_aut = 0;
            for (size_t j = not_covering_begin_idx; j < automata_per_clause;) {
                pos_aut <<= 1;
                pos_aut |= eval_automaton(clause_automata[j++]);
                conj_aut <<= 1;
                conj_aut |= eval_automaton(clause_automata[j++]);
            }

            tint input_buf = input_b[input.buf_len - 1];
            tint input_conj = ~input_buf;

            tint result1 = (input_conj & pos_aut);
            tint result2 = (input_buf & conj_aut);

            ret &= result1;
            ret &= result2;
        }

        // Compute the table for the inp, and also for ~inp and its
        // corresponding automata.
        return ret ? 0 : 1;
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

        tint *backing = clause_outputs.buf;
        int sum = 0;
        if constexpr (clean) {
            // Case: The boundary between positive and negative clause outputs
            // is the boundary of a buffer.
            for (size_t i = r1_begin; i < r1_end; i++)
                sum += std::popcount<tint>(backing[i]);
            for (size_t i = r2_begin; i < r2_end; i++)
                sum -= std::popcount<tint>(backing[i]);
            return sum;
        } else {
            // Case: The boundary between positive and negative clause outputs
            // is inside a buffer.
            for (size_t i = r1_begin; i < r1_end; i++)
                sum += std::popcount<tint>(backing[i]);

            // Deal with the middle
            for (size_t j = 0; j < split_idx; j++)
                sum += (backing[middle_idx] & (1 << j)) ? 1 : 0;
            for (size_t j = split_idx; j < TINT_BIT_NUM; j++)
                sum -= (backing[middle_idx] & (1 << j)) ? 1 : 0;

            for (size_t i = r2_begin; i < r2_end; i++)
                sum -= std::popcount<tint>(backing[i]);

            return sum;
        }
    }

    static bool
    threshold_forward(int sum) {
        return sum >= 0;
    }

    bool
    forward(TBitset<input_bits> &input) {
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
        return forward(input);
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

    float
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
                       bool clause_output, bool literal) {
        bool include = prev_state >= 0 ? 1 : 0;
        long long feedback = 0;

        feedback += t1 * rgen.rand_bernoulli(
                             t1feedback_table(clause_output, literal, include));

        feedback += t2 * t2feedback_table(clause_output, literal, include);

        TsetlinAutomaton ret = bound_TA(prev_state + feedback);

        return ret;
    }

    void
    backward(TBitset<input_bits> &input, TBitset<num_clauses> clause_outputs,
             int sum) {
        // Calculate probability of feedback
        static constexpr size_t _T = summation_target;
        static constexpr float _2T = 2.0 * _T;

        // Note that 1-feedback_prob is the chance
        //
        float feedback_prob = (_T + clip(sum)) / _2T;

        // For each clause
        for (size_t cl_num = 0; cl_num < num_clauses; cl_num++) {

            bool clause_out = clause_outputs[cl_num];
            TsetlinAutomaton *automata_for_clause = automataForClause(cl_num);

            // For each automaton, sample chance to apply t1 or t2 feedback
            bool polarity = cl_num < clauses_per_polarity;
            char samples = rgen.sample_feedback(feedback_prob, polarity);
            bool t1 = samples & (1 << 0) ? 1 : 0;
            bool t2 = samples & (1 << 1) ? 1 : 0;

            for (size_t i = 0, j = 0; i < input_bits; i += 1, j += 2) {
                // Update the the automata in the clause corresponding to the
                // input and conjugate (conj variables end in _).
                TsetlinAutomaton *current_state_pos = automata_for_clause + j;
                TsetlinAutomaton *current_state_pos_ = current_state_pos + 1;
                TsetlinAutomaton current_state = *current_state_pos;
                TsetlinAutomaton current_state_ = *current_state_pos_;

                *current_state_pos = calculate_feedback(current_state, t1, t2,
                                                        clause_out, input[i]);
                *current_state_pos_ = calculate_feedback(current_state_, t1, t2,
                                                         clause_out, ~input[i]);
            }
        }
    }

    bool
    forward_backward(TBitset<input_bits> &input) {
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
        backward(input, clause_outputs, sum);

        return output;
    }

    /////////////
    // UTILITY //
    /////////////
};

#endif  // TSETLIN_MACHINE_INCLUDE