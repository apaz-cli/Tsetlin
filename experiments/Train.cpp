#include <cmath>
#include <memory>
#include <ostream>

#include "../machines/MultiClassTsetlinMachine.h"
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

static inline void
print_train(size_t current_train, size_t num_train, size_t num_accurate) {
    std::cout << '\r' << "Progress: " << current_train << '/' << num_train
              << ", " << '(' << std::floor(num_accurate / (float)current_train)
              << " accuracy in this batch." << std::endl;
    std::flush(std::cout);
}

int
main() {
    char folder[] = "../MNIST-dataloader-for-C/data/";
    auto bmnist = std::unique_ptr<BinaryMNIST>(new BinaryMNIST(folder));
    std::cout << "Loaded dataset." << std::endl;
    std::flush(std::cout);

    TsetlinMachine<TsetlinConfig>* model = new TsetlinMachine<TsetlinConfig>();
    std::cout << "Initialized model." << std::endl;
    std::flush(std::cout);

    for (size_t epoch = 0; epoch < EPOCHS; epoch++) {
        size_t num_accurate = 0;

        // Train loop
        for (size_t i = 0; i < bmnist->num_train; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->train_60k[i];
            unsigned char label = bmnist->label_60k[i];
            bool output = model->ctm_forward_backward(img, label);

            num_accurate += output == label ? 1 : 0;

            print_train(i, bmnist->num_train, num_accurate);
        }

        // Valid loop
        uint correct = 0;
        for (size_t i = 0; i < bmnist->num_test; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->test_10k[i];
            unsigned char label = bmnist->label_10k[i];
            bool res = model->ctm_forward(img);
            if (label == res) correct++;
        }
    }
}