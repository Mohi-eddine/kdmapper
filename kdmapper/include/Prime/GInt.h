
#ifndef __GInt__
#define __GInt__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "intrin.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#ifndef min
#define min(a, b) ((a) < (b)) ? (a) : (b)
#endif // min
#ifndef max
#define max(a, b) ((a) > (b))? (a) : (b)
#endif // max

#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#endif

#ifndef true
#define true 1
#endif // true
#ifndef false
#define false 0
#endif // false

#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef volatile struct _CASH_INFO
{
    uint64_t Hash;
    size_t UsageCount;
}CASH_INFO, * PCASH_INFO;

typedef uint32_t BASE_INT;
typedef uint64_t OP_INT;

#define GInt_ElementCount ((size_t)(512/sizeof(BASE_INT))) // 512 bytes = 2048 bits this number must always be a multiple of 8
#define GInt_DigitBits 8 // 8 bits = 1 byte
#define GInt_Base 256 // 2^8 = 256

typedef BASE_INT gint[GInt_ElementCount];
#define GInt_Size ((size_t) sizeof(gint))


static gint ggint_cache[GInt_Base + 1][GInt_Base + 1] = { 0 };
static CASH_INFO ggint_cache_info[GInt_Base + 1] = { 0 };
static bool ggint_cache_full = false;
//bool ggint_cache_empty = true;
//static gint ggint_lastnum = { 0 };

extern CRITICAL_SECTION ModCriticalSection;


