#include <stdio.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <ostream>

// #include "../machines/MultiClassTsetlinMachine.h"
#include "../machines/TsetlinMachine.h"
#include "../utils/BinaryMNIST.h"
#include "../utils/TsetlinBitset.h"

#define EPOCHS 400
#define NUM_TRAIN 60000
#define NUM_TEST 10000

// MultiClassTsetlinMachine<MNIST_IMG_SIZE, 40000, 50> model;

class TsetlinConfig {
   public:
    static constexpr size_t input_bits = MNIST_IMG_SIZE;
    static constexpr size_t num_clauses = 4000;
    static constexpr size_t summation_target = 50;
    static constexpr float S = 10.0;
    static constexpr size_t num_states = 256;
    // Any signed integer type not larger than long long.
    static char TsetlinAutomaton;
};

static constexpr bool progress_print = 0;

static inline void
print_progress(const char* train_test, size_t current, size_t num_train,
               size_t num_accurate) {
    if (progress_print) {
        printf(
            "\r%s epoch progress: %zu/%zu, epoch accuracy: %zu/%zu, (%.2f%%)",
            train_test, current, num_train, num_accurate, current,
            100.0 * (float)num_accurate / (float)current);
        fflush(stdout);
    }
}

static inline void
print_epoch(size_t epoch_num, size_t num_accurate, size_t num_test,
            auto epoch_start, auto train_end, auto valid_end) {
    auto train_duration = std::chrono::duration_cast<std::chrono::minutes>(
                              train_end - epoch_start)
                              .count();
    auto valid_duration =
        std::chrono::duration_cast<std::chrono::minutes>(valid_end - train_end)
            .count();
    auto total_duration = std::chrono::duration_cast<std::chrono::minutes>(
                              valid_end - epoch_start)
                              .count();

    std::cout << "\r============================="
              << "\nFinished epoch " << epoch_num << '.'
              << "\nAccuracy: " << num_accurate << '/' << num_test << " ("
              << 100.0 * (float)num_accurate / (float)num_test << "%)"
              << "\nTrain time: " << train_duration << " minutes."
              << "\nValidation time: " << valid_duration << " minutes."
              << "\nTotal time: " << total_duration << " minutes.\n" << std::endl;
}

int
main() {
    // Dataset
    char folder[] = "../utils/MNIST-dataloader-for-C/data/";
    auto bmnist = std::unique_ptr<BinaryMNIST>(new BinaryMNIST(folder));

    // Model
    TsetlinMachine<TsetlinConfig>* model = new TsetlinMachine<TsetlinConfig>();

    for (size_t epoch = 0; epoch < EPOCHS; epoch++) {
        auto epoch_start_time = std::chrono::high_resolution_clock::now();

        // Train loop
    size_t num_accurate = 0;
        for (size_t i = 0; i < bmnist->num_train; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->train(i);
            unsigned char label = bmnist->train_label(i);

            bool output = model->forward_backward(img);

            num_accurate += output == (bool)label ? 1 : 0;

            print_progress("Train", i, bmnist->num_train, num_accurate);
        }

        auto train_end_time = std::chrono::high_resolution_clock::now();

        // Valid loop
        num_accurate = 0;
        for (size_t i = 0; i < bmnist->num_test; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->test(i);
            unsigned char label = bmnist->test_label(i);
            bool output = model->forward(img);

            num_accurate += output == (bool)label ? 1 : 0;

            print_progress("Test", i, bmnist->num_test, num_accurate);
        }

        auto test_end_time = std::chrono::high_resolution_clock::now();

        // Print epoch results
        print_epoch(epoch, num_accurate, bmnist->num_test, epoch_start_time,
                    train_end_time, test_end_time);
    }
}