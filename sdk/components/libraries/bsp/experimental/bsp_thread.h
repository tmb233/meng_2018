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

 /**@file
 *
 * @defgroup bsp_thread Thread BSP Module
 * @{
 * @ingroup bsp
 *
 * @brief Module for setting LEDs according to Thread protocol state.
 */

#ifndef BSP_THREAD_H__
#define BSP_THREAD_H__

#include <stdint.h>
#include "bsp.h"
#include "sdk_errors.h"
#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief BSP indication of Thread connection states.
 */
typedef enum
{
    BSP_INDICATE_THREAD_DISABLED = BSP_INDICATE_IDLE,
    BSP_INDICATE_THREAD_DETACHED = BSP_INDICATE_BONDING,
    BSP_INDICATE_THREAD_CONNECTED = BSP_INDICATE_CONNECTED,
} bsp_indication_thread_t;

/** @brief BSP indication of Thread commissioning states.
 */
typedef enum
{
    BSP_INDICATE_COMMISSIONING_NONE,
    BSP_INDICATE_COMMISSIONING_NOT_COMMISSIONED,
    BSP_INDICATE_COMMISSIONING_IN_PROGRESS,
    BSP_INDICATE_COMMISSIONING_SUCCESS,
} bsp_indication_commissioning_t;

/**@brief Function for initializing the Thread BSP Module.
 *
 * Before calling this function, the BSP module must be initialized.
 *
 * @param[in]  p_instance       A pointer to the OpenThread instance.
 *
 * @retval     NRF_SUCCESS      If initialization was successful. Otherwise, a propagated error
 *                              code is returned.
 */
ret_code_t bsp_thread_init(otInstance * p_instance);

/**@brief Function for configuring connection indicators to required states.
 *
 * @details     This function indicates the required state by means of LEDs (if enabled).
 *
 * @param[in]   indicate   State to be indicated.
 *
 * @retval      NRF_SUCCESS               If the state was successfully indicated.
 * @retval      NRF_ERROR_NO_MEM          If the internal timer operations queue was full.
 * @retval      NRF_ERROR_INVALID_STATE   If the application timer module has not been initialized,
 *                                        or internal timer has not been created.
 */
ret_code_t bsp_thread_indication_set(bsp_indication_thread_t indicate);

/**@brief Function for configuring commissioning indicators to required states.
 *
 * @details     This function indicates the required state by means of LEDs (if enabled).
 *
 * @param[in]   indicate   State to be indicated.
 *
 * @retval      NRF_SUCCESS               If the state was successfully indicated.
 * @retval      NRF_ERROR_NO_MEM          If the internal timer operations queue was full.
 * @retval      NRF_ERROR_INVALID_STATE   If the application timer module has not been initialized,
 *                                        or internal timer has not been created.
 */
ret_code_t bsp_thread_commissioning_indication_set(bsp_indication_commissioning_t indicate);

/**@brief Function to indicate received PING packet.
 *
 * @details     This function indicates the required state by means of LEDs (if enabled).
 *
 * @retval      NRF_SUCCESS               If the state was successfully indicated.
 * @retval      NRF_ERROR_NO_MEM          If the internal timer operations queue was full.
 * @retval      NRF_ERROR_INVALID_STATE   If the application timer module has not been initialized,
 *                                        or internal timer has not been created.
 */
ret_code_t bsp_thread_ping_indication_set(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_THREAD_H__ */

/** @} */
