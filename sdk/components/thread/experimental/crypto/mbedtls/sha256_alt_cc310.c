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

#include "mbedtls/sha256.h"

#include <string.h>

#ifdef MBEDTLS_SHA256_ALT

void mbedtls_sha256_init(mbedtls_sha256_context * ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

void mbedtls_sha256_free(mbedtls_sha256_context * ctx)
{
    (void)CRYS_HASH_Free(&ctx->user_context);
}

void mbedtls_sha256_clone(mbedtls_sha256_context *dst, const mbedtls_sha256_context *src)
{
    memcpy(dst, src, sizeof(*dst));
}

void mbedtls_sha256_starts(mbedtls_sha256_context * ctx, int is224)
{
    if (is224)
    {
        ctx->mode = CRYS_HASH_SHA224_mode;
    }
    else
    {
        ctx->mode = CRYS_HASH_SHA256_mode;
    }

    (void)CRYS_HASH_Init(&ctx->user_context, ctx->mode);
}

void mbedtls_sha256_update(mbedtls_sha256_context * ctx, const unsigned char * input, size_t ilen)
{
    (void)CRYS_HASH_Update(&ctx->user_context, (unsigned char *)input, ilen);
}

void mbedtls_sha256_finish(mbedtls_sha256_context * ctx, unsigned char output[32])
{
    CRYS_HASH_Result_t result;

    memset(result, 0, sizeof(result));

    (void)CRYS_HASH_Finish(&ctx->user_context, result);

    uint8_t size = CRYS_HASH_SHA256_DIGEST_SIZE_IN_BYTES;

    if (ctx->mode == CRYS_HASH_SHA224_mode)
    {
        size = CRYS_HASH_SHA224_DIGEST_SIZE_IN_BYTES;
    }

    memcpy(output, result, size);
}

void mbedtls_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64])
{
    mbedtls_sha256_update(ctx, data, 64);
}

#endif /* MBEDTLS_SHA256_ALT */