#ifdef __cplusplus
extern "C" {
#endif

    // only call this when the cash is not being used by any thread
    inline void ggint_reset_cash();

    // generate random number a
    inline void ggint_rand(gint a);

    // generate random number a < b
    inline void ggint_rand_range(gint a, gint b);

    // a == b
    inline bool ggint_equal(gint a, gint b);

    // a < b
    inline bool ggint_less(gint a, gint b);

    // a <= b
    inline bool ggint_less_or_equal(gint a, gint b);

    // a == 0
    inline bool ggint_is_zero(gint a);

    // a & 1 == 0
    inline bool ggint_is_even(gint a);

    // a & 1 == 1
    inline bool ggint_is_odd(gint a);

    // a < 0 where (a[GInt_Size - 1] & 0x80) != 0
    inline bool ggint_is_signed(gint a);

    // a = 0
    inline void ggint_zero(gint a);

    // a = 1
    inline void ggint_one(gint a);

    // a = value
    inline void ggint_set(gint a, uint64_t value);

    // a = b
    inline void ggint_set_gint(gint a, gint b);

    //b = b & a
    inline void ggint_bw_and(gint a, gint b);

    //b = b | a
    inline void ggint_bw_or(gint a, gint b);

    //b = b ^ a
    inline void ggint_bw_xor(gint a, gint b);

    //a = ~a
    inline void ggint_bw_not(gint a);

    //a = negate(a)
    inline void ggint_negate(gint a);

    //b = b + a
    void ggint_add_gint(gint a, gint b);

    //b = b + a
    void ggint_add_int(BASE_INT a, gint b);

    //b = b - a
    void ggint_sub_gint(gint a, gint b);

    //b = b - a
    void ggint_sub_int(BASE_INT a, gint b);

    //a = a << 8*sh (base 256)
    inline void ggint_shl(gint a, size_t sh);

    //a = a >> 8*sh (base 256)
    inline void ggint_shr(gint a, size_t sh);

    //a = a << sh
    void ggint_shbl(gint a, size_t sh);

    // a = a >> sh
    void ggint_shbr(gint a, size_t sh);

    // b = b * a
    void ggint_mul_int(BASE_INT a, gint b);

    // p = a * b
    void ggint_mul_gint(gint a, gint b, gint p);

    // b / a = q, b % a = r
    void ggint_div_gint(gint a, gint b, gint q, gint r);

    // b % a = r
    void ggint_mod_gint2(gint a, gint b, gint r);

    // b % a = r
    void ggint_mod_gint(gint a, gint b, gint r);

    // b % a = r
    void ggint_mod_int(size_t a, gint b, size_t* r);

    //inv = ((x % m) + m) % m; where x is from gcd_extended(a, m, x, y, gcd)
    void ggint_mod_inverse(gint a, gint m, gint inv);

    // r = a^x mod n
    void ggint_pow_mod(gint a, gint x, gint n, gint r);

    //a = sqrt(a)
    void ggint_sqrt(gint a, gint result);

    //result = a^x
    void ggint_pow(gint a, gint x, gint result);

    //ax + bx = gcd = gcd(a,b,x,y);
    inline void ggint_gcd_extended(gint a, gint b, gint x, gint y, gint gcd);

    //gcd = gcd(a,b)
    inline void ggint_gcd(gint a, gint b, gint gcd);

    bool miiller_test(gint d, gint n, gint _n_1);
    bool ggint_is_prime_miller(gint n, size_t iterations);
    bool ggint_is_prime_fermat(gint n, size_t iterations);

    //void ggint_print_format(char* pref, gint x, bool printBytes);

    void ggint_print(gint a);
    void ggint_print_as_aob(gint a);

    void hex_str_to_gint(const char* hex_str, gint _big_number);

    //inlined functions 

    // only call this when the cash is not being used by any thread
    void ggint_reset_cash()
    {
        for (size_t i = 0; i < (GInt_Base + 1); i++)
        {
            for (size_t j = 0; j < (GInt_Base + 1); ++j)
            {
                memset(ggint_cache[i][j], 0, sizeof(gint));
            }
            ggint_cache_info[i].Hash = 0;
            ggint_cache_info[i].UsageCount = 0;
        }
    }

    // generate random number a
    void ggint_rand(gint a)
    {
        size_t i = 0;
        uint32_t temp = 0;
        for (i = 0; i < (sizeof(gint) / sizeof(uint32_t)); i++)
        {
            srand((uint32_t)(__rdtsc()));
            ((uint32_t*)a)[i] = _mm_crc32_u32((uint32_t)__rdtsc() | temp, rand());
            temp = ((uint32_t*)a)[i];
        }
    }

    // generate random number a = ggint_rand(a) < b ( a[2..b-4] )
    void ggint_rand_range(gint a, gint b)
    {
        ggint_rand(a);
        ggint_bw_and(b, a);
        ggint_sub_int(4, a);

        a[0] |= 2;
    }

    // a == b
    bool ggint_equal(gint a, gint b)
    {
        size_t i;
        for (i = 0; i < (sizeof(gint) / 8); i++)
        {
            if (((uint64_t*)a)[i] != ((uint64_t*)b)[i])
            {
                return false;
            }
        }
        return true;
    }

    // a < b
    bool ggint_less(gint a, gint b)
    {
        size_t i;
        if (ggint_is_signed(a) && !ggint_is_signed(b))
        {
            return true;
        }

        if (ggint_is_signed(a) && ggint_is_signed(b))
        {
            bool Result = false;
            ggint_negate(a);
            ggint_negate(b);

            for (i = (sizeof(gint) / 8) - 1; ; i--)
            {
                if (((uint64_t*)a)[i] < ((uint64_t*)b)[i])
                {
                    Result = false;
                    break;
                }
                if (((uint64_t*)a)[i] > ((uint64_t*)b)[i])
                {
                    Result = true;
                    break;

                }
                if (i == 0)
                {
                    break;
                }
            }

            ggint_negate(a);
            ggint_negate(b);
            return Result;
        }

        for (i = (sizeof(gint) / 8) - 1; ; i--)
        {
            if (((uint64_t*)a)[i] < ((uint64_t*)b)[i])
            {
                return true;
            }
            if (((uint64_t*)a)[i] > ((uint64_t*)b)[i])
            {
                return false;
            }
            if (i == 0)
            {
                break;
            }
        }
        return false;
    }

    // a <= b
    bool ggint_less_or_equal(gint a, gint b)
    {
        if (ggint_is_signed(a) && !ggint_is_signed(b))
        {
            return true;
        }
        size_t i;

        if (ggint_is_signed(a) && ggint_is_signed(b))
        {
            bool Result = true;
            ggint_negate(a);
            ggint_negate(b);

            for (i = (sizeof(gint) / 8) - 1; ; i--)
            {
                if (((uint64_t*)a)[i] < ((uint64_t*)b)[i])
                {
                    Result = false;
                    break;
                }
                if (((uint64_t*)a)[i] > ((uint64_t*)b)[i])
                {
                    Result = true;
                    break;

                }
                if (i == 0)
                {
                    break;
                }
            }

            ggint_negate(a);
            ggint_negate(b);
            return Result;
        }

        for (i = (sizeof(gint) / 8) - 1; ; i--)
        {
            if (((uint64_t*)a)[i] < ((uint64_t*)b)[i])
            {
                return true;
            }
            if (((uint64_t*)a)[i] > ((uint64_t*)b)[i])
            {
                return false;
            }
            if (i == 0)
            {
                break;
            }
        }
        return true;
    }

    // a == 0
    bool ggint_is_zero(gint a)
    {
        size_t i;
        for (i = 0; i < (sizeof(gint) / 8); ++i)
        {
            if (((uint64_t*)a)[i] != 0)
            {
                return false;
            }
        }
        return true;
    }

    // a & 1 == 0
    bool ggint_is_even(gint a)
    {
        return (a[0] & 1) == 0;
    }

    // a & 1 == 1
    bool ggint_is_odd(gint a)
    {
        return (a[0] & 1) == 1;
    }

    // a < 0 where (a[GInt_Size - 1] & 0x80) != 0
    bool ggint_is_signed(gint a)
    {
        return (a[GInt_ElementCount - 1] & (1 << ((sizeof(BASE_INT) * 8) - 1))) != 0;
    }

    // a = 0
    void ggint_zero(gint a)
    {
        memset(a, 0, sizeof(gint));
    }

    // a = 1
    void ggint_one(gint a)
    {
        ggint_zero(a);
        a[0] = 1;
    }

    // a = value
    void ggint_set(gint a, uint64_t value)
    {
        ggint_zero(a);
        ((uint64_t*)a)[0] = value;
    }

    // a = b
    void ggint_set_gint(gint a, gint b)
    {
        memcpy(a, b, sizeof(gint));
    }

    //b = b & a
    void ggint_bw_and(gint a, gint b)
    {
        for (size_t i = 0; i < (sizeof(gint) / sizeof(uint64_t)); i++)
        {
            ((uint64_t*)b)[i] &= ((uint64_t*)a)[i];
        }
    }

    //b = b | a
    void ggint_bw_or(gint a, gint b)
    {
        for (size_t i = 0; i < (sizeof(gint) / sizeof(uint64_t)); i++)
        {
            ((uint64_t*)b)[i] |= ((uint64_t*)a)[i];
        }
    }

    //b = b ^ a
    void ggint_bw_xor(gint a, gint b)
    {
        for (size_t i = 0; i < (sizeof(gint) / sizeof(uint64_t)); i++)
        {
            ((uint64_t*)b)[i] ^= ((uint64_t*)a)[i];
        }
    }

    //a = ~a
    void ggint_bw_not(gint a)
    {
        for (size_t i = 0; i < (sizeof(gint) / sizeof(uint64_t)); i++)
        {
            ((uint64_t*)a)[i] = ~((uint64_t*)a)[i];
        }
    }

    //a = negate(a)
    void ggint_negate(gint a)
    {
        if (ggint_is_zero(a))
            return;

        ggint_bw_not(a);
        ggint_add_int(1, a);
    }

    //a = a << 8*sh (base 256)
    void ggint_shl(gint _a, size_t sh)
    {
        if (sh == 0 || ggint_is_zero(_a))
            return;

        uint8_t* a = (uint8_t*)_a;
        sh = min(sizeof(gint), sh);
        size_t i = (sizeof(gint) / 8) - 1;
        size_t j = sizeof(gint);

        while ((((uint64_t*)a)[i] == 0) && (i > 0))
        {
            i--;
        }
        j = ((++i) * 8);
        j += sh;
        j = min(j, GInt_Size - 1);//make sure that we don't overflow the buffer

        for (i = j/*GInt_Size - 1*/; i >= sh; i--)
            a[i] = a[i - sh];
        //memcpy(a + sh, a, j);

        memset(a, 0, sh);
    }

    //a = a >> 8*sh (base 256)
    void ggint_shr(gint _a, size_t sh)
    {
        if (sh == 0 || ggint_is_zero(_a))
            return;

        uint8_t* a = (uint8_t*)_a;
        sh = min(sizeof(gint), sh);
        size_t i = (sizeof(gint) / 8) - 1;
        size_t j = sizeof(gint);

        while ((((uint64_t*)a)[i] == 0) && (i > 0))
        {
            i--;
        }
        j = ((++i) * 8);
        if (sh < j)
        {
            j -= sh;
        }

        //memcpy(a, a + sh, j);
        for (i = 0; i < j/*GInt_Size - sh*/; i++)
            a[i] = a[i + sh];

        memset(a + j, 0, GInt_Size - i);
        /*for (i = 0; i < sh; i++)
            a[GInt_Size - 1 - i] = 0;*/
    }

#ifdef __cplusplus
}
#endif

#endif
