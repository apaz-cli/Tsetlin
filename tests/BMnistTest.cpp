#include "../utils/BinaryMNIST.h"

int
main() {
    // char folder[] = "../MNIST-dataloader-for-C/data/";
    // MNIST mnist;
    // MNIST_load(&mnist, folder);
    // unsigned char* img;
    // for (size_t i = 0; i < 10000; i++) {
    //    img = MNIST_test_img(&mnist, i);
    //    unsigned char c = *(img+MNIST_IMG_SIZE-1);
    //    std::cout << (int)c;
    //}
    // MNIST_destroy(&mnist);

    char folder[] = "../MNIST-dataloader-for-C/data/";
    auto bmnist = std::unique_ptr<BinaryMNIST>(new BinaryMNIST(folder));
    std::cout << "Loaded dataset." << std::endl;
    std::flush(std::cout);
}