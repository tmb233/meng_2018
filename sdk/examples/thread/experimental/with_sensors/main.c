/* Copyright (c) 2016-2017 Nordic Semiconductor. All Rights Reserved.
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

/** @file
 *
 * @defgroup cloud_coap_client_example_main main.c
 * @{
 * @ingroup cloud_coap_client_example_example
 * @brief Simple Cloud CoAP Client Example Application main file.
 *
 * @details This example demonstrates a CoAP client application that sends emulated
 *          temperature value to the thethings.io cloud. Example uses NAT64 on the
 *          Nordic's Thread Border Router soulution for IPv4 connectivity.
 *
 *
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_drv_gpiote.h"
#include "nrf_drv_clock.h"
#include "bsp_thread.h"
#include "app_timer.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"
#include "app_util_platform.h"

#include "vl6180x.h"
#include "lps22hb.h"
#include "i2c.h"

#include <openthread/openthread.h>
#include <openthread/diag.h>
#include <openthread/coap.h>
#include <openthread/cli.h>
#include <openthread/dns.h>
#include <openthread/platform/platform.h>
#include <openthread/platform/alarm.h>

// General application timer settings.
#define APP_TIMER_PRESCALER             15    // Value of the RTC1 PRESCALER register.
#define APP_TIMER_OP_QUEUE_SIZE         3     // Size of timer operation queues.

#define LED_INTERVAL      200

#define CLOUD_HOSTNAME    "coap.thethings.io"            /**< Hostname of the thethings.io cloud. */
#define DNS_SERVER_IP     "fd00:64:123:4567::1"          /**< IPv6 of the DNS server (DNS64). */

#define NODE1_TOKEN       "v2/things/tEh7ucriAeGEDj-mC9p4u6LrmkmCqHZ52DJGLmxhYcs"      /**< Put your things URI here. */

#define LUX_RESOURCE      "Lux"                      /**< Thing resource name. */
#define LUX_INIT          10000                      /**< The initial value of temperature. */
#define LUX_MIN           0                          /**< Minimal possible temperature value. */
#define LUX_MAX           89000                      /**< Maximum possible temperature value. */

#define CENTIMETERS_RESOURCE      "Centimeters"      /**< Thing resource name. */
#define CENTIMETERS_INIT          100                /**< The initial value of temperature. */
#define CENTIMETERS_MIN           0                  /**< Minimal possible temperature value. */
#define CENTIMETERS_MAX           400                /**< Maximum possible temperature value. */

#define CELSIUS_RESOURCE          "Celsius"          /**< Thing resource name. */

#define MILLIBARS_RESOURCE        "Millibars"        /**< Thing resource name. */

#define CLOUD_COAP_CONTENT_FORMAT 50                 /**< Use application/json content format type. */


APP_TIMER_DEF(m_led_a_timer_id);

typedef enum
{
    APP_STATE_IDLE = 0,            /**< Application in IDLE state. */
    APP_STATE_HOSTNAME_RESOLVING,  /**< Application is currently resolving cloud hostname. */
    APP_STATE_RUNNING              /**< Application is running. */
} state_t;

typedef struct
{
    otInstance   * p_ot_instance;   /**< A pointer to the OpenThread instance. */
    otIp6Address   cloud_address;   /**< NAT64 address of the thethings.io cloud. */
    uint16_t       lux;     /**< The value of emulated temperature. */
    uint16_t       centimeters;     /**< The value of emulated temperature. */
    uint16_t       celsius;
    uint16_t       mbars;
    state_t        state;           /**< The currect state of application. */
} application_t;

application_t m_app =
{
    .p_ot_instance = NULL,
    .cloud_address = { .mFields.m8 = { 0 } },
    .lux           = LUX_INIT,
    .centimeters   = CENTIMETERS_INIT,
    .state         = APP_STATE_IDLE
};

static const char m_cloud_hostname[] = CLOUD_HOSTNAME;
static const otIp6Address m_unspecified_ipv6 = { .mFields.m8 = { 0 } };

static void address_print(const otIp6Address *addr)
{
    char ipstr[40];
    snprintf(ipstr, sizeof(ipstr), "%x:%x:%x:%x:%x:%x:%x:%x",
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 0)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 1)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 2)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 3)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 4)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 5)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 6)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 7)));
}

/***************************************************************************************************
 * @section DNS Client
 **************************************************************************************************/

