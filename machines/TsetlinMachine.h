#ifndef TSETLIN_MACHINE_INCLUDE
#define TSETLIN_MACHINE_INCLUDE

#include <bits/stdint-uintn.h>

#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <ostream>

// AES intrinsics

#include <limits>
#include <sstream>

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
    // TODO Assert that num_states/2 fits in TsetlinAutomaton

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

    // 0 if positive, 1 if negative.
    static inline bool
    clause_polarity(size_t clause_num) {
        return clause_num < clauses_per_polarity;
    }

    /////////////
    // Methods //
    /////////////

   public:
    void
    print_clauses() {
        for (size_t i = 0; i < num_clauses; i++) {
            TsetlinAutomaton *aut = automataForClause(i);
            std::cout << "Clause: " << i
                      << (clause_polarity(i) ? " (Positive)" : " (Negative)")
                      << '\n';
            for (size_t j = 0; j < automata_per_clause; j++)
                std::cout << (int)aut[j] << ' ';
            std::cout << '\n';
        }
        std::cout << std::endl;
    }

    void
    print_clause(size_t n, size_t width, size_t height) {
        char cl_str[input_bits + 1];
        cl_str[input_bits] = '\0';

        TsetlinAutomaton *cl = automataForClause(n);
        for (size_t i = 0; i < input_bits; i++) {
            bool inc = eval_automaton(cl[i]);
            bool conj_inc = eval_automaton(cl[i + input_bits]);
            if (inc && conj_inc)
                cl_str[i] = 'X';
            else if (inc)
                cl_str[i] = '1';
            else if (conj_inc)
                cl_str[i] = '0';
            else
                cl_str[i] = '*';
        }

        std::cout << '\n';
        for (size_t i = 0; i < height; i++) {
            for (size_t j = 0; j < width; j++) {
                std::cout << cl_str[(width * i) + j];
            }
            std::cout << '\n';
        }
        std::cout << std::endl;
    }

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

        static constexpr tint (*pack_fn)(tint, size_t) = [](tint prev,
                                                            size_t num_times) {
            if (num_times == 0) return prev;

            prev <<= 1;
            prev |= 1;
            return pack_fn(prev, num_times - 1);
        };

        // For each bit, compute the following truth table.
        //            inp
        //          1     0
        //       X-----X-----X
        //     1 X  0  |  1  X
        // inc   X-----X-----X
        //     0 X  0  |  0  X
        //       X-----X-----X

        // Get the bits
        tint *input_b = input.buf;

        tint ret = 0;
        for (size_t inp_idx = 0, aut_idx = 0; inp_idx < cover; inp_idx += 1) {
            // Each loop iterates:
            // TINT_BIT_NUM (1 tint) into input_b (inp_idx), and also
            // 2 * TINT_BIT_NUM into automata_states (aut_idx).

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

            tint result1 = (input_conj & pos_aut);
            tint result2 = (input_buf & conj_aut);

            ret |= result1;
            ret |= result2;
        }

        // We just went two at a time. Now if there's a last one, we have to do
        // it.
        if constexpr (not_covering) {
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

            ret |= result1;
            ret |= result2;

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
                      << "\n\n";

            std::cout << "ret:        " << std::bitset<TINT_BIT_NUM>(result2)
                      << '\n'
                      << std::endl;
                      */
        }

        // Compute the table for the inp, and also for ~inp and its
        // corresponding automata.
        return ret ? 1 : 0;
    }

    void
    clauses_forward(TBitset<input_bits> &input, TBitset<num_clauses> &output) {
        for (size_t i = 0; i < num_clauses; i++)
            output[i] = clause_forward(automataForClause(i), input);
    }

    static inline int
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
                sum += (backing[middle_idx] & ((size_t)1 << j)) ? 1 : 0;
            for (size_t j = split_idx; j < TINT_BIT_NUM; j++)
                sum -= (backing[middle_idx] & ((size_t)1 << j)) ? 1 : 0;

            for (size_t i = r2_begin; i < r2_end; i++)
                sum -= std::popcount<tint>(backing[i]);

            return sum;
        }
    }

    void
    summation_test() {
        TBitset<num_clauses> test_outputs;

        for (size_t its = 0; its < 10000; its++) {
            int sum = 0;
            for (size_t i = 0; i < num_clauses / 2; i++) {
                bool b = rgen.rand_64() % 2;
                test_outputs[i] = b;
                sum += b;
            }
            for (size_t i = num_clauses / 2; i < num_clauses; i++) {
                bool b = rgen.rand_64() % 2;
                test_outputs[i] = b;
                sum -= b;
            }

            int our_sum = summation_forward(test_outputs);
            if (sum != our_sum) {
                std::cout << "Test outputs: " << test_outputs << std::endl;
                std::cout << "Actual sum: " << sum << std::endl;
                std::cout << "Our sum: " << our_sum << std::endl;
            }

            // for (size_t i = 0; i < test_outputs.backing_size(); i++)
            //    test_outputs.buf[i] = rgen.rand_64();
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

    TsetlinAutomaton
    saturated_add(TsetlinAutomaton a, TsetlinAutomaton b) {
        if constexpr (TA_min == std::numeric_limits<TsetlinAutomaton>::min() &&
                      TA_max == std::numeric_limits<TsetlinAutomaton>::max()) {
            // The same as below but branchless in a specific case
            TsetlinAutomaton c;
            bool over = __builtin_add_overflow(a, b, &c);
            return over ? a < 0 ? TA_min : TA_max : c;
        } else {
            if ((a > 0) & (b > TA_max - a))
                return TA_max;
            else if ((a < 0) & (b < TA_min - a))
                return TA_min;
        }
        return a + b;
    }

    static inline float
    t1feedback_table(bool clause, bool literal, bool include) {
        static constexpr float type1rewards[] = {(1 / S), ((S - 1) / S)};

        bool NA = literal | !clause | !include;
        float reward_or_penalty_probability =
            type1rewards[(clause && literal)] * NA;
        return reward_or_penalty_probability;
    }

    static inline TsetlinAutomaton
    t1feedback_is_penalty(bool clause, bool literal, bool include) {
        char options[] = {1, -1};
        return options[((clause & literal) ^ include)];
    }

    static inline void
    test_t1_table() {
        // Verify that the table is correct.
        auto f = [](bool a, bool b, bool c) {
            std::cout << "(" << (int)a << ", " << (int)b << ", " << (int)c
                      << ") -> " << t1feedback_table(a, b, c) << " * "
                      << (int)t1feedback_is_penalty(a, b, c) << '\n';
        };

        std::cout << "Type 1 feedback:\n";
        std::cout << "(clause, literal, include):\n";
        f(0, 0, 0);
        f(0, 0, 1);
        f(0, 1, 0);
        f(1, 0, 0);
        f(0, 1, 1);
        f(1, 1, 0);
        f(1, 0, 1);
        f(1, 1, 1);
    }

    inline TsetlinAutomaton
    calc_t1_feedback(TsetlinAutomaton prev_state, bool clause_output,
                     bool literal, bool include) {
        // Sample from the table
        TsetlinAutomaton t1_reward =
            rgen.rand_bernoulli(t1feedback_table(clause_output, literal, include)) *
            t1feedback_is_penalty(clause_output, literal, include);

        // Apply the reward or penalty in the correct direction.
        // If the automaton is positive, a reward adds, and a penalty subtracts.
        // If the automaton is negative, a reward subtracts, and a penalty adds.
        static constexpr TsetlinAutomaton directions[] = {-1, 1};
        t1_reward *= directions[include];

        // Apply the feedback to the current state and return the new state.
        return saturated_add(prev_state, t1_reward);
    }

    inline void
    apply_t1_feedback(TsetlinAutomaton *automata_for_clause,
                      TBitset<input_bits> &input, bool clause_output) {
        // std::cout << "Applying t1 feedback" << std::endl;
        // Calculate and apply feedback
        for (size_t i = 0, j = 0; i < input_bits; i += 1, j += 2) {
            TsetlinAutomaton *current_state_pos = automata_for_clause + j;
            TsetlinAutomaton *current_state_pos_ = current_state_pos + 1;

            TsetlinAutomaton current_state = *current_state_pos;
            TsetlinAutomaton current_state_ = *current_state_pos_;
            bool include = eval_automaton(current_state);
            bool include_ = eval_automaton(current_state_);
            bool literal = input[i];
            bool literal_ = ~input[i];

            // clang-format off
            *current_state_pos = calc_t1_feedback(
                current_state, clause_output, literal, include);
            *current_state_pos_ = calc_t1_feedback(
                current_state_, clause_output, literal_, include_);
            // clang-format on
        }
    }

    static inline bool
    t2feedback_table(bool clause, bool literal, bool include) {
        return (clause && literal && !include);
    }

    inline TsetlinAutomaton
    calc_t2_feedback(TsetlinAutomaton prev_state, bool clause_output,
                     bool literal, bool include) {
        // No need to do any actual random sampling, the probability for
        // feedback is always 1 or 0.
        bool t2_feedback = t2feedback_table(clause_output, literal, include);
        TsetlinAutomaton t2_is_penalty = -1;  // t2 is always penalty
        TsetlinAutomaton t2_reward = t2_feedback * t2_is_penalty;

        // Apply the reward in the correct direction
        t2_reward *= include ? 1 : -1;
        TsetlinAutomaton res = saturated_add(prev_state, t2_reward);

        /*
        std::cout << "clause_out: " << (int)clause_output
                  << " literal: " << (int)literal
                  << " include: " << (int)include << std::endl;
        std::cout << "t2: prev: " << (int)prev_state << " next: " << (int)res
                  << std::endl;
                  */
        return res;
    }

    inline void
    apply_t2_feedback(TsetlinAutomaton *automata_for_clause,
                      TBitset<input_bits> &input, bool clause_output) {
        // std::cout << "Applying t2 feedback" << std::endl;
        // Calculate and apply feedback
        for (size_t i = 0, j = 0; i < input_bits; i += 1, j += 2) {
            TsetlinAutomaton *current_state_pos = automata_for_clause + j;
            TsetlinAutomaton *current_state_pos_ = current_state_pos + 1;

            TsetlinAutomaton current_state = *current_state_pos;
            TsetlinAutomaton current_state_ = *current_state_pos_;
            bool include = eval_automaton(current_state);
            bool include_ = eval_automaton(current_state_);
            bool literal = input[i];
            bool literal_ = ~input[i];

            // clang-format off
            *current_state_pos = calc_t2_feedback(
                current_state, clause_output, literal, include);
            *current_state_pos_ = calc_t2_feedback(
                current_state_, clause_output, literal_, include_);
            // clang-format on
        }
    }

    static int
    clip(int x) {
        constexpr int iT = (int)summation_target;
        return std::max(-iT, std::min(x, iT));
    }

    void
    backward(TBitset<input_bits> &input, bool desired_output,
             TBitset<num_clauses> clause_outputs, int sum) {
        // std::cout << "Backwards: (" << input << ", " << desired_output
        //          << ")\nclauses: (" << clause_outputs << ", " << sum << ")"
        //          << std::endl;

        // Calculate probability of feedback
        static constexpr size_t _T = summation_target;
        static constexpr float _2T = 2.0 * _T;

        float feedback_prob[2] = {((_T - clip(sum)) / _2T),   // eq. 3
                                  ((_T + clip(sum)) / _2T)};  // eq. 4

        // For each clause
        for (size_t cl_num = 0; cl_num < num_clauses; cl_num++) {
            bool clause_out = clause_outputs[cl_num];
            TsetlinAutomaton *automata_for_clause = automataForClause(cl_num);

            // For each automaton, sample chance to apply t1 or t2 feedback
            bool polarity = clause_polarity(cl_num);
            float satisfy = feedback_prob[desired_output];
            bool satisfied = rgen.rand_bernoulli(satisfy);

            // std::cout << "Updating clause: " << cl_num << " ("
            //          << (int)clause_out
            //          << "), (polarity: " << ((polarity) ? '+' : '-') << "),
            //          ";

            // C^1 = polarity 0 is positive, C^0 = polarity 1 is negative.

            if (!(polarity ^ desired_output)) {
                // std::cout << "Applying t1 feedback: " << satisfied << '\n';
                if (satisfied)
                    apply_t1_feedback(automata_for_clause, input, clause_out);

            } else {
                // std::cout << "Applying t2 feedback: " << satisfied << '\n';
                if (satisfied)
                    apply_t2_feedback(automata_for_clause, input, clause_out);
            }
        }
    }

    bool
    forward_backward(TBitset<input_bits> &input, bool desired_output) {
        /////////////
        // Forward //
        /////////////

        // Clauses forward
        TBitset<num_clauses> clause_outputs;
        clauses_forward(input, clause_outputs);

        // std::cout << '\n' << clause_outputs << '\n';

        // Summation forward
        int sum = summation_forward(clause_outputs);

        // Threshold
        bool output = threshold_forward(sum);

        // Update TM teams
        backward(input, desired_output, clause_outputs, sum);

        return output;
    }

    /////////////
    // UTILITY //
    /////////////
};

#endif  // TSETLIN_MACHINE_INCLUDE