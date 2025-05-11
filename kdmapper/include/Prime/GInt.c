//
// Created by ubuntu on 15/05/2019.
// optimized and extended by MoHieDDiNNE on 08/05/2025
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "GInt.h"

//gint ggint_cache[GInt_Base + 1][GInt_Base + 1] = { 0 };
//CASH_INFO ggint_cache_info[GInt_Base + 1] = { 0 };
//bool ggint_cache_full = false;
//bool ggint_cache_empty = true;
//gint ggint_lastnum = { 0 };

CRITICAL_SECTION ModCriticalSection = { 0 };

//b = b + a
void ggint_add_gint( gint a, gint b)
{
    if (ggint_is_zero(a))
    {
        return;
    }
    else if (ggint_is_zero(b))
    {
        memcpy(b, a, sizeof(gint));
        return;
    }
    OP_INT x = 0;
    size_t i = (sizeof(gint) / 8) - 1;
    size_t j = GInt_ElementCount;

    while ((((uint64_t*)a)[i] == 0) && (((uint64_t*)b)[i] == 0) && (i > 0))
    {
        i--;
    }
    j = ((++i) * 8) / (sizeof(BASE_INT));
    if ((a[j - 1] | b[j - 1]) != 0)
    {
        j++;//add one more unit space for the carry
        j = min(j, GInt_ElementCount);//make sure that we don't overflow the buffer
    }

    for (i = 0; i < j; i++)
    {
        x = (OP_INT) b[i] + a[i] + x;
        b[i] = (BASE_INT)x;
        x >>= sizeof(BASE_INT) * 8;
    }
}

//b = b + a
void ggint_add_int(BASE_INT a, gint b)
{
    if (a == 0)
        return;

    OP_INT x = a;
    size_t i = (sizeof(gint) / 8) - 1;
    size_t j = GInt_ElementCount;

    while ((((uint64_t*)b)[i] == 0) && (i > 0))
    {
        i--;
    }
    j = ((++i) * 8) / (sizeof(BASE_INT));
    if (b[j - 1] != 0)
    {
        j++;//add one more unit space for the carry
        j = min(j, GInt_ElementCount);//make sure that we don't overflow the buffer
    }

    for (i = 0; i < j; i++)
    {
        x = (OP_INT) b[i] + x;
        b[i] = (BASE_INT)x;
        x >>= sizeof(BASE_INT) * 8;

        if (x == 0)
            break;
    }
}

//b = b - a
void ggint_sub_gint(gint a, gint b)
{
    if (ggint_equal(a, b))
    {
        memset(b, 0, sizeof(gint));
        return;
    }
    else if (ggint_is_zero(b))
    {
        memcpy(b, a, sizeof(gint));
        ggint_negate(b);
        return;
    }

    gint _a;
    memcpy(_a, a, sizeof(_a));
    ggint_negate(_a);
    ggint_add_gint(_a, b);
}

//b = b - a
void ggint_sub_int(BASE_INT a, gint b)
{
    if (a == 0)
    {
        return;
    }
    else if (ggint_is_zero(b))
    {
        b[0] = a;
        ggint_negate(b);
        return;
    }
    gint _a = { 0 };
    _a[0] = a;
    ggint_negate(_a);
    ggint_add_gint(_a, b);
}

//a = a << sh
void ggint_shbl(gint _a, size_t sh)
{
    if (sh == 0)
        return;
    uint8_t* a = (uint8_t*)_a;
    ggint_shl(_a, sh / GInt_DigitBits);
    sh = sh % GInt_DigitBits;

    uint16_t mask = GInt_Base - (1 << (GInt_DigitBits - sh));
    uint8_t bits0 = 0, bits1 = 0;
    size_t i = 0;
    for (i = 0; i < sizeof(gint); ++i)
    {
        bits1 = a[i] & mask;
        a[i] <<= sh;
        a[i] |= bits0 >> (GInt_DigitBits - sh);
        bits0 = bits1;
    }
}

