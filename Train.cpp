#include <memory>

#include "BinaryMNIST.h"

#define EPOCHS 400

int
main() {
    char folder[] = "MNIST-dataloader-for-C/data/";

    auto bmnist = std::unique_ptr<BinaryMNIST<>>(new BinaryMNIST<>(folder));
    
    for (size_t epoch = 0; epoch < EPOCHS; i++) {
        
    }

}