#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "intrin.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "Gint.h"


typedef struct _DATA_TYPE
{
    BASE_INT* data;
    size_t NBits;
    size_t ShiftValue;
    volatile size_t ncheck;
    volatile size_t DoneThreadCount;
    volatile long IsFound;

}DATA_TYPE, * PDATA_TYPE;

#ifdef __cplusplus
extern "C" {
#endif
    uint64_t generate_random_with_crc32(uint64_t init_value);

    void generate_prime(size_t NBits, gint Prime);
#ifdef __cplusplus
}
#endif