// a = a >> sh
void ggint_shbr(gint _a, size_t sh)
{
    if (sh == 0)
        return;

    uint8_t* a = (uint8_t*)_a;
    ggint_shr(_a, sh / GInt_DigitBits);
    sh = sh % GInt_DigitBits;

    uint16_t mask = (1 << sh) - 1;
    uint8_t bits0 = 0, bits1 = 0;
    size_t i = 0;
    for (i = sizeof(gint) - 1;; --i)
    {
        bits1 = a[i] & mask;
        a[i] >>= sh;
        a[i] |= bits0 << (GInt_DigitBits - sh);
        bits0 = bits1;
        if (i == 0)
            break;
    }
}

// b = b * a
void ggint_mul_int(BASE_INT a, gint b)
{
    OP_INT r = 0;
    size_t i = (sizeof(gint) / 8) - 1;
    size_t j = GInt_ElementCount;

    if (a == 0)
    {
        memset(b, 0, sizeof(gint));
        return;
    }
    else if (ggint_is_zero(b))
    {
        return;
    }
    while ((((uint64_t*)b)[i] == 0) && (i > 0))
    {
        i--;
    }
    j = ((++i) * 8) / sizeof(BASE_INT);
    if (b[j - 1] != 0)
    {
        j++;//add one more unit space for the carry
        j = min(j, GInt_ElementCount);//make sure that we don't overflow the buffer
    }

    for (i = 0; i < j; ++i)
    {
        r += (OP_INT)a * b[i];
        b[i] = (BASE_INT)r;
        r >>= sizeof(BASE_INT) * 8;
    }
}

// p = a * b
void ggint_mul_gint( gint a,  gint b, gint p)
{
    gint t;
    ggint_zero(p);
    if (ggint_is_zero(a) || ggint_is_zero(b))
        return;

    size_t i = (sizeof(gint) / 8) - 1;
    size_t j = GInt_ElementCount;

    while ((((uint64_t*)a)[i] == 0) && (i > 0))
    {
        i--;
    }
    j = ((++i) * 8) / sizeof(BASE_INT);
    if (a[j - 1] != 0)
    {
        j++;//add one more unit space for the carry
        j = min(j, GInt_ElementCount);//make sure that we don't overflow the buffer
    }

    for (i = 0; i < j; ++i)
    {
        memcpy(t,b, sizeof(t));
        ggint_mul_int(a[i], t); 
        ggint_shl(t, i * sizeof(BASE_INT));
        ggint_add_gint(t, p);
    }
}

// b / a = q, b % a = r
void ggint_div_gint( gint a,  gint b, gint q, gint r)
{
    ggint_zero(q);
    ggint_zero(r);
    if (ggint_is_zero(a))//undefined , we should return large number (+-inf) 
    {
        fprintf(stderr, "Error: Division by zero in ggint_div_gint.\n");
        // Optionally, set q and r to a specific value to indicate an error
        memset(q, UINT64_MAX,sizeof(gint)); // Set q to the maximum value to indicate infinity
        memset(r, UINT64_MAX, sizeof(gint)); // Set r to the maximum value to indicate undefined

        return;
    }
    gint t;
    ggint_zero(t);
    size_t i = (sizeof(gint) / 8) - 1;

    while ((((uint64_t*)b)[i] == 0) && (i > 0))
    {
        i--;
    }
    i = ((++i) * 8); /*/ sizeof(BASE_INT)*/
    i = min(i, GInt_Size - 1);//make sure that we don't overflow the buffer

    for (/*i = j - 1*/;; --i)
    {
        ggint_shl(r, 1/**sizeof(BASE_INT)*/);
        ggint_add_int(((uint8_t*)b)[i], r);
        if (ggint_less_or_equal(a, r))
        {
            ggint_zero(t);
            uint8_t k = 0;
            do
            {
                ++k;
                ggint_add_gint(a, t);
            } while (ggint_less_or_equal(t, r));
            ((uint8_t*)q)[i] = k - 1;
            ggint_sub_gint(a, t);
            ggint_sub_gint(t, r);
        }
        else
        {
            ((uint8_t*)q)[i] = 0;
        }

        if (i == 0)
            break;
    }
}

