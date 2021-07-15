#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <ostream>

// #include "../machines/MultiClassTsetlinMachine.h"
#include "../machines/TsetlinMachine.h"
#include "../utils/BinaryMNIST.h"
#include "../utils/TsetlinBitset.h"

static constexpr bool progress_print = true;
static constexpr bool epoch_print = true;

#define EPOCHS 400
#define NUM_TRAIN 60000
#define NUM_TEST 10000

// MultiClassTsetlinMachine<MNIST_IMG_SIZE, 40000, 50> model;

class MNISTTsetlinConfig {
   public:
    static constexpr size_t input_bits = MNIST_IMG_SIZE;
    static constexpr size_t num_clauses = 40000;
    static constexpr size_t summation_target = 50;
    static constexpr float S = 10.0;

    static char TsetlinAutomaton;
    static constexpr size_t num_states = 256;
};

class XORTsetlinConfig {
   public:
    static constexpr size_t input_bits = 2;
    static constexpr size_t num_clauses = 4;
    static constexpr size_t summation_target = 2;
    static constexpr float S = 3.0;
    static constexpr size_t num_states = 256;
    // Any signed integer type not larger than long long.
    static char TsetlinAutomaton;
};

static inline void
print_progress(const char* train_test, size_t sample_num, size_t num_train,
               size_t num_accurate) {
    if (progress_print) {
        printf(
            "\r%s epoch progress: %zu/%zu, epoch accuracy: %zu/%zu, (%.2f%%)",
            train_test, sample_num, num_train, num_accurate, sample_num,
            100.0 * (float)num_accurate / (float)sample_num);
        fflush(stdout);
    }
}

static inline void
print_epoch(size_t epoch_num, size_t train_accurate, size_t valid_accurate,
            size_t num_train, size_t num_valid, auto epoch_start,
            auto train_end, auto valid_end) {
    if (epoch_print) {
        // clang-format off
        char tn[] = "seconds";
        using timelength = std::chrono::seconds;
        auto train_duration = std::chrono::duration_cast<timelength>(train_end - epoch_start).count();
        auto valid_duration = std::chrono::duration_cast<timelength>(valid_end - train_end).count();
        auto total_duration = std::chrono::duration_cast<timelength>(valid_end - epoch_start).count();

        std::cout << "\r\33[2K"  // Erase progress line
                  << "============================="
                  << "\nFinished epoch " << epoch_num << '.'
                  << "\nTrain Accuracy: " << train_accurate << '/' << num_train << " (" << 100.0 * (float)train_accurate / (float)num_train << "%)"
                  << "\nValid Accuracy: " << valid_accurate << '/' << num_valid << " (" << 100.0 * (float)valid_accurate / (float)num_valid << "%)"
                  << "\nTrain time: " << train_duration << " " << tn << "."
                  << "\nValid time: " << valid_duration << " " << tn << "."
                  << "\nTotal time: " << total_duration << " " << tn << ".\n" << std::endl;
        // clang-format on
    }
}

void
mnist() {
    // Dataset
    char folder[] = "../utils/MNIST-dataloader-for-C/data/";
    auto bmnist = std::unique_ptr<BinaryMNIST>(new BinaryMNIST(folder));

    // Model
    TsetlinMachine<MNISTTsetlinConfig>* model =
        new TsetlinMachine<MNISTTsetlinConfig>();

    for (size_t epoch = 0; epoch < EPOCHS; epoch++) {
        auto epoch_start_time = std::chrono::high_resolution_clock::now();

        // Train loop
        size_t train_accurate = 0;
        for (size_t i = 0; i < bmnist->num_train; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->train(i);
            unsigned char label = bmnist->train_label(i);

            bool output = model->forward_backward(img, label%2 ? 1 : 0);

            train_accurate += output == label%2 ? 1 : 0;

            print_progress("Train", i, bmnist->num_train, train_accurate);
            model->print_clause(0, 28, 28);
        }

        auto train_end_time = std::chrono::high_resolution_clock::now();

        // Valid loop
        size_t valid_accurate = 0;
        for (size_t i = 0; i < bmnist->num_test; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->test(i);
            unsigned char label = bmnist->test_label(i);
            bool output = model->forward(img);

            valid_accurate += output == label%2 ? 1 : 0;

            print_progress("Test", i, bmnist->num_test, valid_accurate);
        }

        auto test_end_time = std::chrono::high_resolution_clock::now();

        // model->print_clauses();

        // Print epoch results
        print_epoch(epoch, train_accurate, valid_accurate, bmnist->num_train,
                    bmnist->num_test, epoch_start_time, train_end_time,
                    test_end_time);
    }
}

void
xor_() {
    TsetlinMachine<XORTsetlinConfig>* model =
        new TsetlinMachine<XORTsetlinConfig>();

    // clang-format off
    auto _11 = TBitset<2>();
         _11[0] = 1;
         _11[1] = 1;
    auto _01 = TBitset<2>();
         _01[0] = 0;
         _01[1] = 1;
    auto _10 = TBitset<2>();
         _10[0] = 1;
         _10[1] = 0;
    auto _00 = TBitset<2>();
         _00[0] = 0;
         _00[1] = 0;
    
    /*
    model->forward_backward(_11, 0);
    model->forward_backward(_01, 1);
    model->forward_backward(_10, 1);
    model->forward_backward(_00, 0);
    */

    // clang-format on
    size_t num_accurate = 0;
    size_t num_total = 0;
    size_t current_it = 0;
    while (true) {
        current_it++;

        bool a = model->forward_backward(_11, 0);
        num_accurate += (a == 0);
        num_total++;

        bool b = model->forward_backward(_01, 1);
        num_accurate += (b == 1);
        num_total++;

        bool c = model->forward_backward(_10, 1);
        num_accurate += (c == 1);
        num_total++;

        bool d = model->forward_backward(_00, 0);
        num_accurate += (d == 0);
        num_total++;

        std::cout << "\r" << (int)a << (int)b << (int)c << (int)d << " ("
                  << num_accurate << '/' << num_total << ") "
                  << (num_accurate / (float)num_total) << std::endl;

        // model->print_clauses();
    }
}

int
main() {
    mnist();
    // xor_();
    // TsetlinMachine<XORTsetlinConfig>().summation_test();
}
