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

#include "nrf.h"

#include "crys_rnd.h"
#include "sns_silib.h"

#include <openthread/platform/logging.h>

CRYS_RND_Context_t   m_rndContext;
CRYS_RND_WorkBuff_t  m_rndWorkBuff;

CRYS_RND_Context_t  * pRndContext = &m_rndContext;
CRYS_RND_WorkBuff_t * pRndWorkBuff = &m_rndWorkBuff;

void nrf5CryptoInit(void)
{
    NRF_CRYPTOCELL->ENABLE = 1;

    int ret = SaSi_LibInit(pRndContext, pRndWorkBuff);
    if (ret != SA_SILIB_RET_OK)
    {
        otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_PLATFORM, "Failed SaSi_SiLibInit - ret = 0x%x", ret);
    }
}

void nrf5CryptoDeinit(void)
{
    NRF_CRYPTOCELL->ENABLE = 0;

    (void)SaSi_LibFini(pRndContext);
}