// b % a = r
void ggint_mod_gint2( gint a,  gint b, gint r)
{
    gint q;
    ggint_div_gint(a,b,q,r);
}

// b % a = r
void ggint_mod_gint(gint a, gint b, gint r)
{
    if (ggint_less(b, a))
    {
        ggint_set_gint(r,b);
        return;
    }

    ggint_zero(r);

    bool IsNew = false;
    gint t;

    uint64_t Hash = _mm_crc32_u64(((uint64_t*)a)[1], ((uint64_t*)a)[0] ^ ((uint64_t*)a)[1]) ^ ((uint64_t*)a)[0];
    uint64_t Index = Hash % (GInt_Base + 1);
    if (ggint_cache_full)
    {
    FullCache:
        //look for unused cache
        for (; Index < (GInt_Base + 1); ++Index)
        {
            if (ggint_cache_info[Index].UsageCount == 0)
            {
                break;
            }
        }
        if (Index >= (GInt_Base + 1))
        {
            Index = 0;//check the other half if the initial index was near the end
            ggint_cache_full = true;
            goto FullCache;
        }
    }
    else if ((ggint_cache_info[Index].Hash != 0) && (ggint_cache_info[Index].Hash != Hash))
    {
        IsNew = true;
        uint64_t temp = Index;
        bool SearchFromStart = false;
        //look for empty slots
        while (ggint_cache_info[Index].Hash && (ggint_cache_info[Index].Hash != Hash))
        {
            ++Index;
            if (Index >= (GInt_Base + 1))
            {
                if (SearchFromStart)
                {
                    Index = temp;
                    ggint_cache_full = true;
                    goto FullCache;
                }
                
                Index = 0;
                SearchFromStart = true;  
            }
        }
    }

    _interlockedincrement64((volatile int64_t*)&(ggint_cache_info[Index].UsageCount));

    if (IsNew || !ggint_equal(a, ggint_cache[Index][1]))
    {   
        ggint_cache_info[Index].Hash = Hash;
        ggint_zero(t);
        size_t k;
        for (k = 0; k < (sizeof(ggint_cache[Index]) / sizeof(ggint_cache[Index][0])); k++)
        {
            ggint_set_gint(ggint_cache[Index][k], t);
            ggint_add_gint(a, t);
        }
        //ggint_set_gint(ggint_lastnum, a);    
    }

    size_t i = (sizeof(gint) / 8) - 1;

    while ((((uint64_t*)b)[i] == 0) && (i > 0))
    {
        i--;
    }
    i = ((++i) * 8);/*/ sizeof(BASE_INT);*/
    i = min(i, GInt_Size - 1);//make sure that we don't overflow the buffer

    for (/*i = GInt_Size - 1*/; ; --i)
    {
        ggint_shl(r, 1);
        ggint_add_int(((uint8_t*)b)[i], r);

        if (ggint_less_or_equal(a, r))
        {
            if (ggint_less_or_equal(ggint_cache[Index][GInt_Base - 1], r))
            {
                ggint_sub_gint(ggint_cache[Index][GInt_Base - 1], r);
            }
            else
            {
                int k0 = 0;
                int k1 = GInt_Base;
                while (true)
                {
                    int m = (k0 + k1)/2;
                    if (ggint_less_or_equal(ggint_cache[Index][m], r))
                    {
                        k0 = m;
                    }
                    else
                    {
                        k1 = m;
                    }
                    if (k1 == k0 + 1) break;
                }
                ggint_sub_gint(ggint_cache[Index][k0], r);
            }
        }
        if (i == 0) break;
    }
    _interlockeddecrement64((volatile int64_t*)&ggint_cache_info[Index].UsageCount);
}

// b % a = r
void ggint_mod_int(size_t a,  gint _b, size_t *r)
{
    uint8_t *b = (uint8_t*)_b;
    *r = 0;
    size_t i = (GInt_Size / 8) - 1;

    while ((((uint64_t*)b)[i] == 0) && (i > 0))
    {
        i--;
    }
    i = ((++i) * 8);
    i = min(i, sizeof(gint) - 1);//make sure that we don't overflow the buffer

    for (/*i = GInt_Size - 1*/;; --i)
    {
        *r = (*r * GInt_Base + b[i]) % a;
        if (i == 0)
            break;
    }
}

