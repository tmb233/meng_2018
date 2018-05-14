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
#include "nrf_drv_twi.h"
#include "vl6180x.h"


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

/* TWI instance ID. */
#define TWI_INSTANCE_ID     0

/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

static uint8_t m_sample;

uint16_t lux;
uint8_t range;
uint16_t g_lux;
uint8_t g_cm;
uint16_t g_pressure;
uint16_t g_temperature;


static uint8_t set[40][3] = {
    {0x02, 0x07, 0x01},
    {0x02, 0x08, 0x01},
    {0x00, 0x96, 0x00},
    {0x00, 0x97, 0xfd},
    {0x00, 0xe3, 0x00},
    {0x00, 0xe4, 0x04},
    {0x00, 0xe5, 0x02},
    {0x00, 0xe6, 0x01},
    {0x00, 0xe7, 0x03},
    {0x00, 0xf5, 0x02},
    {0x00, 0xd9, 0x05},
    {0x00, 0xdb, 0xce},
    {0x00, 0xdc, 0x03},
    {0x00, 0xdd, 0xf8},
    {0x00, 0x9f, 0x00},
    {0x00, 0xa3, 0x3c},
    {0x00, 0xb7, 0x00},
    {0x00, 0xbb, 0x3c},
    {0x00, 0xb2, 0x09},
    {0x00, 0xca, 0x09},
    {0x01, 0x98, 0x01},
    {0x01, 0xb0, 0x17},
    {0x01, 0xad, 0x00},
    {0x00, 0xff, 0x05},
    {0x01, 0x00, 0x05},
    {0x01, 0x99, 0x05},
    {0x01, 0xa6, 0x1b},
    {0x01, 0xac, 0x3e},
    {0x01, 0xa7, 0x1f},
    {0x00, 0x30, 0x00},
    {0x00, 0x11, 0x10}, // Enables polling for 'New Sample ready'
                        // when measurement completes
    {0x01, 0x0a, 0x30}, // Set the averaging sample period
                        // (compromise between lower noise and
                        // increased execution time)
    {0x00, 0x3f, 0x46}, // Sets the light and dark gain (upper
                        // nibble). Dark gain should not be
                        // changed.
    {0x00, 0x31, 0xFF}, // sets the # of range measurements after
                        // which auto calibration of system is
                        // performed
    {0x00, 0x40, 0x63}, // Set ALS integration time to 100ms
    {0x00, 0x2e, 0x01}, // perform a single temperature calibration
                        // of the ranging sensor
    {0x00, 0x1b, 0x09}, // Set default ranging inter-measurement
                        // period to 100ms
    {0x00, 0x3e, 0x31}, // Set default ALS inter-measurement period
                        // to 500ms
    {0x00, 0x14, 0x24}, // Configures interrupt on 'New Sample
                        // Ready threshold event'
    {0x00, 0x16, 0x00}  // Set fresh out of reset bit
};


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
/*
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

    //NRF_LOG_INFO("%s\r\n", (uint32_t)ipstr);
}
*/
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
/*
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
 }*/
/*
 static void cloud_data_update(void)
 {
     char payload_buffer[64];
     //NRF_LOG_INFO("LUX: %d\r\n", global_lux);
     //NRF_LOG_INFO("CENTIMETERS: %d\r\n", global_centimeters);

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
*/
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
    //uint32_t err_code;
    
    // Check if cloud hostname has been resolved.
    if (m_app.state != APP_STATE_RUNNING)
    {
        return;
    }

    // If IPv6 address of the cloud is unknown try to resolve hostname.
    if (otIp6IsAddressEqual(&m_app.cloud_address, &m_unspecified_ipv6))
    {
        hostname_resolve(m_app.p_ot_instance, m_cloud_hostname);
        return;
    }

    switch (event)
    {
        case BSP_EVENT_KEY_0:
            NRF_LOG_INFO("BSP_EVENT_KEY_0 Callback\r\n");
            if (m_app.lux == LUX_MIN)
            {
                // The minimal temperature has been already reached.
                break;
            }

            // Decrement temperature value.
            m_app.lux -= 1000;

            NRF_LOG_INFO("Decremented temperature to: %d\r\n", m_app.lux);
            break;

        case BSP_EVENT_KEY_1:
            NRF_LOG_INFO("BSP_EVENT_KEY_1 Callback\r\n");
            if (m_app.lux == LUX_MAX)
            {
                // The maximum temperature has been already reached.
                break;
            }

            // Increment temperature value.
            m_app.lux += 1000;

            NRF_LOG_INFO("Incremented temperature to: %d\r\n", m_app.lux);
            break;

        case BSP_EVENT_KEY_2:
            NRF_LOG_INFO("BSP_EVENT_KEY_2 Callback\r\n");
            if (m_app.centimeters == CENTIMETERS_MIN)
            {
                // The minimal temperature has been already reached.
                break;
            }

            // Decrement temperature value.
            m_app.centimeters -= 10;

            NRF_LOG_INFO("Decremented centimeters to: %d\r\n", m_app.centimeters);
            break;

        case BSP_EVENT_KEY_3:
            NRF_LOG_INFO("BSP_EVENT_KEY_3 Callback\r\n");
            if (m_app.centimeters == CENTIMETERS_MAX)
            {
                // The minimal temperature has been already reached.
                break;
            }

            // Decrement temperature value.
            m_app.centimeters += 10;

            NRF_LOG_INFO("Incremented centimeters to: %d\r\n", m_app.centimeters);
            break;

        default:
            return; // no implementation needed
    }
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

    //NRF_LOG_INFO("Thread version: %s\r\n", (uint32_t)otGetVersionString());
    //NRF_LOG_INFO("Network name:   %s\r\n", (uint32_t)otThreadGetNetworkName(p_instance));

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