static void dns_response_handler(void * p_context, const char * p_hostname, otIp6Address * p_address,
                                 uint32_t ttl, otError error)
{
    (void)p_context;

    if (m_app.state != APP_STATE_HOSTNAME_RESOLVING)
    {
        return;
    }

    m_app.state = APP_STATE_RUNNING;

    if (error != OT_ERROR_NONE)
    {
        NRF_LOG_INFO("DNS response error %d.\r\n", error);
        return;
    }

    // Check if its hostname of interest.
    if (p_hostname == m_cloud_hostname && ttl > 0)
    {
        m_app.cloud_address = *p_address;
        m_app.state         = APP_STATE_RUNNING;
    }
}

static void hostname_resolve(otInstance * p_instance,
                             const char * p_hostname)
{
    otError       error;
    otDnsQuery    query;
    otMessageInfo message_info;

    memset(&message_info, 0, sizeof(message_info));
    message_info.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;
    message_info.mPeerPort    = OT_DNS_DEFAULT_DNS_SERVER_PORT;
    otIp6AddressFromString(DNS_SERVER_IP, &message_info.mPeerAddr);

    query.mHostname    = m_cloud_hostname;
    query.mMessageInfo = &message_info;
    query.mNoRecursion = false;

    error = otDnsClientQuery(p_instance, &query, dns_response_handler, NULL);
    if (error != OT_ERROR_NONE)
    {
        NRF_LOG_INFO("Failed to perform DNS Query.\r\n");
    }
}

