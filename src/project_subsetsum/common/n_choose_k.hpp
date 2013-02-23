#ifndef SSS_N_CHOOSE_K_HPP
#define SSS_N_CHOOSE_K_HPP

#include <stdint.h>

void n_choose_k_init();

uint64_t n_choose_k_new(uint32_t n, uint32_t k);

/**
 *  This only works up 68 choose 34.  After that we need to use a big number library
 */
uint64_t n_choose_k(uint32_t n, uint32_t k);
#endif
