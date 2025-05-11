#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <intrin.h>
#include <vcruntime_string.h>

#ifndef SHA256_ALIGN_BYTES
#define SHA256_ALIGN_BYTES 32
#endif

#ifndef TRUE
#define TRUE  1
#endif // !TRUE

#ifndef FALSE
#define FALSE 0
#endif

#ifndef __cplusplus
typedef unsigned char bool;
#endif

#if __INTEL_COMPILER || _MSC_VER
#define SHA256_ALIGN __declspec(align(SHA256_ALIGN_BYTES))
#else
#define SHA256_ALIGN __attribute__ ((aligned (SHA256_ALIGN_BYTES)))
#endif

typedef struct SHA256_ALIGN {
	__m128i state0; // First 4 hash values (H0-H3)
	__m128i state1; // Last 4 hash values (H4-H7)
	//uint8_t buffer[64]; // Buffer for partial data
	//size_t bufferLength; // Length of data in the buffer
	//size_t totalLength; // Total length of data processed
} SHA256_SIMD_CTX;

#ifdef __cplusplus
extern "C" {
#endif

	bool IsSha256Supported();
	void InitSha256(SHA256_SIMD_CTX* ctx);
	void SwUpdateSha256(__m128i state[2], const uint8_t* msg, size_t num_blocks);
	void HwUpdateSha256(__m128i state[2], const uint8_t* msg, size_t num_blocks);
	void ComputeSha256(SHA256_SIMD_CTX* ctx, const uint8_t* data, size_t length, __m128i hash[2]);

#ifdef __cplusplus
}
#endif