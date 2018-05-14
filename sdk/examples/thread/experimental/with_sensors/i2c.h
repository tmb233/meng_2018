#include "nrf_drv_twi.h"

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

/**
 * @brief Function for handling data from sensor.
 *
 * @param[in] cm
 */
__STATIC_INLINE void data_handler(uint8_t mm)
{
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