/***************************************************************************************************
 * @section Cloud CoAP
 **************************************************************************************************/

 static void cloud_data_send(otInstance * p_instance,
                             const char * p_uri_path,
                             char       * p_payload)
 {
     otError      error = OT_ERROR_NO_BUFS;
     otCoapHeader header;
     otCoapOption content_format_option;
     otMessage * p_request;
     otMessageInfo message_info;
     uint8_t content_format = CLOUD_COAP_CONTENT_FORMAT;

     do
     {
         content_format_option.mNumber = OT_COAP_OPTION_CONTENT_FORMAT;
         content_format_option.mLength = 1;
         content_format_option.mValue  = &content_format;

         otCoapHeaderInit(&header, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
         otCoapHeaderAppendUriPathOptions(&header, p_uri_path);
         otCoapHeaderAppendOption(&header, &content_format_option);
         otCoapHeaderSetPayloadMarker(&header);

         p_request = otCoapNewMessage(p_instance, &header);
         if (p_request == NULL)
         {
             NRF_LOG_INFO("Failed to allocate message for CoAP Request\r\n");
             break;
         }

         error = otMessageAppend(p_request, p_payload, strlen(p_payload));
         if (error != OT_ERROR_NONE)
         {
             break;
         }

         memset(&message_info, 0, sizeof(message_info));
         message_info.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;
         message_info.mPeerPort = OT_DEFAULT_COAP_PORT;
         message_info.mPeerAddr = m_app.cloud_address;

         error = otCoapSendRequest(p_instance, p_request, &message_info, NULL, NULL);

     } while (false);

     if (error != OT_ERROR_NONE && p_request != NULL)
     {
         NRF_LOG_INFO("Failed to send CoAP Request: %d\r\n", error);
         otMessageFree(p_request);
     }
 }

 static void cloud_data_update(void)
 {
     char payload_buffer[64];

     sprintf(payload_buffer,
             "{\"values\":[{\"key\":\"%s\",\"value\":\"%d\"}]}",
             LUX_RESOURCE, g_lux);

     cloud_data_send(m_app.p_ot_instance, NODE1_TOKEN, payload_buffer);

     sprintf(payload_buffer,
             "{\"values\":[{\"key\":\"%s\",\"value\":\"%d\"}]}",
             CENTIMETERS_RESOURCE, g_cm);

     cloud_data_send(m_app.p_ot_instance, NODE1_TOKEN, payload_buffer);


     sprintf(payload_buffer,
             "{\"values\":[{\"key\":\"%s\",\"value\":\"%d\"}]}",
             CELSIUS_RESOURCE, g_temperature);

     cloud_data_send(m_app.p_ot_instance, NODE1_TOKEN, payload_buffer);


     sprintf(payload_buffer,
             "{\"values\":[{\"key\":\"%s\",\"value\":\"%d\"}]}",
             MILLIBARS_RESOURCE, g_pressure);

     cloud_data_send(m_app.p_ot_instance, NODE1_TOKEN, payload_buffer);
 }

/***************************************************************************************************
 * @section Default CoAP Handler
 **************************************************************************************************/

static void coap_default_handler(void *p_context, otCoapHeader *p_header, otMessage *p_message,
                                 const otMessageInfo *p_message_info)
{
    (void)p_context;
    (void)p_header;
    (void)p_message;
    (void)p_message_info;

    NRF_LOG_INFO("Received CoAP message that does not match any request or resource\r\n");
}

/***************************************************************************************************
 * @section State
 **************************************************************************************************/

static void handle_role_change(void * p_context, otDeviceRole role)
{
    switch(role)
    {
        case OT_DEVICE_ROLE_CHILD:
        case OT_DEVICE_ROLE_ROUTER:
        case OT_DEVICE_ROLE_LEADER:
            if (m_app.state == APP_STATE_IDLE)
            {
                m_app.state = APP_STATE_HOSTNAME_RESOLVING;
                hostname_resolve(p_context, m_cloud_hostname);
            }
            break;

        case OT_DEVICE_ROLE_DISABLED:
        case OT_DEVICE_ROLE_DETACHED:
        default:
            m_app.state         = APP_STATE_IDLE;
            m_app.cloud_address = m_unspecified_ipv6;
            break;
    }
}

static void state_changed_callback(uint32_t flags, void * p_context)
{
    if (flags & OT_CHANGED_THREAD_ROLE)
    {
        handle_role_change(p_context, otThreadGetDeviceRole(p_context));
    }

    //NRF_LOG_INFO("State changed! Flags: 0x%08x Current role: %d\r\n", flags, otThreadGetDeviceRole(p_context));
}

/***************************************************************************************************
 * @section Buttons
 **************************************************************************************************/

static void bsp_event_handler(bsp_event_t event)
{
}

/***************************************************************************************************
 * @section Initialization
 **************************************************************************************************/

static void thread_init(void)
{
    otInstance *p_instance;

    PlatformInit(0, NULL);

    p_instance = otInstanceInit();
    assert(p_instance);

    otCliUartInit(p_instance);

    assert(otSetStateChangedCallback(p_instance, &state_changed_callback, p_instance) == OT_ERROR_NONE);

    if (!otDatasetIsCommissioned(p_instance))
    {
        assert(otLinkSetChannel(p_instance, THREAD_CHANNEL) == OT_ERROR_NONE);
        assert(otLinkSetPanId(p_instance, THREAD_PANID) == OT_ERROR_NONE);
    }

    assert(otIp6SetEnabled(p_instance, true) == OT_ERROR_NONE);
    assert(otThreadSetEnabled(p_instance, true) == OT_ERROR_NONE);

    m_app.p_ot_instance = p_instance;
}

static void coap_init()
{
    assert(otCoapStart(m_app.p_ot_instance, OT_DEFAULT_COAP_PORT) == OT_ERROR_NONE);
    otCoapSetDefaultHandler(m_app.p_ot_instance, coap_default_handler, NULL);
}

static void timer_init(void)
{
    uint32_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void leds_init(void)
{
    LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);
}

static void thread_bsp_init(void)
{
    uint32_t err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_thread_init(m_app.p_ot_instance);
    APP_ERROR_CHECK(err_code);
}


// Function starting the internal LFCLK oscillator.
// This is needed by RTC1 which is used by the application timer
// (When SoftDevice is enabled the LFCLK is always running and this is not needed).
static void lfclk_request(void)
{
    uint32_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

// Timeout handler for the repeated timer (WORKS!)
static void timer_a_handler(void * p_context)
{
    //NRF_LOG_INFO("LUX: %d\r\n", m_app.lux);
    //NRF_LOG_INFO("CENTIMETERS: %d\r\n", m_app.centimeters);
    read_lux();
    read_range();
    g_pressure = read_pressure();
    g_temperature = read_temperature();

    nrf_drv_gpiote_out_toggle(BSP_LED_3);
    cloud_data_update();
    address_print(&m_app.cloud_address);
}

// Create timers
static void create_timers()
{   
    uint32_t err_code;

    // Create timers
    err_code = app_timer_create(&m_led_a_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                timer_a_handler);
    APP_ERROR_CHECK(err_code);
}

/***************************************************************************************************
 * @section Main
 **************************************************************************************************/

int main(int argc, char *argv[])
{
    uint32_t err_code;

    // Request LF clock.
    lfclk_request();

    NRF_LOG_INIT(NULL);

    thread_init();
    coap_init();

    timer_init();
    thread_bsp_init();
    leds_init();

    // Create application timers.
    create_timers();
    err_code = app_timer_start(m_led_a_timer_id, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(err_code);

    twi_init();
    m_sample = 1;
    nrf_delay_ms(2);

    lps22hb_setup();
    nrf_delay_ms(2);

    vl6180x_load_settings();

    while (true)
    {
        otTaskletsProcess(m_app.p_ot_instance);
        PlatformProcessDrivers(m_app.p_ot_instance);
    }
}
