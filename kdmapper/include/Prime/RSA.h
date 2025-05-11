#pragma once

#include "prime.h"

typedef struct _RSA_KEY
{
    gint mod;//n
    gint exp;//e or d
}RSA_KEY, * PRSA_KEY;

#ifdef __cplusplus
extern "C" {
#endif

    void generate_rsa_session_keys(size_t BitLength, gint p, gint q, gint n, gint e, gint d);
    void calculate_phi(gint p, gint q, gint phi);
    //public key
    void generate_encryption_key(gint phi, uint64_t InitialValue, gint e);
    //private key
    void generate_decryption_key(gint phi, gint e, gint d);

    inline bool is_decryption_key_valid(gint phi, gint e, gint d)
    {
        gint _1 = { 1 };
        gint _e_d;
        gint r;

        ggint_mul_gint(d, e, _e_d);
        ggint_mod_gint(phi, _e_d, r);
        if (ggint_equal(r, _1))
            return true;

        return false;
    }

    void rsa_encrypt_single(PRSA_KEY public_key, gint data, gint encrypted_data);//here use public key
    void rsa_decrypt_single(PRSA_KEY private_key, gint encrypted_data, gint decrypted_data);//here use private key

#ifdef __cplusplus
}
#endif