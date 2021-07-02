#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MNIST_IMG_SIZE 784  // 28*28

#ifndef MNIST_BOUNDCHECK
#define MNIST_BOUNDCHECK 1
#endif

static inline int
mnist_reverseInt(int i) {
    unsigned char c1, c2, c3, c4;
    c1 = i & 255;
    c2 = (i >> 8) & 255;
    c3 = (i >> 16) & 255;
    c4 = (i >> 24) & 255;
    return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
}

#define mnist_panic(name, ac, ex)                                            \
    {                                                                        \
        printf("Read %s: %i, which did not match expected: %i.\n", name, ac, \
               ex);                                                          \
        exit(-1);                                                            \
    }

static inline unsigned char *
read_mnist_images(char *file_path, int num_imgs) {
    // Open file
    int fd;
    if ((fd = open(file_path, O_RDONLY)) == -1) {
        fprintf(stderr, "Couldn't open mnist file: %s.", file_path);
        exit(-1);
    }

    // Read header
    int magic_number = 0, n_rows = 0, n_cols = 0, n_imgs = 0;
    read(fd, &magic_number, 4);
    magic_number = mnist_reverseInt(magic_number);
    if (magic_number != 2051) mnist_panic("magic_number", magic_number, 2051);
    read(fd, &n_imgs, 4);
    n_imgs = mnist_reverseInt(n_imgs);
    if (num_imgs != n_imgs) mnist_panic("n_imgs", n_imgs, num_imgs);
    read(fd, &n_rows, 4);
    n_rows = mnist_reverseInt(n_rows);
    if (n_rows != 28) mnist_panic("n_rows", n_rows, 28);
    read(fd, &n_cols, 4);
    n_cols = mnist_reverseInt(n_cols);
    if (n_cols != 28) mnist_panic("n_cols", n_cols, 28);

    // Read all rasters at once
    size_t buf_size = num_imgs * MNIST_IMG_SIZE;
    unsigned char *dataset = (unsigned char *)malloc(buf_size);
    read(fd, dataset, buf_size);

    // Close the file
    if (close(fd) == -1) {
        fprintf(stderr, "Couldn't close mnist file: %s.", file_path);
        exit(-1);
    }
    return dataset;
}

inline unsigned char *
read_mnist_labels(char *file_path, int num_labels) {
    int fd;
    if ((fd = open(file_path, O_RDONLY)) == -1) {
        fprintf(stderr, "Couldn't open mnist file: %s.", file_path);
        exit(-1);
    }

    int magic_number = 0, n_labels;
    read(fd, &magic_number, 4);
    magic_number = mnist_reverseInt(magic_number);
    if (magic_number != 2049) mnist_panic("magic_number", magic_number, 2049);
    read(fd, &n_labels, 4);
    n_labels = mnist_reverseInt(n_labels);
    if (num_labels != n_labels) mnist_panic("n_labels", n_labels, num_labels);

    unsigned char *dataset = (unsigned char *)malloc(num_labels);
    for (int i = 0; i < num_labels; i++) {
        read(fd, dataset + i, 1);
    }
    return dataset;
}

// Use the accessors to get individual images.
struct MNIST {
    unsigned char *train_60k;
    unsigned char *label_60k;
    unsigned char *test_10k;
    unsigned char *label_10k;
};
typedef struct MNIST MNIST;

inline MNIST *
MNIST_load(MNIST *mnist, char *data_folder) {
    char fname[256];
    char f1[] = "train-images.idx3-ubyte";
    char f2[] = "train-labels.idx1-ubyte";
    char f3[] = "t10k-images.idx3-ubyte";
    char f4[] = "t10k-labels.idx1-ubyte";

    // String copy into fname, append '/' if
    // it isn't there, and null terminate.
    // Keep track of the end.
    unsigned int i = 0;
    while (i < 200 && data_folder[i]) {
        fname[i] = data_folder[i];
        i++;
    }
    if (fname[i - 1] != '/') {
        fname[i] = '/';
        i++;
    }

    // Load all the files
    fname[i] = '\0';
    mnist->train_60k = read_mnist_images(strcat(fname, f1), 60000);
    fname[i] = '\0';
    mnist->label_60k = read_mnist_labels(strcat(fname, f2), 60000);
    fname[i] = '\0';
    mnist->test_10k = read_mnist_images(strcat(fname, f3), 10000);
    fname[i] = '\0';
    mnist->label_10k = read_mnist_labels(strcat(fname, f4), 10000);

    return mnist;
}

inline void
MNIST_destroy(MNIST *mnist) {
    free(mnist->train_60k);
    free(mnist->label_60k);
    free(mnist->test_10k);
    free(mnist->label_10k);
}

inline unsigned char *
MNIST_train_img(MNIST *mnist, size_t i) {
#if MNIST_BOUNDCHECK
    if (i >= 60000) {
        printf("Cannot get MNIST test image %zu, there are only 60k.\n", i);
        exit(-1);
    }
#endif
    return mnist->train_60k + (i * MNIST_IMG_SIZE);
}

inline unsigned char *
MNIST_test_img(MNIST *mnist, size_t i) {
#if MNIST_BOUNDCHECK
    if (i >= 10000) {
        printf("Cannot get MNIST test image %zu, there are only 10k.\n", i);
        exit(-1);
    }
#endif
    return mnist->test_10k + (i * MNIST_IMG_SIZE);
}

inline unsigned char
MNIST_train_label(MNIST *mnist, size_t i) {
#if MNIST_BOUNDCHECK
    if (i >= 60000) {
        printf("Cannot get MNIST train label %zu, there are only 60k.\n", i);
        exit(-1);
    }
#endif
    return mnist->label_60k[i];
}

inline unsigned char
MNIST_test_label(MNIST *mnist, size_t i) {
#if MNIST_BOUNDCHECK
    if (i >= 10000) {
        printf("Cannot get MNIST test label %zu, there are only 10k.\n", i);
        exit(-1);
    }
#endif
    return mnist->label_10k[i];
}

inline bool
MNIST_save_image(unsigned char *image, char *fname) {
    // Open/create file
    FILE *fp;
    if ((fp = fopen(fname, "wb")) == NULL) return 1;

    // Write header
    if (fputs("P5\n"
              "28\n"
              "28\n"
              "255\n",
              fp) == EOF) {
        fclose(fp);
        return 1;
    }

    // Write raster
    for (unsigned int y = 0; y < 28; y++) {
        for (unsigned int x = 0; x < 28; x++) {
            if (fputc(image[y * 28 + x], fp) == EOF) return 1;
        }
    }

    // Yeet
    return fclose(fp) ? 1 : 0;
}
