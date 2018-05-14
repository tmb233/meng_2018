/* Copyright (c) 2017 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef MBEDTLS_AES_ALT_H
#define MBEDTLS_AES_ALT_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef MBEDTLS_AES_ALT

#include "ssi_aes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AES context structure
 */
typedef struct
{
    SaSiAesUserContext_t user_context;   ///< User context for CC310 AES.
    uint8_t              key_buffer[32]; ///< Buffer for an encryption key.
    SaSiAesUserKeyData_t key;            ///< CC310 AES key structure.
    SaSiAesEncryptMode_t mode;           ///< Current context operation mode (encrypt/decrypt).
}
mbedtls_aes_context;

/**
 * @brief Initialize AES context
 *
 * @param[in,out] ctx AES context to be initialized
 */
void mbedtls_aes_init(mbedtls_aes_context * ctx);

/**
 * @brief Clear AES context
 *
 * @param[in,out] ctx AES context to be cleared
 */
void mbedtls_aes_free(mbedtls_aes_context * ctx);

/**
 * @brief AES key schedule (encryption)
 *
 * @param[in,out] ctx     AES context to be initialized
 * @param[in]     key     encryption key
 * @param[in]     keybits must be 128, 192 or 256
 *
 * @return 0 if successful
 */
int mbedtls_aes_setkey_enc(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits);

/**
 * @brief AES key schedule (decryption)
 *
 * @param[in,out] ctx     AES context to be initialized
 * @param[in]     key     decryption key
 * @param[in]     keybits must be 128, 192 or 256
 *
 * @return 0 if successful
 */
int mbedtls_aes_setkey_dec(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits);

/**
 * @brief AES-ECB block encryption/decryption
 *
 * @param[in,out] ctx    AES context
 * @param[in]     mode   MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * @param[in]     input  16-byte input block
 * @param[out]    output 16-byte output block
 *
 * @return 0 if successful
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context * ctx,
                          int                   mode,
                          const unsigned char   input[16],
                          unsigned char         output[16]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_ALT_H */