//inv = ((x % m) + m) % m; where x is from gcd_extended(a, m, x, y, gcd)
void ggint_mod_inverse(gint a, gint m, gint inv)
{
    gint gcd, x, y;
    ggint_one(inv);
    ggint_gcd_extended(a, m, x, y, gcd);

    if (!ggint_equal(gcd, inv)) //gcd != 1
    {
        inv[0] = 0;
        printf("Inverse doesn't exist");
        return;
    }
    else
    {
        // m is added to handle negative x
        //int res = (x % M + M) % M;
        ggint_mod_gint(m, x, y);
        ggint_add_gint(m, y);
        ggint_mod_gint(m, y, inv);
    }
}

// r = a^x mod n
void ggint_pow_mod(gint a, gint x,  gint n, gint r)
{
    gint t = { 0 };
    gint _x;
    gint _a;

    // Copy exponent to avoid modifying the original
	memcpy(_x, x, sizeof(gint));

    // Reduce base modulo n to avoid unnecessary large multiplications
    ggint_mod_gint(n, a, _a);

    // Initialize result to 1
    ggint_one(r);

    while (ggint_is_zero(_x) == false)
    {
        // If exp is odd, multiply result by base and reduce modulo n
        if (ggint_is_odd(_x))
        {
            ggint_mul_gint(_a, r, t);
            ggint_mod_gint(n, t, r);
        }
        // Divide exp by 2
        ggint_shbr(_x, 1);

        // Square the base and reduce modulo n
        ggint_mul_gint(_a, _a, t);
        ggint_mod_gint(n, t, _a);
    }
}

//result = a^x
void ggint_pow(gint a, gint x, gint result) 
{
    gint temp, base, exp;

    // Initialize result to 1
    ggint_one(result);

    // Copy base and exponent to avoid modifying the originals
    ggint_set_gint(base, a);
    ggint_set_gint(exp, x);

    while (!ggint_is_zero(exp)) 
    {
        // If exp is odd, multiply result by base
        if (ggint_is_odd(exp)) 
        {
            ggint_mul_gint(result, base, temp);
            ggint_set_gint(result, temp);
        }

        // Divide exp by 2
        ggint_shbr(exp, 1);

        // Square the base
        ggint_mul_gint(base, base, temp);
        ggint_set_gint(base, temp);
    }
}

//a = sqrt(a)
void ggint_sqrt(gint a, gint result)
{
    gint low, high, mid, square;

    ggint_zero(low);
    ggint_set_gint(high, a);
    ggint_zero(result);

    while (ggint_less_or_equal(low, high))
    {
        // mid = (low + high) / 2
        ggint_set_gint(mid, low);
        ggint_add_gint(high, mid);
        ggint_shbr(mid, 1);

        // square = mid * mid
        ggint_mul_gint(mid, mid, square);

        if (ggint_equal(square, a)) 
        {
            ggint_set_gint(result, mid);
            return;
        }
        else if (ggint_less(square, a)) 
        {
            ggint_set_gint(low, mid);
            ggint_add_int(1, low);
            ggint_set_gint(result, mid);
        }
        else 
        {
            ggint_set_gint(high, mid);
            ggint_sub_int(1, high);
        }
    }
}

