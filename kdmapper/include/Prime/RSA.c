#include "RSA.h"

void generate_rsa_session_keys(size_t BitLength, gint p, gint q, gint n, gint e, gint d)
{
    //gint p = { 0 };
    generate_prime(BitLength, p);
    printf("\n\n>> p_aob: "); ggint_print_as_aob(p); printf("\n");

    //gint q = { 0 };
    generate_prime(BitLength, q);
    printf("\n\n>> q_aob: "); ggint_print_as_aob(q); printf("\n");

    ggint_mul_gint(p, q, n); //N = p * q
    printf("\n\n>> n: "); ggint_print(n); printf("\n");
    printf("\n\n>> n_aob: "); ggint_print_as_aob(n); printf("\n");

    gint phi;//phi(n)
    calculate_phi(p, q, phi);
    printf("\n\n>> phi(n): "); ggint_print(phi); printf("\n");

    generate_encryption_key(phi, 65536, e);
    printf("\n\n>> e: "); ggint_print(e); printf("\n");
    printf("\n\n>> e_aob: "); ggint_print_as_aob(e); printf("\n");

    generate_decryption_key(phi, e, d);
    printf("\n\n>> d: "); ggint_print(d); printf("\n");
    printf("\n\n>> d_aob: "); ggint_print_as_aob(d); printf("\n");

    printf("\n\n>> is d valid: %i", is_decryption_key_valid(phi, e, d));  printf("\n");
}

void calculate_phi(gint p, gint q, gint phi)
{
    ggint_zero(phi);

    ggint_sub_int(1, p);
    ggint_sub_int(1, q);
    ggint_mul_gint(p, q, phi);// T = (p-1)*(q-1)
    printf("\n\n>> phi(n): "); ggint_print(phi); printf("\n");
    //restore primes
    ggint_add_int(1, p);
    ggint_add_int(1, q);
}

//public key
void generate_encryption_key(gint phi, uint64_t InitialValue, gint e)
{
    gint gcd, _1 = { 1 };
    if (!InitialValue)
        ggint_set(e, 2);
    else
        ggint_set(e, InitialValue);
    // Choose e, where 1 < e < phi(n) and gcd(e, phi(n)) == 1
    for (; ggint_less(e, phi); ggint_add_int(1, e))
    {
        ggint_gcd(e, phi,gcd);
        if (ggint_equal(gcd,_1))
            break;
    }

}

//private key
void generate_decryption_key(gint phi, gint e, gint d)
{
    // Compute d such that e * d ? 1 (mod phi(n))
    ggint_mod_inverse(e, phi, d);
}

//here use public key
void rsa_encrypt_single(PRSA_KEY public_key, gint data, gint encrypted_data)
{
    ggint_pow_mod(data, public_key->exp, public_key->mod, encrypted_data); //m^e mod n
}
//here use private key
void rsa_decrypt_single(PRSA_KEY private_key, gint encrypted_data, gint decrypted_data)
{
    ggint_pow_mod(encrypted_data, private_key->exp, private_key->mod, decrypted_data);//m^d mod n
}