#include <memory>

#include "../utils/BinaryMNIST.h"
#include "MultiClassTsetlinMachine.h"
#include "TsetlinBitset.h"
#include "TsetlinMachine.h"

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
    static char TsetlinAutomaton;
};

int
main() {
    char folder[] = "MNIST-dataloader-for-C/data/";
    auto bmnist = std::unique_ptr<BinaryMNIST>(new BinaryMNIST(folder));
    std::cout << "Loaded dataset." << std::endl;

    TsetlinMachine<TsetlinConfig>* model = new TsetlinMachine<TsetlinConfig>();
    std::cout << "Initialized model." << std::endl;

    for (size_t epoch = 0; epoch < EPOCHS; epoch++) {
        // Train loop
        for (size_t i = 0; i < 60000; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->train_60k[i];
            unsigned char label = bmnist->label_60k[i];
            model->forward_backward(img, label);
        }

        // Valid loop
        uint correct = 0;
        for (size_t i = 0; i < 10000; i++) {
            TBitset<MNIST_IMG_SIZE>& img = bmnist->test_10k[i];
            unsigned char label = bmnist->label_10k[i];
            bool res = model->forward(img);
            if (label == res) correct++;
        }
        
    }
}