void ggint_gcd_extended(gint a, gint b, gint x, gint y, gint gcd) 
{
    // *x = 1, * y = 0;
    ggint_one(x);
    ggint_zero(y);

    gint x1 = { 0 }, y1 = { 1 }, a1, b1, q, temp;
    ggint_set_gint(a1, a);
    ggint_set_gint(b1, b);
    while (!ggint_is_zero(b1))
    {
        //int q = a1 / b1;
        ggint_div_gint(b1, a1, q, gcd);//using gcd as temp var

        // Update x and x1
        //int temp = *x;
        //*x = x1;
        //x1 = temp - q * x1;
        ggint_set_gint(temp, x);
        ggint_set_gint(x, x1);
        ggint_mul_gint(q, x1, gcd);//using gcd as temp var
        ggint_sub_gint(gcd, temp);
        ggint_set_gint(x1, temp);

        // Update y and y1
       /* temp = *y;
        *y = y1;
        y1 = temp - q * y1;*/
        ggint_set_gint(temp, y);
        ggint_set_gint(y, y1);
        ggint_mul_gint(q, y1, gcd);//using gcd as temp var
        ggint_sub_gint(gcd, temp);
        ggint_set_gint(y1, temp);

        // Update a1 and b1
       /* temp = a1;
        a1 = b1;
        b1 = temp - q * b1;*/
        ggint_set_gint(temp, a1);
        ggint_set_gint(a1, b1);
        ggint_mul_gint(q, b1, gcd);//using gcd as temp var
        ggint_sub_gint(gcd, temp);
        ggint_set_gint(b1, temp);
    }
    //gcd = a1;
    ggint_set_gint(gcd, a1);
}

//gcd = gcd(a,b)
void ggint_gcd(gint a, gint b, gint gcd)
{
    gint x, y;
    ggint_gcd_extended(a, b, x, y, gcd);
}

bool miiller_test(gint d, gint n, gint _n_1)
{
    // Pick a random number in [2..n-2]
    // Corner cases make sure that n > 4
    gint a;
    ggint_rand_range(a,n);// { 2 + (rand() % (n - 4)) };

    //gint _n_x;
    gint temp = { 0 };

    // Compute a^d % n
    gint x;
    ggint_pow_mod(a, d, n, x); // Compute x = a^d % n
    temp[0] = 1;
    if (ggint_equal(x, temp)) // x == 1
    {
        return TRUE;
    }

    if (ggint_equal(x, _n_1))
    {
        return TRUE;
    }

    // Keep squaring x while one of the following doesn't
    // happen
    // (i)   d does not reach n-1
    // (ii)  (x^2) % n is not 1
    // (iii) (x^2) % n is not n-1;
    memcpy(a, d, sizeof(gint));//copy d to a to avoid data overwrite 
    while (!ggint_equal(a, _n_1))//while (d != n - 1) ; d == a
    {
        temp[0] = 2;
        ggint_pow_mod(x, temp, n, x);
        ggint_mul_int(2, a); //d *= 2; d == a

        temp[0] = 1;
        if (ggint_equal(x, temp))//(x == 1)
        {
            return FALSE;
        }

        if (ggint_equal(x, _n_1)) //(x == (n - 1)) //_n_1 = n - 1 
        {
            return TRUE;
        }
    }

    // Return composite
    return FALSE;
}

bool ggint_is_prime_miller(gint n, size_t iterations)
{
    // Corner cases
    gint d = { 1 };
    if (ggint_less_or_equal(n, d))
    {
        return FALSE;
    }
    d[0] = 4;
    if (ggint_equal(n, d))
    {
        return TRUE;
    }

    d[0] = 3;
    if (ggint_less_or_equal(n, d))
    {
        return TRUE;
    }
    
    // Find r such that n = 2^d * r + 1 for some r >= 1
    gint _n_1;
    memcpy(_n_1, n, sizeof(gint));
    ggint_sub_int(1, _n_1);//_n_1 = n - 1;

    memcpy(d, _n_1, sizeof(gint));
    while ((d[0] & 1) == 0) // (d % 2) == 0
    {
        ggint_shbr(d, 1); // d /= 2; is same as d >>= 1
    }
   
    // Iterate given number of 'k' times
    for (size_t i = 0; i < iterations; i++)
    {
        if (!miiller_test(d, n, _n_1))
        {
            return FALSE;
        }
    }

    return TRUE;
}

