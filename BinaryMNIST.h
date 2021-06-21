
extern "C" {
#include "MNIST-dataloader-for-C/mnist.h"
}
#include "TsetlinBitset.h"
#include "TsetlinMachine.h"

#define MNIST_AS_TINT_SIZE ((MNIST_IMG_SIZE / TINT_BIT_NUM) + 1)

template <size_t img_size = MNIST_IMG_SIZE>
struct BinaryMNIST {
    TBitset<img_size> train_60k[60000];
    TBitset<img_size> test_10k[10000];
    unsigned char* label_60k;
    unsigned char* label_10k;

    BinaryMNIST(char data_folder[], float threshold = .3)
        : train_60k(), test_10k() {
        MNIST mnist;
        MNIST_load(&mnist, data_folder);

        // Convert and store the images
        auto convert = [&](unsigned char* img, TBitset<img_size>* bref) {
            for (size_t j = 0; j < MNIST_IMG_SIZE; j++) {
                bref[j] = (img[j] > (255 * threshold));
            }
        };

        for (size_t i = 0; i < 60000; i++) {
            unsigned char* img = MNIST_train_img(&mnist, i);
            TBitset<img_size>* bref = train_60k + i;
            convert(img, bref);
        }

        for (size_t i = 0; i < 10000; i++) {
            unsigned char* img = MNIST_test_img(&mnist, i);
            TBitset<img_size>* bref = test_10k + i;
            convert(img, bref);
        }

        // Yoink the label references
        this->label_60k = mnist.label_60k;
        this->label_10k = mnist.label_10k;

        // Prevent these two references from being freed,
        // because we just yoinked them.
        mnist.label_60k = NULL;
        mnist.label_10k = NULL;
        MNIST_destroy(&mnist);
    }
    ~BinaryMNIST() {
        free(label_60k);
        free(label_10k);
    }
};