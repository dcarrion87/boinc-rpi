#include <stdint.h>

#include "n_choose_k.hpp"

/*
 * 67 choose 34 is the max that can fit into a uint64_t
 * After that we need to use a big number library.
 */
uint64_t n_choose_k_lookup_table[68][35];

void n_choose_k_init() {
    for (int i = 0; i < 68; i++) {
        for (int j = 0; j <= 34; j++) {
            n_choose_k_lookup_table[i][j] = 0;
        }
    }

    for (int i = 0; i < 68; i++) {
        n_choose_k_lookup_table[i][0] = 1;
        for (int j = 1; j <= i && j <= 34; j++) {
            n_choose_k_lookup_table[i][j] = n_choose_k_lookup_table[i-1][j] + n_choose_k_lookup_table[i-1][j-1];
        }
    }
}

uint64_t n_choose_k(uint32_t n, uint32_t k) {
    return n_choose_k_lookup_table[n][k];
}


/**
 *  This only works up 68 choose 34.  After that we need to use a big number library
 */
uint64_t n_choose_k_old(uint32_t n, uint32_t k) {
    /**
     *  Create pascal's triangle and use it for look up
     *  implement for ints of arbitrary length (need to implement binary add and less than)
     */
    uint32_t numerator = n - (k - 1);
    uint32_t denominator = 1;

    uint64_t combinations = 1;

    while (numerator <= n) {
        combinations *= numerator;
        combinations /= denominator;

        numerator++;
        denominator++;
    }

    return combinations;
}


