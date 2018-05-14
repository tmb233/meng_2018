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

#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef MBEDTLS_SHA256_ALT

#include "crys_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SHA-256 context structure
 */
typedef struct
{
    CRYS_HASHUserContext_t    user_context; ///< User context for CC310 SHA256
    CRYS_HASH_OperationMode_t mode;         ///< CC310 hash operation mode
}
mbedtls_sha256_context;

/**
 * @brief Initialize SHA-256 context
 *
 * @param[in,out] ctx SHA-256 context to be initialized
 */
void mbedtls_sha256_init(mbedtls_sha256_context * ctx);

/**
 * @brief Clear SHA-256 context
 *
 * @param[in,out] ctx SHA-256 context to be cleared
 */
void mbedtls_sha256_free(mbedtls_sha256_context * ctx);

/**
 * @brief Clone (the state of) a SHA-256 context
 *
 * @param[out] dst The destination context
 * @param[in]  src The context to be cloned
 */
void mbedtls_sha256_clone(mbedtls_sha256_context * dst, const mbedtls_sha256_context * src);

/**
 * @brief SHA-256 context setup
 *
 * @param[in,out] ctx   context to be initialized
 * @param[in]     is224 0 = use SHA256, 1 = use SHA224
 */
void mbedtls_sha256_starts(mbedtls_sha256_context * ctx, int is224);

/**
 * @brief SHA-256 process buffer
 *
 * @param[in,out] ctx   SHA-256 context
 * @param[in]     input buffer holding the  data
 * @param[in]     ilen  length of the input data
 */
void mbedtls_sha256_update(mbedtls_sha256_context * ctx, const unsigned char * input, size_t ilen);

/**
 * @brief SHA-256 final digest
 *
 * @param[in,out] ctx    SHA-256 context
 * @param[out]    output SHA-224/256 checksum result
 */
void mbedtls_sha256_finish(mbedtls_sha256_context * ctx, unsigned char output[32]);

/* Internal use */
void mbedtls_sha256_process(mbedtls_sha256_context * ctx, const unsigned char data[64]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SHA256_ALT */

#endif /* MBEDTLS_SHA256_ALT_H */
