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

#include "mbedtls/aes.h"

#include <string.h>

#ifdef MBEDTLS_AES_ALT

static void aes_init(mbedtls_aes_context * ctx, SaSiAesEncryptMode_t mode)
{
    memset(&ctx->user_context, 0, sizeof(ctx->user_context));

    ctx->mode = mode;
    ctx->key.pKey = ctx->key_buffer;

    (void)SaSi_AesInit(&ctx->user_context, mode, SASI_AES_MODE_ECB, SASI_AES_PADDING_NONE);
}

void mbedtls_aes_init(mbedtls_aes_context * ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    aes_init(ctx, SASI_AES_ENCRYPT);
}

void mbedtls_aes_free(mbedtls_aes_context * ctx)
{
    (void)SaSi_AesFree(&ctx->user_context);
}

int mbedtls_aes_setkey_enc(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits)
{
    ctx->key.keySize = (keybits + 7) / 8;
    ctx->key.pKey = ctx->key_buffer;

    memcpy(ctx->key_buffer, key, ctx->key.keySize);

    return SaSi_AesSetKey(&ctx->user_context, SASI_AES_USER_KEY, &ctx->key, sizeof(ctx->key));
}

int mbedtls_aes_setkey_dec(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits)
{
    return mbedtls_aes_setkey_enc(ctx, key, keybits);
}

int mbedtls_aes_crypt_ecb(mbedtls_aes_context * ctx,
                          int                   mode,
                          const unsigned char   input[16],
                          unsigned char         output[16])
{
    SaSiAesEncryptMode_t reinit_mode = SASI_AES_ENCRYPT_MODE_LAST;

    if ((mode == MBEDTLS_AES_ENCRYPT) && (ctx->mode != SASI_AES_ENCRYPT))
    {
        reinit_mode = SASI_AES_ENCRYPT;
    }
    else if ((mode == MBEDTLS_AES_DECRYPT) && (ctx->mode != SASI_AES_DECRYPT))
    {
        reinit_mode = SASI_AES_DECRYPT;
    }

    if ((reinit_mode == SASI_AES_ENCRYPT) || (reinit_mode == SASI_AES_DECRYPT))
    {
        aes_init(ctx, reinit_mode);

        uint32_t error = SaSi_AesSetKey(&ctx->user_context,
                                        SASI_AES_USER_KEY,
                                        &ctx->key,
                                        sizeof(ctx->key));

        if (error)
        {
            return error;
        }
    }

    return SaSi_AesBlock(&ctx->user_context, (uint8_t *)input, 16, output);
}

#endif /* MBEDTLS_AES_ALT */
