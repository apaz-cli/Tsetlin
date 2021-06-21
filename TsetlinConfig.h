
#ifndef TSETLIN_CONFIG_INCLUDE
#define TSETLIN_CONFIG_INCLUDE

#include <cinttypes>
#include <cstddef>

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