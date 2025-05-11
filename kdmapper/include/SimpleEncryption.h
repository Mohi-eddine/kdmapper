#pragma once
#include <stdlib.h>
#include <stdio.h>
#include "intrin.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include "Sha256_SIMD.h"

#ifndef __cplusplus
typedef unsigned char bool;
#endif

#define RotateLeft(value, shift)  ((value << shift) | (value >> ((sizeof(value) * 8)- shift)))
#define RotateRight(value, shift) ((value >> shift) | (value << ((sizeof(value) * 8)- shift)))

// Rotate left for __m128i
#define RotateLeft128_64(value, shift) \
    _mm_or_si128(_mm_slli_epi64((value), (shift)), _mm_srli_epi64((value), (64 - (shift))))

#define RotateLeft128_32(value, shift) \
    _mm_or_si128(_mm_slli_epi32((value), (shift)), _mm_srli_epi32((value), (32 - (shift))))

#define RotateLeft128_16(value, shift) \
    _mm_or_si128(_mm_slli_epi16((value), (shift)), _mm_srli_epi16((value), (16 - (shift))))

// Rotate right for __m128i
#define RotateRight128_64(value, shift) \
    _mm_or_si128(_mm_srli_epi64((value), (shift)), _mm_slli_epi64((value), (64 - (shift))))

#define RotateRight128_32(value, shift) \
    _mm_or_si128(_mm_srli_epi32((value), (shift)), _mm_slli_epi32((value), (32 - (shift))))

#define RotateRight128_16(value, shift) \
    _mm_or_si128(_mm_srli_epi16((value), (shift)), _mm_slli_epi16((value), (16 - (shift))))

#define IsValidKeyChar(Char) (isprint(Char))
#define IsValidKeyWChar(Char) (iswprint(Char))

#ifdef __cplusplus
extern "C" {
#endif

bool IsSSE42Supported();

uint32_t GetKeyHash(__m128i Key[2]);
uint32_t KeyGenerator(wchar_t* Password, size_t PasswordSize, __m128i Key[2]);
uint32_t RandKeyGenerator(__m128i Key[2]);
void ShuffleLeft(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash);
void ShuffleRight(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash);
void UndoShuffleLeft(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash);
void UndoShuffleRight(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash);
#ifdef ENCRYPT_ENABLE
bool EncryptBytes(uint8_t* Bytes, size_t BytesCount, __m128i Key[2]);
#endif
bool DecryptBytes(uint8_t* Bytes, size_t BytesCount, __m128i Key[2]);


#ifdef __cplusplus
}
#endif