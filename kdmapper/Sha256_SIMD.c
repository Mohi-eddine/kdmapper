#include "SHA256_SIMD.h"

//Hw related Macros
#define LOAD(X)       _mm_load_si128((__m128i const *)X)
#define LOAD_U(X)     _mm_loadu_si128((__m128i const *)X)
#define STORE(X, Y)   _mm_store_si128((__m128i *)X,Y)
#define ALIGNR(X, Y)  _mm_alignr_epi8(X,Y,4)
#define ADD(X, Y)     _mm_add_epi32(X,Y)
//#define HIGH(X)     _mm_srli_si128(X,8)
#define HIGH(X)       _mm_shuffle_epi32(X, 0x0E)
#define SHA(X, Y, Z)  _mm_sha256rnds2_epu32(X,Y,Z)
#define MSG1(X, Y)    _mm_sha256msg1_epu32(X,Y)
#define MSG2(X, Y)    _mm_sha256msg2_epu32(X,Y)
#define CVLO(X, Y, Z) _mm_shuffle_epi32(_mm_unpacklo_epi64(X,Y),Z)
#define CVHI(X, Y, Z) _mm_shuffle_epi32(_mm_unpackhi_epi64(X,Y),Z)
//#define L2BMASK       _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203)
#define L2B(X)        _mm_shuffle_epi8(X,_mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203))

//Sw Related Macros
#define ROTATE(x,y)  (((x)>>(y)) | ((x)<<(32-(y))))
#define Sigma0(x)    (ROTATE((x), 2) ^ ROTATE((x),13) ^ ROTATE((x),22))
#define Sigma1(x)    (ROTATE((x), 6) ^ ROTATE((x),11) ^ ROTATE((x),25))
#define sigma0(x)    (ROTATE((x), 7) ^ ROTATE((x),18) ^ ((x)>> 3))
#define sigma1(x)    (ROTATE((x),17) ^ ROTATE((x),19) ^ ((x)>>10))

#define Ch(x,y,z)    (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)   (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define B2U32(val,sh)  (((uint32_t)(val)) << (sh))

