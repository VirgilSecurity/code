/*
 * Copyright (c) 2018, Koninklijke Philips N.V.
 */

/**
 * @file
 * Implementation of the encrypt and decrypt functions based on the CCA KEM.
 * algorithm.
 */

#include "r5_cca_pke.h"
#include "r5_parameter_sets.h"
#include "r5_cca_kem.h"
#include "r5_dem.h"
#include "r5_hash.h"
#include "misc.h"
#include "rng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Public functions
 ******************************************************************************/

int r5_cca_pke_keygen(unsigned char *pk, unsigned char *sk) {
    return r5_cca_kem_keygen(pk, sk);
}

int r5_cca_pke_encrypt(unsigned char *ct, unsigned long long *ct_len, const unsigned char *m, const unsigned long long m_len, const unsigned char *pk) {
    int ret = 0;
    const unsigned long long c1_len = PARAMS_CT_SIZE + PARAMS_KAPPA_BYTES;
    unsigned char c1[PARAMS_CT_SIZE + PARAMS_KAPPA_BYTES];
    unsigned long long c2_len;
    unsigned char k[PARAMS_KAPPA_BYTES];

    /* Determine c1 and k */
    ret = r5_cca_kem_encapsulate(c1, k, pk);
    if (ret < 0){
        return ret;
    }
    
    /* Copy c1 into first part of ct */
    memcpy(ct, c1, c1_len);
    *ct_len = c1_len;

    /* Apply DEM to get second part of ct */
    if (round5_dem(ct + c1_len, &c2_len, k, m, m_len)) {
        DEBUG_ERROR("Failed to apply DEM\n");
        return -1;
    }
    
    *ct_len += c2_len;

    return ret;
}

int r5_cca_pke_decrypt(unsigned char *m, unsigned long long *m_len, const unsigned char *ct, unsigned long long ct_len, const unsigned char *sk) {
    int ret = 0;
    unsigned char k[PARAMS_KAPPA_BYTES];
    const unsigned char * const c1 = ct;
    const unsigned long long c1_len = PARAMS_CT_SIZE + PARAMS_KAPPA_BYTES;
    const unsigned char * const c2 = ct + c1_len;
    const unsigned long c2_len = ct_len - c1_len;

    /* Check length, should be at least c1_len + 16 (for the DEM tag) */
    if (ct_len < (c1_len + 16U)) {
        DEBUG_ERROR("Invalid ciphertext message: %llu < %llu\n", ct_len, c1_len + 16U);
        return -1;
    }

    /* Determine k */
    ret = r5_cca_kem_decapsulate(k, c1, sk);
    if (ret < 0){
        return ret;
    }
    
    /* Apply DEM-inverse to get m */
    if (round5_dem_inverse(m, m_len, k, c2, c2_len)) {
        DEBUG_ERROR("Failed to apply DEM-inverse\n");
        return -1;
    }

    return ret;
}
