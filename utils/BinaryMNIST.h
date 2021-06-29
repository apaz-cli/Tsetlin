extern "C" {
#include "../MNIST-dataloader-for-C/mnist.h"
}

#include "TsetlinBitset.h"

// For convernience in creating a unique_ptr of this class
#include <memory>

struct BinaryMNIST {
    static constexpr size_t num_train = 60000;
    static constexpr size_t num_test = 10000;
    TBitset<MNIST_IMG_SIZE> train_60k[num_train];
    TBitset<MNIST_IMG_SIZE> test_10k[num_test];
    unsigned char* label_60k;
    unsigned char* label_10k;

    BinaryMNIST(char data_folder[], float threshold = .3)
        : train_60k(), test_10k() {
        // Load the original dataset.
        MNIST mnist;
        MNIST_load(&mnist, data_folder);

        // Convert and store the images
        auto convert = [&](unsigned char* img, TBitset<MNIST_IMG_SIZE>* bref) {
            for (size_t j = 0; j < MNIST_IMG_SIZE; j++)
                (*bref)[j] = (img[j] > (255 * threshold));
        };
        for (size_t i = 0; i < num_train; i++)
            convert(MNIST_train_img(&mnist, i), this->train_60k + i);
        for (size_t i = 0; i < num_test; i++)
            convert(MNIST_test_img(&mnist, i), this->test_10k + i);

        // Yoink the label references and prevent them from being freed by
        // setting them to null, we just yoinked them.
        this->label_60k = mnist.label_60k;
        this->label_10k = mnist.label_10k;
        mnist.label_60k = NULL;
        mnist.label_10k = NULL;

        // Free the original dataset.
        MNIST_destroy(&mnist);
    }
    ~BinaryMNIST() {
        free(label_60k);
        free(label_10k);
    }

    TBitset<MNIST_IMG_SIZE>&
    train(size_t i) {
        return train_60k[i];
    }
    TBitset<MNIST_IMG_SIZE>&
    test(size_t i) {
        return test_10k[i];
    }
    unsigned char
    train_label(size_t i) {
        return label_60k[i];
    }
    unsigned char
    test_label(size_t i) {
        return label_10k[i];
    }
};