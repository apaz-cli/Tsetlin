
#ifndef TSETLIN_CONFIG_INCLUDE
#define TSETLIN_CONFIG_INCLUDE

#include <cinttypes>
#include <cstddef>
#include <exception>

//////////////////////////////////////////////////
// Make the code work on 32 and 64 bit machines //
//////////////////////////////////////////////////

// This is also set in the makefile with a script.

#ifndef BITNUM
#define BITNUM 32
#endif

#if BITNUM == 64
#define TINT_PRI "%" PRIu64
#define TINT_BIT_NUM 64
#define TINT_MAX 0xFFFFFFFFFFFFFFFF
#define tint uint64_t
#else
#define TINT_PRI "%" PRIu32
#define TINT_BIT_NUM 32
#define TINT_MAX 0xFFFFFFFF
#define tint uint32_t
#endif  // BITNUM

////////////////////////
// Argmax Tiebreaking //
////////////////////////

// AM_LAST is computationally cheapest.
// AM_RANDOM is computationally most expensive.
#ifndef ARGMAX_TIEBREAK
#define AM_LAST 0
#define AM_FIRST 1
#define AM_RANDOM 2
#define ARGMAX_TIEBREAK AM_LAST
#endif  // ARGMAX_TIEBREAK

#endif  // TSETLIN_CONFIG_INCLUDE