const SHA256_ALIGN uint32_t CONST_K2[64] = {
	0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
	0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
	0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
	0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
	0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
	0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
	0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
	0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
	0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
	0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
	0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
	0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
	0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
	0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
	0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
	0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

const SHA256_ALIGN uint32_t INIT_STATE[8] = {
	0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
	0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

static bool IsSha256HwSupported = FALSE;

bool IsSha256Supported()
{
	int info[4] = { 0 };

#if defined(_MSC_VER)
	__cpuid(info, 0);
	if (info[0] < 7)
		return FALSE;
	__cpuidex(info, 7, 0); // Get extended CPU feature information (EAX=7, ECX=0)
#else
	__cpuid(0, info[0], info[1], info[2], info[3]);
	if (info[0] < 7)
		return FALSE;
	__cpuid_count(7, 0, info[0], info[1], info[2], info[3]);
#endif

	// Check if SHA support is available (bit 29 of ECX register)
	IsSha256HwSupported = ((info[1] & (1 << 29)) != 0);
	return IsSha256HwSupported;
}

void InitSha256(SHA256_SIMD_CTX* ctx)
{
	// Initialize the SHA-256 state (H0-H7)
	memset(ctx, 0, sizeof(SHA256_SIMD_CTX));
	ctx->state0 = _mm_set_epi32(INIT_STATE[3], INIT_STATE[2], INIT_STATE[1], INIT_STATE[0]);
	ctx->state1 = _mm_set_epi32(INIT_STATE[7], INIT_STATE[6], INIT_STATE[5], INIT_STATE[4]);
	//ctx->bufferLength = 0;
	//ctx->totalLength = 0;
}

//software version of sha256
void SwUpdateSha256(__m128i state[2], const uint8_t* msg, size_t num_blocks)
{
	uint32_t a, b, c, d, e, f, g, h, s0, s1, T1, T2;
	uint32_t X[16], i;

	while (num_blocks--)
	{
		a = state[0].m128i_u32[0];
		b = state[0].m128i_u32[1];
		c = state[0].m128i_u32[2];
		d = state[0].m128i_u32[3];
		e = state[1].m128i_u32[0];
		f = state[1].m128i_u32[1];
		g = state[1].m128i_u32[2];
		h = state[1].m128i_u32[3];

		for (i = 0; i < 16; i++)
		{
			X[i] = B2U32(msg[0], 24) | B2U32(msg[1], 16) | B2U32(msg[2], 8) | B2U32(msg[3], 0);
			msg += 4;

			T1 = h;
			T1 += Sigma1(e);
			T1 += Ch(e, f, g);
			T1 += CONST_K2[i];
			T1 += X[i];

			T2 = Sigma0(a);
			T2 += Maj(a, b, c);

			h = g;
			g = f;
			f = e;
			e = d + T1;
			d = c;
			c = b;
			b = a;
			a = T1 + T2;
		}

		for (; i < 64; i++)
		{
			s0 = X[(i + 1) & 0x0f];
			s0 = sigma0(s0);
			s1 = X[(i + 14) & 0x0f];
			s1 = sigma1(s1);

			T1 = X[i & 0xf] += s0 + s1 + X[(i + 9) & 0xf];
			T1 += h + Sigma1(e) + Ch(e, f, g) + CONST_K2[i];
			T2 = Sigma0(a) + Maj(a, b, c);
			h = g;
			g = f;
			f = e;
			e = d + T1;
			d = c;
			c = b;
			b = a;
			a = T1 + T2;
		}

		state[0].m128i_u32[0] += a;
		state[0].m128i_u32[1] += b;
		state[0].m128i_u32[2] += c;
		state[0].m128i_u32[3] += d;
		state[1].m128i_u32[0] += e;
		state[1].m128i_u32[1] += f;
		state[1].m128i_u32[2] += g;
		state[1].m128i_u32[3] += h;
	}
}

//hardware accelerated sha256 version
void HwUpdateSha256(__m128i state[2], const uint8_t* msg, size_t num_blocks)
{
	int i, j;
	__m128i A0, C0, ABEF, CDGH, X0, Y0, Ki, W0[4];

	X0 = LOAD(state + 0);
	Y0 = LOAD(state + 1);

	A0 = CVLO(X0, Y0, 0x1B);
	C0 = CVHI(X0, Y0, 0x1B);

	while (num_blocks > 0)
	{
		ABEF = A0;
		CDGH = C0;
		for (i = 0; i < 4; i++)
		{
			Ki = LOAD(CONST_K2 + i);
			W0[i] = L2B(LOAD_U(msg + i));
			X0 = ADD(W0[i], Ki);
			Y0 = HIGH(X0);
			C0 = SHA(C0, A0, X0);
			A0 = SHA(A0, C0, Y0);
		}
		for (j = 1; j < 4; j++)
		{
			for (i = 0; i < 4; i++)
			{
				Ki = LOAD(CONST_K2 + 4 * j + i);
				X0 = MSG1(W0[i], W0[(i + 1) % 4]);
				Y0 = ALIGNR(W0[(i + 3) % 4], W0[(i + 2) % 4]);
				X0 = ADD(X0, Y0);
				W0[i] = MSG2(X0, W0[(i + 3) % 4]);
				X0 = ADD(W0[i], Ki);
				Y0 = HIGH(X0);
				C0 = SHA(C0, A0, X0);
				A0 = SHA(A0, C0, Y0);
			}
		}
		A0 = ADD(A0, ABEF);
		C0 = ADD(C0, CDGH);
		msg += 64;
		num_blocks--;
	}

	X0 = CVHI(A0, C0, 0xB1);
	Y0 = CVLO(A0, C0, 0xB1);

	STORE(state + 0, X0);
	STORE(state + 1, Y0);
}

void ComputeSha256(SHA256_SIMD_CTX* ctx, const uint8_t* data, size_t length, __m128i hash[2])
{
	if (!ctx || !data || !hash)
	{
		return;
	}
	//set the hash to 0
	hash[0] = _mm_xor_si128(hash[0], hash[0]);
	hash[1] = _mm_xor_si128(hash[1], hash[1]);

	__m128i state[2];

	if (!ctx->state0.m128i_u64[0] && !ctx->state0.m128i_u64[1])
	{
		// SHA-256 initial hash values (H0-H7)
		state[0] = _mm_set_epi32(INIT_STATE[3], INIT_STATE[2], INIT_STATE[1], INIT_STATE[0]);
	}
	else
	{
		STORE(&state[0], ctx->state0);
	}

	if (!ctx->state1.m128i_u64[0] && !ctx->state1.m128i_u64[1])
	{
		state[1] = _mm_set_epi32(INIT_STATE[7], INIT_STATE[6], INIT_STATE[5], INIT_STATE[4]);
	}
	else
	{
		STORE(&state[1], ctx->state1);
	}

	size_t BlocksCount = length / 64;
	size_t TotalMessageBitLength = _byteswap_uint64(length * 8); // Total length in bits as big endian
	uint8_t PaddedData[128] = { 0x80 };// Padding starts with 0x80 (1000 0000)b
	size_t PaddedMsgLength = 64;

	if (BlocksCount)
	{
		if (IsSha256HwSupported)
		{
			HwUpdateSha256(state, data, BlocksCount);
		}
		else
		{
			SwUpdateSha256(state, data, BlocksCount);
		}
	}

	size_t AlignmentOverrunLength = (length % 64);
	if (AlignmentOverrunLength)
	{
		memcpy(PaddedData, (data + (length - AlignmentOverrunLength)), AlignmentOverrunLength);
		size_t PaddingLength = 64 - AlignmentOverrunLength;

		if (PaddingLength < 9)//(0x80 + msg length of 8bytes)
		{
			PaddedMsgLength = 128;
			//PaddingLength += 64;
		}

		_bittestandset((long*)&PaddedData[AlignmentOverrunLength], 7);// Padding starts with 0x80 (1000 0000)b
	}

	*(size_t*)(PaddedData + (PaddedMsgLength - 8)) = TotalMessageBitLength;
	if (IsSha256HwSupported)
	{
		HwUpdateSha256(state, PaddedData, PaddedMsgLength / 64);
	}
	else
	{
		SwUpdateSha256(state, PaddedData, PaddedMsgLength / 64);
	}
	
	// Store the final hash values
	_mm_storeu_si128(&ctx->state0, state[0]);
	_mm_storeu_si128(&ctx->state1, state[1]);

	hash[0] = L2B(state[0]);
	hash[1] = L2B(state[1]);
	return;
}