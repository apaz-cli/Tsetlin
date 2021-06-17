#include "BinaryMNIST.h"

int main() {
    char folder[] = "MNIST-dataloader-for-C/data/";
    MNIST mnist;
    MNIST_load(&mnist, folder);

}