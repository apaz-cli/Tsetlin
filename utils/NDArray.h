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
    static constexpr size_t
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

    /////////
    // GET //
    /////////

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
        return arr[index1d()];
    }

    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1)
    size_t
    get_checked(size_t arg1, Args... args) {
        bounds_check(arg1, args...);
        return arr[index1d(arg1, args...)];
    }
    // clang-format on

    /////////
    // SET //
    /////////

    void
    set(T value) requires(dimensions == 0) {
        arr[0] = value;
    }

    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1) size_t
        set(T value, size_t arg1, Args... args) noexcept {
        arr[index1d(arg1, args...)] = value;
    }

    void
    set_checked(T value) requires(dimensions == 0) {
        arr[0] = value;
    }

    template <std::convertible_to<std::size_t>... Args>
    requires(sizeof...(Args) == dimensions - 1) void set_checked(T value,
                                                                 size_t arg1,
                                                                 Args... args) {
        bounds_check(arg1, args...);
        arr[index1d(arg1, args...)] = value;
    }
};

#endif  // NDARRAY_INCLUDE