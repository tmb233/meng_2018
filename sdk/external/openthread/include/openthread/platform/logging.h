/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file includes the platform abstraction for the debug log service.
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-logging
 *
 * @brief
 *   This module includes the platform abstraction for the debug log service.
 *
 * @{
 *
 */

/**
 * Log levels.
 */
#define OT_LOG_LEVEL_NONE  0  ///< None
#define OT_LOG_LEVEL_CRIT  1  ///< Critical
#define OT_LOG_LEVEL_WARN  2  ///< Warning
#define OT_LOG_LEVEL_INFO  3  ///< Info
#define OT_LOG_LEVEL_DEBG  4  ///< Debug

typedef uint8_t otLogLevel;

/**
 * This enum represents log regions.
 *
 */
typedef enum otLogRegion
{
    OT_LOG_REGION_API      = 1,  ///< OpenThread API
    OT_LOG_REGION_MLE      = 2,  ///< MLE
    OT_LOG_REGION_ARP      = 3,  ///< EID-to-RLOC mapping.
    OT_LOG_REGION_NET_DATA = 4,  ///< Network Data
    OT_LOG_REGION_ICMP     = 5,  ///< ICMPv6
    OT_LOG_REGION_IP6      = 6,  ///< IPv6
    OT_LOG_REGION_MAC      = 7,  ///< IEEE 802.15.4 MAC
    OT_LOG_REGION_MEM      = 8,  ///< Memory
    OT_LOG_REGION_NCP      = 9,  ///< NCP
    OT_LOG_REGION_MESH_COP = 10, ///< Mesh Commissioning Protocol
    OT_LOG_REGION_NET_DIAG = 11, ///< Network Diagnostic
    OT_LOG_REGION_PLATFORM = 12, ///< Platform
    OT_LOG_REGION_COAP     = 13, ///< CoAP
} otLogRegion;

/**
 * This function outputs logs.
 *
 * @param[in]  aLogLevel   The log level.
 * @param[in]  aLogRegion  The log region.
 * @param[in]  aFormat     A pointer to the format string.
 * @param[in]  ...         Arguments for the format specification.
 *
 */
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DEBUG_H_