/**
 * @brief Function for handling data from sensor.
 *
 * @param[in] cm
 */
__STATIC_INLINE void data_handler(uint8_t mm)
{
}

/**
 * @brief TWI events handler.
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                data_handler(m_sample);
            }
            m_xfer_done = true;
            break;

        case NRF_DRV_TWI_EVT_ADDRESS_NACK:
            NRF_LOG_INFO("NRF_DRV_TWI_EVT_ADDRESS_NACK: %x\r\n", m_sample);
            break;

        case NRF_DRV_TWI_EVT_DATA_NACK:
            NRF_LOG_INFO("NRF_DRV_TWI_EVT_DATA_NACK: %x\r\n", m_sample);
            break;

        default:
            break;
    }
}

/**
 * @brief TWI initialization.
 */
void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

uint8_t status(uint8_t status)
{
    uint8_t lps_sample;
    ret_code_t err_code;
    m_xfer_done = false;
    uint8_t addr[1] = {LPS22HB_STATUS_REG};

    int count = 1000;
    uint8_t data = 0xff;
    do {
        err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr, sizeof(addr), false);
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;

        err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &lps_sample, sizeof(lps_sample));
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;
        --count;
        if (count < 0)
            break;
    } while ((data & status) == 0);

    if (count < 0)
        return -1;
    else
        return 0;
}

void read_range(void)
{
    ret_code_t err_code;
    m_sample = 0;
    uint8_t addr3[2] = {0x00, 0x4d};
    while (!(m_sample & 0x01))
    {
        err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr3, sizeof(addr3), false);
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;

        err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, &m_sample, sizeof(m_sample));
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;
    }

    // REG_SYSRANGE_START
    uint8_t addr4[3] = {0x00, 0x18, 0x01};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr4, sizeof(addr4), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;


    m_sample = 0;
    uint8_t addr5[2] = {0x00, 0x4f};
    while (!(m_sample & 0x04))
    {
        err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr5, sizeof(addr5), false);
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;

        err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, &m_sample, sizeof(m_sample));
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;
    }

    range = 0;
    uint8_t addr6[2] = {0x00, 0x62};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr6, sizeof(addr6), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, &range, sizeof(range));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    range = range/10.0;

    //NRF_LOG_INFO("RANGE: %d cm\r\n", range);
    g_cm = range;
    NRF_LOG_INFO("RANGE: %d cm\r\n", g_cm);

    // Clear Interrupt
    uint8_t addr7[3] = {0x00, 0x15, 0x07};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr7, sizeof(addr7), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;
}

void read_lux(void)
{
    ret_code_t err_code;
    uint8_t reg;
    uint8_t gain = VL6180X_ALS_GAIN_5;

    uint8_t addr1[2] = {0x00, 0x14};

    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr1, sizeof(addr1), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, &reg, sizeof(reg));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    reg &= ~0x38;
    reg |= (0x4 << 3); // IRQ on ASL ready
    uint8_t addr2[3] = {0x00, 0x14, reg};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    uint8_t addr3[3] = {0x00, 0x40, 0x00};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr3, sizeof(addr3), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    uint8_t addr4[3] = {0x00, 0x41, 0x64};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr4, sizeof(addr4), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    uint8_t addr5[3] = {0x00, 0x3f, 0x40 | gain};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr5, sizeof(addr5), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    // Start ALS
    uint8_t addr6[3] = {0x00, 0x38, 0x01 | gain};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr6, sizeof(addr6), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    // Poll until new sample ready threshold event is set
    reg = 0;
    uint8_t addr7[2] = {0x00, 0x4f};
    while (((reg >> 3) & 0x07) != 4)
    {
        err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr7, sizeof(addr7), false);
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;

        err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, &reg, sizeof(reg));
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;
    }

    uint8_t lux_bytes[2] = {0x00, 0x00};
    uint8_t addr8[2] = {0x00, 0x50};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr8, sizeof(addr8), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, lux_bytes, sizeof(lux_bytes));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    // Clear Interrupt
    uint8_t addr9[3] = {0x00, 0x15, 0x07};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr9, sizeof(addr9), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    lux = 0;
    lux = (lux_bytes[0] << 8) | lux_bytes[1];

    lux *= 0.32; // calibrated count/lux
    switch(gain) { 
    case VL6180X_ALS_GAIN_1: 
      break;
    case VL6180X_ALS_GAIN_1_25: 
      lux /= 1.25;
      break;
    case VL6180X_ALS_GAIN_1_67: 
      lux /= 1.76;
      break;
    case VL6180X_ALS_GAIN_2_5: 
      lux /= 2.5;
      break;
    case VL6180X_ALS_GAIN_5: 
      lux /= 5;
      break;
    case VL6180X_ALS_GAIN_10: 
      lux /= 10;
      break;
    case VL6180X_ALS_GAIN_20: 
      lux /= 20;
      break;
    case VL6180X_ALS_GAIN_40: 
      lux /= 20;
      break;
    }
    lux *= 100;
    lux /= 100; // integration time in ms
    g_lux = lux;
    NRF_LOG_INFO("LUX: %d\r\n", g_lux);
}

