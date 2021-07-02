

#ifndef MULTICLASS_TSETLIN_MACHINE_INCLUDE
#define MULTICLASS_TSETLIN_MACHINE_INCLUDE

#include <cstddef>

#include "TsetlinMachine.h"

#ifndef ARGMAX_TIEBREAK
#define AM_LAST 0
#define AM_FIRST 1
#define AM_RANDOM 2
#define ARGMAX_TIEBREAK AM_LAST
#endif  // ARGMAX_TIEBREAK

template <size_t num_classes, typename config>
class MultiClassTsetlinMachine {
   private:
    static constexpr size_t input_bits = config::input_bits;

    TsetlinMachine<config> machines[num_classes];
    bool machine_outputs[num_classes];
    bool machine_finished[num_classes];

   public:
    void
    spin_threads_forward() {
        auto do_work = [&](TBitset<input_bits> &input, size_t i) {

        };

        
    }

    void
    spin_threads_forward_backward() {
        auto do_work = [&](TBitset<input_bits> &input, size_t i) {

        };
    }

    size_t
    forward(TBitset<input_bits> &input) {}

    void
    backward() {}

    size_t
    forward_backward(TBitset<input_bits> &input) {}
};

#endif