#ifndef NDARRAY_INCLUDE
#define NDARRAY_INCLUDE
#include <bits/c++config.h>

#include <concepts>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "TsetlinConfig.h"

template <typename T, size_t... shape>
class NDArray {
    static_assert((1 and ... and shape), "All dimensions must be non-zero");

    static constexpr size_t dimensions = sizeof...(shape);
    static constexpr size_t arr_size = (1 * ... * shape);
    T arr[arr_size];

    // clang-format off
    static std::string
    shape_str()
    requires(dimensions == 0) {
        return "[]";
    }
    
    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1) 
    static std::string
    shape_str(size_t arg1, Args... args) {
        return "[" + std::to_string(arg1) +
               (("," + std::to_string(args)) + ... + "") + "]";
    }
    // clang-format on

   public:
    constexpr static size_t
    safe_mult(const size_t l, const size_t r) noexcept {
        return l * r;
    }

    T*
    get_backing() {
        return arr;
    }

    size_t
    get_backing_size() {
        return arr_size;
    }

    static size_t
    index1d() requires(dimensions == 0) {
        return 0;
    }

    // clang-format off
    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1)
    static size_t
    index1d(size_t arg1, Args... args) noexcept {
        size_t totalOffset = 1;
        size_t idx = arg1;

        ([&](size_t shape_dim) {
            ([&](size_t arg) {
                totalOffset *= shape_dim;
                idx += arg * totalOffset;
            }(args), ...);
        }(shape), ...);

        return idx;
    }

    size_t
    get() requires(dimensions == 0) { return arr[0]; }

    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1)
    size_t
    get(size_t arg1, Args... args) noexcept {
        return arr[index1d(arg1, args...)];
    }

    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions)
    static void
    bounds_check(Args... args) {    
        size_t i = 0;
        ([&](size_t shape_dim) {
            ([&](size_t arg) {
                if (arg >= shape_dim) {
                    throw std::out_of_range("Argument #" + 
                                      std::to_string(i) + 
                                      " (got: " +std::to_string(arg) + 
                                      ") out of range for shape: " + 
                                      shape_str(shape...) + ".");
                }
                i++;
            }(args), ...);
        }(shape), ...);
    }

    size_t
    get_checked() requires(dimensions == 0) {
        return 0;
    }

    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1)
    size_t
    get_checked(size_t arg1, Args... args) {
        bounds_check(arg1, args...);
        return arr[index1d(arg1, args...)];
    }
    // clang-format on
};

#endif  // NDARRAY_INCLUDE

int
main() {
    NDArray<int> nd0;
    NDArray<int, 5> nd1;
    NDArray<int, 1, 2, 3> nd3;

    std::cout << nd0.get() << std::endl;
    std::cout << nd1.get(4) << std::endl;
    std::cout << nd3.get(1, 1, 1) << std::endl;
    nd0.get_checked();
    nd1.get_checked(5);
    nd3.get_checked(1, 1, 1);

    return 0;
}
/*
```java
public static int index1d(int[] shape, int... indices) {
    int totalOffset = 1;
    int idx = indices[0], d = 0;

    for (d = 1; d < indices.length; d++) {
                          totalOffset *= shape[d - 1];
                          idx += indices[d] * totalOffset;
                }

    return idx;
}
```
*/