float read_pressure(void)
{
    uint8_t press_out_h;
    uint8_t press_out_l;
    uint8_t press_out_xl;
    ret_code_t err_code;
    m_xfer_done = false;

    uint8_t addr[2] = {LPS22HB_CTRL_REG2,0x01};
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr, sizeof(addr), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    if (status(0x01) < 0)
    {
        return 1.23;
    }

    uint8_t addr2[1] = {LPS22HB_PRES_OUT_H};
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &press_out_h, sizeof(press_out_h));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    addr2[0] = LPS22HB_PRES_OUT_L;
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &press_out_l, sizeof(press_out_l));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    addr2[0] = LPS22HB_PRES_OUT_XL;
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &press_out_xl, sizeof(press_out_xl));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    long val = (((long)press_out_h << 24) | ((long)press_out_l << 16) | ((long)press_out_xl << 8)) >> 8;
    val = val/4096.0f;
    NRF_LOG_INFO("PRESSURE: %d\r\n", val);
    return val;
}

float read_temperature(void)
{
    uint8_t temp_out_h;
    uint8_t temp_out_l;
    ret_code_t err_code;
    m_xfer_done = false;

    uint8_t addr[2] = {LPS22HB_CTRL_REG2,0x01};
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr, sizeof(addr), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    if (status(0x2) < 0)
    {
        return 4.56;
    }

    uint8_t addr2[1] = {LPS22HB_TEMP_OUT_H};
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &temp_out_h, sizeof(temp_out_h));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    addr2[0] = LPS22HB_TEMP_OUT_L;
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &temp_out_l, sizeof(temp_out_l));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    uint16_t val = ((temp_out_h << 8) | (temp_out_l & 0xff));
    val = ((float)val)/100.0f;
    NRF_LOG_INFO("TEMPERATURE: %d\r\n", val);
    return val;
}


void lps22hb_setup(void)
{
    uint8_t lps_sample;
    ret_code_t err_code;
    m_xfer_done = false;

    uint8_t addr[2] = {LPS22HB_RES_CONF,0x00};
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr, sizeof(addr), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    addr[0] = LPS22HB_CTRL_REG1;
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr, sizeof(addr), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    uint8_t addr2[1] = {LPS22HB_WHO_AM_I};
    err_code = nrf_drv_twi_tx(&m_twi, LPS22HB_ADDR, addr2, sizeof(addr2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, LPS22HB_ADDR, &lps_sample, sizeof(lps_sample));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;
}

/**
 * @brief TWI initialization.
 */
void vl6180x_load_settings (void)
{
    ret_code_t err_code;
    m_xfer_done = false;

    uint8_t addr1[2] = {0x00, 0x00};
    err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, addr1, sizeof(addr1), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    err_code = nrf_drv_twi_rx(&m_twi, VL6180X_ADDR, &m_sample, sizeof(m_sample));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    m_xfer_done = false;

    for (int i = 0; i <= 39; i++)
    {
        err_code = nrf_drv_twi_tx(&m_twi, VL6180X_ADDR, set[i], sizeof(set[i]), false);
        APP_ERROR_CHECK(err_code);
        while (m_xfer_done == false);
        m_xfer_done = false;
    }

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
/*
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
*/
/***************************************************************************************************
 * @section Main
 **************************************************************************************************/

int main(int argc, char *argv[])
{
    //uint32_t err_code;

    // Request LF clock.
    lfclk_request();

    NRF_LOG_INIT(NULL);

    thread_init();
    coap_init();

    timer_init();
    thread_bsp_init();
    leds_init();

    // Create application timers.
    //create_timers();
    //err_code = app_timer_start(m_led_a_timer_id, APP_TIMER_TICKS(2000), NULL);
    //APP_ERROR_CHECK(err_code);

    //twi_init();
    m_sample = 1;
    nrf_delay_ms(2);

    //lps22hb_setup();
    nrf_delay_ms(2);

    //vl6180x_load_settings();

    while (true)
    {
        otTaskletsProcess(m_app.p_ot_instance);
        PlatformProcessDrivers(m_app.p_ot_instance);
    }
}

/**
 *@}
 **/