bool ggint_is_prime_fermat(gint n, size_t iterations)
{
    // Corner cases
    gint temp = { 1 };

    if (ggint_less_or_equal(n, temp))//n <= 1
        return false;
    temp[0] = 4;
    if (ggint_less_or_equal(n, temp))// || n == 4) 
        return false;

    temp[0] = 3;
    if (ggint_less_or_equal(n, temp)) //(n <= 3)
        return true;

    // Try k times
    while (iterations > 0)
    {
        // Pick a random number in [2..n-2]        
        // Above corner cases make sure that n > 4
        gint a;//int a = 2 + rand() % (n - 4);
        ggint_rand_range(a, n);
        gint gcd;
        ggint_gcd(n, a, gcd);

        temp[0] = 1;
        // Checking if a and n are co-prime
        if (!ggint_equal(gcd, temp)) //(gcd(n, a) != 1)
            return false;

        // Fermat's little theorem
        gint r;
        ggint_set_gint(temp, n);
        ggint_sub_int(1, temp);
        ggint_pow_mod(a, temp, n, r);

        ggint_one(temp);
        if (!ggint_equal(r, temp))//(power(a, n - 1, n) != 1) ; a^(n-1) mod n
            return false;

        iterations--;
    }

    return true;
}

//void ggint_print_format( char * pref, gint _x, bool printBytes)
//{
//    uint8_t* x = (uint8_t*)_x;
//    size_t n = 0;
//    for (n = sizeof(gint) - 1; ; --n)
//    {
//        if (x[n] != 0) break;
//        if (n == 0) break;
//    }
//
//    if (printBytes)
//    {
//        printf(" - %16s : ", pref);
//        for (int i = 0; i <= n; ++i)
//            printf("%3d ", x[i]);
//        printf("\n");
//    }
//
//    gint _10, q, r;
//    ggint_set(_10, 10);
//    char str[4096];
//    memset(str,0,4096);
//
//    n = 0;
//    if (printBytes)
//        printf("   %16s : ", "Decimal");
//    else
//        printf(" - %16s : ", pref);
//    while (ggint_is_zero(_x) == false)
//    {
//        ggint_div_gint(_10, _x, q, r);
//        ggint_set_gint(_x,q);
//        str[n++] = '0' + r[0];
//    }
//    size_t i;
//    if(n > 1)
//    {
//        for (i = n - 1; ; --i)
//        {
//            printf("%c", str[i]);
//            if (i == 0) break;
//        }
//    }
//    printf("\n");
//}

void ggint_print(gint a)
{
    if (ggint_is_zero(a))
    {
        printf("%016zX", 0ull);
        return;
    }
    size_t i;
    for (i = (sizeof(gint) / sizeof(uint64_t)) - 1; ; i--)
    {
        if(((uint64_t*)a)[i])
        {
            printf("%016zX", ((uint64_t*)a)[i]);
        }
        if (i==0)
            break;
    }
    //printf("\n\n");
}
void ggint_print_as_aob(gint a)
{
    if (ggint_is_zero(a))
    {
        printf("{ 0 }");
        return;
    }
    size_t i = (sizeof(gint) / 8) - 1;
    size_t j = sizeof(gint);
    while ((((uint64_t*)a)[i] == 0) && (i > 0))
    {
        i--;
    }
    j = ((++i) * 8);

    printf("{ ");
    for (i = 0; i < j; i++)
    {
       printf("0x%02X, ", ((uint8_t*)a)[i]); 
    }
    printf("}");
}
void hex_str_to_gint(const char* hex_str, gint _big_number)
{
    uint8_t* big_number = (uint8_t*)_big_number;
    size_t slen = strlen(hex_str);
    ggint_zero(_big_number);
    size_t j = slen;
    for (size_t i = 0; ; ++i)
    {
        j--;
        if (hex_str[j] < ':' && hex_str[j] > '/')
        {
            big_number[i] = hex_str[j] - '0';
        }
        else
        {
            big_number[i] = hex_str[j] - '7';
        }
        if (j == 0)
            break;

        j--;
        if (hex_str[j] < ':' && hex_str[j] > '/')
        {
            big_number[i] |= (hex_str[j] - '0') << 4;
        }
        else
        {
            big_number[i] |= (hex_str[j] - '7') << 4;
        }
        if (j == 0)
            break;
    }
}