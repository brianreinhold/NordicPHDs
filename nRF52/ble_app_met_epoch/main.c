/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/** @file
 *
 * @brief Generic Health Sensor Service Sample Application main file.
 *
 * How it works:
 *
 * The basic idea is that the application developer decides what measurements the device
 * is going to generate and report, and a template (array of bytes) is created that
 * supports those measurements. The template follows the MET experimental protoype.
 *
 * When measurement data is received, the updated values are inserted into the template
 * at their proper locations and then the array is pushed over the tunnel to the client.
 *
 * Now for practical matters. I have no sensor equipment or the capability to integrate a
 * sensor into the Development Kit (DK) boards on my kitchen table. So this demo generates fake
 * data. Pressing a button on the DK generates a measurement that is placed into stored data.
 * Every botton press generates a new record. Of course it is fake and the time clock is used
 * to 'randomly' vary a measurement about some value. This button press simulates data being
 * received from a sensor while not connected.
 *
 * Pressing another button starts advertisement. It also starts a timer than every N milliseconds
 * generating fake 'live' data. It is placed in a queue. When live data is requested, the
 * measurements in that queue are sent.
 *
 * WHAT IS THE FORM OF THE SENSOR DATA?
 * Every sensor is different and every sensor application is different. It is assumed that when
 * a real sensor MCU is integrated with the Noridc BT chip that sensor data will be received
 * over a UART or SPI. The fake data generation of this code is commented out. The application 
 * then defines a structure that contains the data sent by the MCU. In this demo, an example
 * of an application structure is s_MsmtDataBp for the blood pressure. It is totally arbitrary.
 * When it is time to send the data, the application has to call the update methods to place
 * this data into the template. This is done in encodeSpecializationMsmts(). The application
 * has to take the structure data and use that to populate the update methods.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "nordic_common.h"
#include "nrf.h"
//#include "nrf_nvic.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdm.h"
#include "nrf_sdh.h"

#include "app_timer.h"
#include "app_gpiote.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
//#include "app_uart.h"
#include "ble_conn_state.h"

#include "ble_gap.h"
#include "ble_gatts.h"

#include "btle_utils.h"
#include "configMetEncoder.h"
#include "nomenclature.h"
#include "msmt_queue.h"
#include "handleSpecializations.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
// lint -e553
#ifdef SVCALL_AS_NORMAL_FUNCTION
#include "ser_phy_debug_app.h"
#endif

#define APP_GPIOTE_MAX_USERS            1                                           /**< Maximum number of users of the GPIOTE handler. */
#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                         /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */


//#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

//#define APP_ADV_INTERVAL                40                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
//#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout in units of seconds. */

//#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
//#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.5 seconds) */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of indication) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(3000)                       /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define MET_COMMAND_DELAY               APP_TIMER_TICKS(50)
#if (SPIROMETER == 1)
    #define MET_LIVE_DELAY              APP_TIMER_TICKS(5000)
#else
    #define MET_LIVE_DELAY              APP_TIMER_TICKS(1000)
#endif
#define CMD_SENSOR_TIME                 APP_TIMER_TICKS(10000)

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

// Colors only valid on the nRF52840 dongle. On the DK there are four different LEDs all of which are yellow.
// LED_0 is the other LED which is green-yellow
// LED_1 is red
// LED_2 is green
// LED_3 is blue

#define ADVERTISING_LED                 BSP_BOARD_LED_2                         /**< Is on when device is advertising. */ 
#define ERROR_LED                       BSP_BOARD_LED_1
#define CONNECTED_LED                   BSP_BOARD_LED_3                         /**< Is on when device has connected.*/
#define MSMT_DATA_LED                  (BSP_BOARD_LED_3 | BSP_BOARD_LED_1)      /**< Flashes pink when Keiser data is received and queued. */
#define DISCONNECTED_LED                BSP_BOARD_LED_0


static unsigned long elapsedTimeStart;

static unsigned short mtu_size                                  = BLE_GATT_ATT_MTU_DEFAULT;
static unsigned short data_length                               = 27;  // Max size of advertisement?
static bool flash_write_needed                                  = true;

APP_TIMER_DEF(m_met_disconnect_timer_id);      /**< disconnect handler */
APP_TIMER_DEF(m_met_live_data_timer_id);
APP_TIMER_DEF(m_met_flash_write_timer_id);
APP_TIMER_DEF(m_app_dummy_timer_id);

//nrf_nvic_state_t nrf_nvic_state = {0};      // Docs say this is needed in some C file to use NVIC via SoftDevice - we use system reset

static uint8_t                  m_adv_handle                    = 0;  // For advertisments
static uint32_t                 m_config_id                     = 1;  // For advertisments
static bool                     stored_data_done_sent;

static uint16_t                 m_connection_handle             = BLE_CONN_HANDLE_INVALID;     /**< Handle of the current connection. */
static uint16_t                 m_met_service_handle            = BLE_GATT_HANDLE_INVALID;
static ble_gatts_char_handles_t m_met_cp_handle;
static ble_gatts_char_handles_t m_met_response_handle;

#define COMMAND_CHAR 0
#define RESPONSE_CHAR 1
static uint8_t                  cp_write[18];
static unsigned char            cccdSet[2]                      = {0, 0};
static unsigned short           noOfCccds                       = 2;
static bool                     hasEncrypted                    = false;
static bool                     send_live_data                  = false;
static unsigned long            live_data_count                 = 0;
static unsigned short           timeSync                        = 0x1F00;       // Unsynchronized
static volatile bool            met_abort                       = false;
static ble_gap_sec_params_t     own_sec_params;
static ble_gap_sec_params_t     peer_sec_params;
static ble_gap_sec_keyset_t     keys;
static uint16_t                 saveDataLength                  = 0;
static uint8_t                  *saveDataBuffer                 = NULL;
static volatile bool            restartAdv                      = false;
ble_gap_conn_params_t           gap_conn_params;
nrf_mutex_t p_mutex;
nrf_mutex_t q_mutex;
//static __align(4) uint32_t evt_buf[CEIL_DIV(BLE_STACK_EVT_MSG_BUF_SIZE, sizeof(uint32_t))];
__ALIGN(4) uint8_t *evt_buf;
__ALIGN(4) uint8_t *evt_buf2;

static uint8_t cpResponse[18];

//=========================== PARAMETERS FOR MET DATA
s_Queue *queue;

/*
 This MET sends indications on two characteristics. These variables tell the indicate data method
 which handles (characteristics) to send indications on.
 When the PHD receives a command, the MET will do one of two things:
    1. indicate a response on the control point characteristic indicating the command has been handled.
       The PHG may send the next command.
    2. indicate a one or more responses on the response characteristic (the response may be fragmented over
       several indications, such as the systemInfo and the measurement groups)
       then indicate a response on the control point characteristic - usually this means the PHD is done
       handling the command, but it may also indicate a measurement group has been completed but more are
       to come. In that case the PHG just handles the measurement group and waits for more. It shall NOT
       send another command. The PHD will send a final response indicating the measurement groups are completed,
       ending the command. Now the PHG may send the next command.
       
       In the live data situation, the command never completes. The PHD will just send measurement groups
       on the response characteristic followed by group finished response on the control point and then
       continue with the next. When the PHD has no more data to send it disconnects. 
 */

volatile s_global_send global_send;
static volatile bool send_flag = false;

extern unsigned short pairing;          // Value of 1 indicates that pairing/bonding is required.
extern unsigned char batteryCharValue;
extern s_SystemInfo *systemInfo;
extern char *DEVICE_NAME;
extern unsigned short SPECIALIZATION;
extern unsigned short BLE_APPEARANCE;

// ------------- Current time and time info

static unsigned char cont_response[] = 
{
    (COMMAND_SEND_LIVE_DATA & 0xFF),
    ((COMMAND_SEND_LIVE_DATA >> 8) & 0xFF),         // Place for the command which will always be transfer live data
    0x00, ((HEADER_FLAGS_OPTIMIZED_RECORD_DONE >> 8) & 0xFF),// flags (no time stamp) subsequent message done
    0x01, 0x00,                                     // total length of the measurement group (123 - 6)
    0x01                                            // group id
};

unsigned long prevCount = 0;
static unsigned long long cycles = 0;
unsigned long long accumulatedCycleCount = 0;
unsigned long long getRtcCount()  // Returns the number of elapsed ticks where 32768 ticks is one second
{
    unsigned long  count = app_timer_cnt_get();
    if (count < prevCount)
    {
        cycles++;
        accumulatedCycleCount = accumulatedCycleCount + (unsigned long long)(APP_TIMER_MAX_CNT_VAL);
        NRF_LOG_DEBUG("Time cycled. Cycles %u  accumulatedCycleCount %u", cycles, accumulatedCycleCount);
    }
    prevCount = count;
    return (unsigned long long)count + accumulatedCycleCount;
}

/*
 * Clock has 32768 ticks per second
 * 1 tick = 30.517578125 microseconds
 *          .030517578125 milliseconds
 *          .000030517578 seconds
 *  seconds = (ticks + 16384) /32768
 *  10ths of milliseconds = (ticks + 1.6384) / 3.2768
 *  milliseconds = (ticks + 16.384) / 32.768
 * or
 *  xth seconds = (FACTOR * ticks + 16384) / 32768  (this gives rounds when using integer divides)
       where FACTOR = 1 for seconds, 10 for tenths, 100 for hundredths, and 1000 for milliseconds
 */
static unsigned long long getRtcTicks()
{
    unsigned long long count = getRtcCount() - elapsedTimeStart;
    return ((count * factor + 16384) / 32768);
}

static unsigned long getTicks()
{
    unsigned long long count = getRtcCount() - elapsedTimeStart;
    return (unsigned long)((count * 1000 + 16384) / 32768);
}

static void sec_params_init(void)
{
    own_sec_params.lesc             = SEC_PARAM_LESC;
    own_sec_params.keypress         = SEC_PARAM_KEYPRESS;
    own_sec_params.bond             = SEC_PARAM_BOND;
    own_sec_params.mitm             = SEC_PARAM_MITM;
    own_sec_params.io_caps          = SEC_PARAM_IO_CAPABILITIES;
    own_sec_params.oob              = SEC_PARAM_OOB;
    own_sec_params.min_key_size     = SEC_PARAM_MIN_KEY_SIZE;
    own_sec_params.max_key_size     = SEC_PARAM_MAX_KEY_SIZE;
    own_sec_params.kdist_peer.enc   = 1;     // peer to send their encryption key;
    own_sec_params.kdist_peer.id    = 1;     // peer to send its IRK
    own_sec_params.kdist_peer.link  = 0;     // peer to derive link key from LTK - This must be 0 or you will get an NRF_ERROR_NOT_SUPPORTED error
    own_sec_params.kdist_peer.sign  = 0;     // peer to send signature resolving key - This must be 0 or you will get an NRF_ERROR_NOT_SUPPORTED error
    own_sec_params.kdist_own.enc    = 1;     // We distribute our LTK
    own_sec_params.kdist_own.id     = 1;     // We distribute our IRK (even though we don't use resolvable random addressing
    own_sec_params.kdist_own.link   = 0;     // we are to derive link key from LTK - This must be 0 or you will get an NRF_ERROR_NOT_SUPPORTED error
    own_sec_params.kdist_own.sign   = 0;     // We to send signature resolving key - This must be 0 or you will get an NRF_ERROR_NOT_SUPPORTED error
}

#define ADVERTISING_INTERVAL_40_MS 64  /**< 0.625 ms = 40 ms */
#define BUFFER_SIZE 30

static ble_gap_adv_params_t     m_adv_params;
static ble_gap_adv_data_t       m_adv_data;
static uint8_t                  data_buffer[BUFFER_SIZE];
static uint8_t                  scan_rsp_buffer[BUFFER_SIZE];
ret_code_t advertisement_met_set(uint8_t *advHandle)
{
    ret_code_t error_code;
    uint8_t  index = 0;
    uint8_t  scan_index = 0;
    char printBuf[256];

    memset(data_buffer, 0, sizeof(data_buffer));
    memset(scan_rsp_buffer, 0, sizeof(scan_rsp_buffer));

    const uint8_t name_length = (uint8_t)strlen(DEVICE_NAME);

    // Set the flags - important to indicate no EDR/BR support for some Androids
    // Actually by spec one is SUPPOSED to set one of the discoverable flags to be discoverable.
    data_buffer[index++] = 2;        // length of flags data type
    data_buffer[index++] = BLE_GAP_AD_TYPE_FLAGS;
    data_buffer[index++] = BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;    // No EDR/BR support and general discoverable

    scan_rsp_buffer[scan_index++] = 3;        // length of appearance data type
    scan_rsp_buffer[scan_index++] = BLE_GAP_AD_TYPE_APPEARANCE;
    scan_rsp_buffer[scan_index++] = (BLE_APPEARANCE & 0xFF);
    scan_rsp_buffer[scan_index++] = ((BLE_APPEARANCE >> 8) & 0xFF);

    // Set the device name.
    if (DEVICE_NAME != NULL)
    {
        scan_rsp_buffer[scan_index++] = name_length + 1; // Device name + data type
        scan_rsp_buffer[scan_index++] = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
        memcpy((char*)&scan_rsp_buffer[scan_index++], DEVICE_NAME, name_length);  // destination, source, length
        scan_index += name_length;
    }

    // Set the device's service.
    data_buffer[index++] = 1 + 2;
    data_buffer[index++] = BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE;
    // Store in little-endian format.
    data_buffer[index++] = BTLE_MET_SERVICE & 0xFF;
    data_buffer[index++] = (BTLE_MET_SERVICE & 0xFF00) >> 8;
    // Add the Service Data for the specializations
    data_buffer[index++] = 1 + 2 + 1 + 2 * 1 + 1; // numberOfSpecializations + pairing option;
    data_buffer[index++] = BLE_GAP_AD_TYPE_SERVICE_DATA;
    // Store in little-endian format.
    data_buffer[index++] = BTLE_MET_SERVICE & 0xFF;
    data_buffer[index++] = (BTLE_MET_SERVICE & 0xFF00) >> 8;

    data_buffer[index++] = 1;

    data_buffer[index++] = (SPECIALIZATION & 0xFF);
    data_buffer[index++] = ((SPECIALIZATION >> 8) & 0xFF);
    data_buffer[index++] = (unsigned char)pairing;        // Support pairing

    ble_gap_adv_properties_t adv_properties;
    adv_properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_properties.anonymous = 0;
    adv_properties.include_tx_power = 0;

    memset(&m_adv_params, 0, sizeof(ble_gap_adv_params_t));
    m_adv_params.properties = adv_properties;
    m_adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    m_adv_params.duration = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
    m_adv_params.p_peer_addr = NULL;
    m_adv_params.interval = ADVERTISING_INTERVAL_40_MS;
    m_adv_params.max_adv_evts = 0;
    m_adv_params.primary_phy = BLE_GAP_PHY_AUTO;
    m_adv_params.secondary_phy = BLE_GAP_PHY_AUTO;
    m_adv_params.channel_mask[0] = 0;
    m_adv_params.channel_mask[1] = 0;
    m_adv_params.channel_mask[2] = 0;
    m_adv_params.channel_mask[3] = 0;
    m_adv_params.channel_mask[4] = 0;

    memset(&m_adv_data, 0, sizeof(m_adv_data));

    m_adv_data.adv_data.p_data = data_buffer;
    m_adv_data.adv_data.len = index;
    m_adv_data.scan_rsp_data.p_data = scan_rsp_buffer;
    m_adv_data.scan_rsp_data.len = scan_index;

    memset(printBuf, 0, 256);
    NRF_LOG_INFO("Advertisement is %s", (uint32_t)byteToHex(data_buffer, printBuf, " ", index));
    NRF_LOG_FLUSH();
    if (scan_index > 0)
    {
        memset(printBuf, 0, 256);
        NRF_LOG_INFO("Scan response is: %s", (uint32_t)byteToHex(scan_rsp_buffer, printBuf, " ", scan_index));
        NRF_LOG_FLUSH();
    }

    error_code = sd_ble_gap_adv_set_configure(advHandle, &m_adv_data, &m_adv_params);

    if (error_code != NRF_SUCCESS)
    {
        if (error_code == 10)
        {
            NRF_LOG_DEBUG("Failed to set advertisement data. Error code: invalid flags");
        }
        else
        {
            NRF_LOG_DEBUG("Failed to set advertisement data. Error code: 0x%02X", error_code);
        }
        return error_code;
    }
    return NRF_SUCCESS;
}

static uint32_t advertising_start()
{
    ret_code_t             error_code;

    error_code = sd_ble_gap_adv_start(m_adv_handle, m_config_id);

    if (error_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Failed to start advertising. Error code: 0x%02X", error_code);
        return error_code;
    }

    NRF_LOG_INFO("Started advertising at time %u", getTicks());
    return NRF_SUCCESS;
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

void ble_disconnected_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t  err_code;
    
    if (m_connection_handle != BLE_CONN_HANDLE_INVALID)
    {
        met_abort = true;
        err_code = sd_ble_gap_disconnect(m_connection_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
    }
}

void dummy(void * p_context)
{
    UNUSED_PARAMETER(p_context);
}

static void command_handler(uint16_t index);
static void write_flash(void * p_context);
static void live_data_handler(void * p_context);
static void timers_init(void)
{
    ret_code_t err_code;

    // Initialize timer module, making it use the scheduler.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_met_disconnect_timer_id,
                            APP_TIMER_MODE_SINGLE_SHOT,
                            ble_disconnected_handler);

    err_code = app_timer_create(&m_met_flash_write_timer_id,
                            APP_TIMER_MODE_SINGLE_SHOT,
                            write_flash);
    err_code = app_timer_create(&m_met_live_data_timer_id,
                            APP_TIMER_MODE_REPEATED,
                            live_data_handler);
    err_code = app_timer_create(&m_app_dummy_timer_id,
                            APP_TIMER_MODE_REPEATED,
                            (void *)getRtcCount);
    APP_ERROR_CHECK(err_code);

}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_connection_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static uint32_t conn_params_init(void)
{
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = &gap_conn_params;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    // Set the connection params in stack
    return sd_ble_gap_ppcp_set(cp_init.p_conn_params);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
/*
static void sleep_mode_enter(void)
{
    ret_code_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);

    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}
*/
#if (USES_TIMESTAMP == 1)
    static void getCurrentTime()
    {
        char buffer[64];
        updateCurrentTimeEpoch(&sTimeInfoData, getRtcTicks() + epoch);
        memset(buffer, 0, 64);
        NRF_LOG_INFO("Getting the current time info: %s", (uint32_t)byteToHex(sTimeInfoData->timeInfoBuf, buffer, " ", sTimeInfoData->dataLength));
        NRF_LOG_FLUSH();
        NRF_LOG_DEBUG("Current Epoch time %llu", (getRtcTicks() + epoch));
    }
#endif

static void handleSetTime(unsigned char *setTime);

static void print_command(char *str)
{
    NRF_LOG_INFO("====> Received %s (%u) command at time %u.", str, global_send.current_command, getTicks());
}

static bool do_auth_write_request(ble_evt_t * p_ble_evt)
{
    NRF_LOG_DEBUG("Read write authorized request signaled");
    if (p_ble_evt->evt.gatts_evt.params.authorize_request.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
    {
        NRF_LOG_DEBUG("Write authorized request at time %u", getTicks());
        ret_code_t err_code;
        ble_gatts_rw_authorize_reply_params_t reply;
        memset(&reply, 0, sizeof(ble_gatts_rw_authorize_reply_params_t));
        reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;

        // Command from Control Point
        if (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.handle == m_met_cp_handle.value_handle)
        {
            if (cccdSet[COMMAND_CHAR])
            {
                uint16_t index = 0;
                reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
                reply.params.write.update = 1;
                reply.params.write.len = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.len;
                reply.params.write.p_data = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data;
                err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle, &reply);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("RW GHS CP reply gave error %u:", err_code);
                }
                uint16_t command = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data[0] +
                    (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data[1] << 8);
                global_send.current_command = command;
                NRF_LOG_DEBUG("Command %d written on CP at time %u", command, getTicks());

                #if (USES_TIMESTAMP == 1)
                    if (command == COMMAND_SET_CURRENT_TIME)
                    {
                        print_command("Set Current Time");
                        handleSetTime(&p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data[2]);
                    }
                #endif
                #if (USES_STORED_DATA == 1)
                    if (command == COMMAND_GET_STORED_RECORDS_BY_INDEX)
                        index = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data[2] +
                            (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data[3] << 8);
                #endif
                command_handler(index);
                return true;
            }
            else
            {
                NRF_LOG_ERROR("Client has not enabled the Command characteristic");
                reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR;
            }
        }
        err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle, &reply);
        return true;
    }
    else
    {
        NRF_LOG_DEBUG("Received an authorized RW request at time %u with type %u", getTicks(), p_ble_evt->evt.gatts_evt.params.authorize_request.type);
    }
    return false;
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static bool on_generic_ble_evt(ble_evt_t * p_ble_evt)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        // MET - Should never happen
        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(m_connection_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break; // BLE_EVT_USER_MEM_REQUEST

        // MET - needed for stupid client write before enabling
        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            if (do_auth_write_request(p_ble_evt))
                break;
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } 
        break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

        default:
            // No implementation needed.
            return false;
    }
    return true;
}

static void print_send_data(unsigned short length, unsigned char *data)
{
    NRF_LOG_INFO("=====> Sending %u bytes of data at time %u: ", length, getTicks());
    // All this crap is just to print the data being sent in 32-byte chunks
    int j = 0;
    int size = (length > 32) ? 32 : length;
    char buffer[104];
    while (true)
    {
        memset(buffer, 0, 104);
        if ((j + 32) > length)
        {
            size = length - j;
        }
        NRF_LOG_INFO("%s", (uint32_t)byteToHex(&data[j], buffer, " ", size));
        NRF_LOG_FLUSH();
        j = j + size;
        if (j >= length) break;
    }
}

static ret_code_t send_data()
{
    if (global_send.data_length == 0 || !send_flag)   // Nothing to send
    {
        return NRF_SUCCESS;
    }
    ret_code_t             error_code;
    uint16_t               hvx_length;
    ble_gatts_hvx_params_t hvx_params;
    if(sd_mutex_acquire(&p_mutex) == NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN)
    {
        NRF_LOG_DEBUG("Mutex locked");
        return NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN;
    }
    int                    count = 1;   // For Debug informational output only
    //  Sole purpose of this if statement is to print the data being sent
    if (global_send.offset == 0 && send_flag)
    {
        print_send_data(global_send.data_length, global_send.data);
    }
    send_flag = false;

    while(true)
    {
        if (met_abort // connection has been terminated by PHG
            || (m_connection_handle == BLE_CONN_HANDLE_INVALID))    // the connection has been killed
        {
            NRF_LOG_DEBUG("=====> Aborted or connection handle invalid");
            emptyQueue(queue);
            error_code = NRF_ERROR_INVALID_STATE;
            break;
        }
        // If amount of data exceeds max size, send chunk sized length of data
        if (global_send.data_length - global_send.offset > global_send.chunk_size) //(mtu_size - OPCODE_LENGTH - HANDLE_LENGTH))
        {
            hvx_length = global_send.chunk_size; //(mtu_size - OPCODE_LENGTH - HANDLE_LENGTH);
        }
        // If the amount is less than that value set it to the amount left.
        else
        {
            hvx_length = global_send.data_length - global_send.offset;
        }
        NRF_LOG_DEBUG("=====> Send # %u: Send %u bytes of %u total from offset %u at time %u",
            count, hvx_length, global_send.data_length, global_send.offset, getTicks());
        count++;

        hvx_params.handle = global_send.handle;
        hvx_params.type = (global_send.handle == m_met_response_handle.value_handle) ? cccdSet[RESPONSE_CHAR] : BLE_GATT_HVX_INDICATION;
        hvx_params.offset = 0; // global_send_offset;
        hvx_params.p_len = &hvx_length;
        hvx_params.p_data = (unsigned char *)(global_send.data + global_send.offset);

        // Send indication or notification
        print_send_data(hvx_length, (unsigned char *)hvx_params.p_data); // Prints just the fragment
        error_code = sd_ble_gatts_hvx(m_connection_handle, &hvx_params);
        if (error_code == NRF_SUCCESS)
        {
            // This is for notifications. We need to make sure all notifications are accounted for before indicating that the 
            // record is complete. So it increments here, and decrements in the BLE_EVT_TX_COMPLETE event. The event may
            // contain more than one packet sent.
            global_send.chunks_outstanding++;
            // Update the position in the buffer for the next send
            global_send.offset = global_send.offset + *hvx_params.p_len;
            // If that update exceeds the buffer size, we are done
            if (global_send.offset >= global_send.data_length)
            {
                NRF_LOG_DEBUG("=====> Entire package sent");
                error_code = NRF_SUCCESS;
                break;
            }
            // If an indication we need to wait for the BLE_GATTS_EVT_HVC before the next send
            if (hvx_params.type == BLE_GATT_HVX_INDICATION)
            {
                error_code = NRF_SUCCESS;
                break; // Wait for event
            }
            // We have more TX buffers so send the next hunk
            //NRF_LOG_DEBUG("=====> TX buffer still available");
            continue;
        }
        // This will only happen for indications. Here we simply loop on redoing the send calls
        // until the send goes, then we can advance the buffer and wait for the BLE_GATTS_EVT_HVC event.
        else if (error_code == NRF_ERROR_BUSY)  // Indications only
        {
            while (sd_ble_gatts_hvx(m_connection_handle, &hvx_params) == NRF_ERROR_BUSY);  // keep calling until not busy.
            global_send.offset = global_send.offset + *hvx_params.p_len;
            global_send.chunks_outstanding++;
            error_code = NRF_SUCCESS;
            break;    // Wait for event
        }
        // If we have no buffers, we have to wait for the BLE_EVT_TX_COMPLETE event and resend
        else if( error_code == NRF_ERROR_RESOURCES) // Notifications only
        {
            NRF_LOG_DEBUG("=====> No TX buffers; wait for event and resend.");
            //global_send.chunks_outstanding++;
            error_code = NRF_SUCCESS;
            break;
        }
        // Hopefully this does not happen.
        else
        {
            NRF_LOG_ERROR("=====> Failed doing the indication. Error code: 0x%02X", error_code);
            global_send.data_length = 0;
            global_send.offset = 0;
            global_send.data = NULL;
            global_send.handle = 0;
            emptyQueue(queue);
            break;
        }
    }
    sd_mutex_release(&p_mutex);
    return error_code;
}

static void createContinuousRecordDone()
{
    global_send.chunks_outstanding = 0;
    global_send.handle = m_met_response_handle.value_handle;
    global_send.data = cont_response;
    global_send.data_length = 7;
    global_send.offset = 0;
    global_send.continuous_stage = CONT_RECORD_DONE;
}

static void createCpResponse(uint16_t error, uint16_t *response)
{
    global_send.chunks_outstanding = 0;
    global_send.handle = m_met_cp_handle.value_handle;
    // Put command into CP response
    cpResponse[0] = (global_send.current_command & 0xFF);
    cpResponse[1] = ((global_send.current_command >> 8) & 0xFF);
    // put result
    cpResponse[2] = (error & 0xFF);
    cpResponse[3] = ((error >> 8) & 0xFF);
    global_send.data_length = 4;
    if (response != NULL)
    {
        NRF_LOG_DEBUG("Response field non NULL: value 0x%u", *response);
        cpResponse[4] = (*response & 0xFF);
        cpResponse[5] = ((*response >> 8) & 0xFF);
        global_send.data_length = 6;
        #if (USES_STORED_DATA == 1)
            if (global_send.current_command == COMMAND_GET_NUMBER_OF_STORED_RECORDS)
            {
                populate_epoch_range_of_stored_data(&cpResponse[6]);
            }
            global_send.data_length = 18;
        #endif
    }
    global_send.data = cpResponse;
    global_send.offset = 0;
}

bool readyToSend()
{
    return (                                      // If PHG has given the send live data command and
        m_connection_handle != BLE_CONN_HANDLE_INVALID   // the connection is valid and
        && hasEncrypted                                     // encryption has been done
        && cccdSet[RESPONSE_CHAR]);                                      // the response characteristic has been enabled
}


/**
  We assume there is some kind of interrupt triggered by the sensor that sends measurement data to this
  module. Those values are loaded into the struct s_MsmtData liveMsmt. Then this method is called to set
  up the global_send parameters.

  Live measurements can only be sent after the PHG has given the command to send live measurements.
 */
bool prepareMeasurements(s_MsmtGroupData *msmtGroupData)
{
    if (readyToSend())
    {
        global_send.data = msmtGroupData->data;

        // Set up parameters for indication of this PDU - likely in fragments
        global_send.chunks_outstanding = 0;
        global_send.handle = m_met_response_handle.value_handle;
        global_send.offset = 0;
        global_send.data_length = (unsigned short)msmtGroupData->dataLength;
        global_send.continuous_stage = CONT_NONE;
        return true;
    }
    return false;
}

/**
  This 
 */
#if (USES_STORED_DATA == 1)
void sendStoredMeasurements(unsigned short stored_count)
{
    if (m_connection_handle != BLE_CONN_HANDLE_INVALID && cccdSet[RESPONSE_CHAR])
    {
        sendStoredSpecializationMsmts(stored_count);
        bsp_board_led_on(MSMT_DATA_LED);
    }
}
#endif

static bool encodeMsmtData(void *data)
{
    if (global_send.handle != 0)  // If the handle is non zero bytes are being sent and it is not yet done
    {
        if (!met_abort)
        {
            emptyQueue(queue);
        }
        NRF_LOG_DEBUG("Not ready for measurement # %lu", live_data_count);
        return false;
    }
    if (send_flag)  // A packet is still in the process of being sent
    {
        NRF_LOG_DEBUG("Not ready for live measurement # %lu, still sending", live_data_count);
        return false;
    }
    return encodeSpecializationMsmts(data);
}

/*
 * For live data I have a timer that triggers a call to this method every MET_LIVE_DELAY.
 * Fake measurements are generated based upon the current time tick in some manner. May not be the
 * greatest approach since the time interval is often a nice even one so one gets the same measurement.
 * That's a minor problem. This is called repeatedly but only generates measurements after the 
 * PHG has given the command to send live data, indicated by 'send_live_data' being true.
 */
static void live_data_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    if (send_live_data && !met_abort)
    {
        live_data_count++;
        NRF_LOG_DEBUG("live data generator called at timestamp32 %lu", getTicks());
        if (isFull(queue))
        {
            NRF_LOG_DEBUG("Queue full - Not ready for measurement %lu at time %lu", live_data_count, getTicks());
            return;
        }
        #if (USE_DK == 0)
        {
            if (live_data_count > LIVE_COUNT_MAX)
            {
                NRF_LOG_DEBUG("Sending disconnect");
                met_abort = false;
                app_timer_start(m_met_disconnect_timer_id, MET_COMMAND_DELAY, NULL);
                app_timer_stop(m_met_live_data_timer_id);
                return;
            }
        }
        #endif
        generateLiveDataForSpecializations(live_data_count, getRtcTicks(), getTicks());
        bsp_board_led_on(MSMT_DATA_LED);
    }
}

/**
  Command handler. This method is called when the PHG writes a command on the MET control point.
 */
static void command_handler(uint16_t index)
{
    // TODO  figure out why this is here (incase mtu size changes from client request?)
    global_send.chunk_size = (mtu_size - OPCODE_LENGTH - HANDLE_LENGTH); // 

    switch (global_send.current_command)
    {

    case COMMAND_GET_SYS_INFO:
    {
        print_command("Get System Info");
        global_send.chunks_outstanding = 0;
        global_send.data_length = systemInfoData->dataLength;
        global_send.data = systemInfoData->systemInfoBuf;
        global_send.offset = 0;
        global_send.handle = m_met_response_handle.value_handle;
        global_send.continuous_stage = CONT_NONE;
        send_flag = true;
    }
    break;
    
    case COMMAND_GET_CURRENT_TIME:
        print_command("Get Current Time Info");
    #if (USES_TIMESTAMP == 1)
    {
        //char buffer[64];
        getCurrentTime();  // Populates the current time field and prints it
        global_send.chunks_outstanding = 0;
        global_send.data_length = sTimeInfoData->dataLength;
        global_send.data = sTimeInfoData->timeInfoBuf;
        global_send.offset = 0;
        global_send.continuous_stage = CONT_NONE;
        global_send.handle = m_met_response_handle.value_handle;
        //memset(buffer, 0, 64);
        //NRF_LOG_INFO("Created Current Time Info %s at time %u", 
        //    (uint32_t)byteToHex(sTimeInfoData->timeInfoBuf, buffer, " ", sTimeInfoData->dataLength), getTicks());
        NRF_LOG_FLUSH();
    }
    #else
        createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
    #endif
    send_flag = true;
    break;
    
    case COMMAND_SET_CURRENT_TIME:
        // Set the current time info time sync to synced by PHG
    #if (USES_TIMESTAMP == 1)
        // The update of the CurrentTimeInfo byte array as well as the date time adjustments was done in the event
        createCpResponse(METCP_COMMAND_DONE, NULL);
    #else
        createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
    #endif
        send_flag = true;
        break;

    case COMMAND_GET_CONFIG_INFO:
        print_command("Get Config Info");
        createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
        send_flag = true;
        break;
    
    case COMMAND_GET_NUMBER_OF_STORED_RECORDS:
        print_command("Get Number of Stored Records");
        #if (USES_STORED_DATA == 1)
            NRF_LOG_INFO("Number of stored records is %u", numberOfStoredMsmtGroups);
            // Respond with command done
            createCpResponse(METCP_COMMAND_DONE, &numberOfStoredMsmtGroups);
        #else
            createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
        #endif
        send_flag = true;
        break;
    
    case COMMAND_GET_ALL_STORED_RECORDS:
        stored_data_done_sent = false;
        print_command("Get All Stored Records");
        #if (USES_STORED_DATA == 1)
            if (numberOfStoredMsmtGroups > 0)
            {
                NRF_LOG_DEBUG("----> Sending stored data element 0");
                global_send.number_of_groups = numberOfStoredMsmtGroups;
                sendStoredMeasurements(0);
            }
        #else
            createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
            send_flag = true;
        #endif
        break;
            
    case COMMAND_GET_STORED_RECORDS_BY_INDEX:
        stored_data_done_sent = false;
        print_command("Get Stored Records by index");
        #if (USES_STORED_DATA == 1)
            if (numberOfStoredMsmtGroups > 0)
            {
                if (index >= numberOfStoredMsmtGroups)
                {
                    NRF_LOG_DEBUG("----> Sending stored data element number\r\n", index);
                    global_send.number_of_groups = 1;
                    sendStoredMeasurements(index);
                }
                else
                {
                    createCpResponse(METCP_COMMAND_ERROR, NULL);
                    send_flag = true;
                }
            }
        #else
            createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
            send_flag = true;
        #endif
        break;
    
    case COMMAND_GET_STORED_RECORDS_BY_TIME:
        stored_data_done_sent = false;
        print_command("Get Stored Records by index");
        createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
        send_flag = true;
        break;

    case COMMAND_DELETE_ALL_STORED_RECORDS:
        print_command("Delete All Stored Records");
        #if (USES_STORED_DATA == 1)
            deleteStoredSpecializationMsmts();  // really don't need this, setting numberOfStoredMsmtGroups to 0 will do it.
            numberOfStoredMsmtGroups = 0;
            // Respond with command done
            createCpResponse(METCP_COMMAND_DONE, NULL);
        #else
            createCpResponse(METCP_COMMAND_UNSUPPORTED, NULL);
        #endif
        send_flag = true;
        break;

    case COMMAND_SEND_LIVE_DATA:
        print_command("Send Live Data");
        send_live_data = true;
        break;

    default:
        print_command("an unsupported");
        // Respond with command done
        createCpResponse(METCP_COMMAND_UNKNOWN, NULL);
        send_flag = true;
    }
}

#if (USES_TIMESTAMP == 1)
    static void handleSetTime(unsigned char *setTime)
    {
        unsigned short i;
        unsigned char oldTime[MET_TIME_LENGTH];
        unsigned char newTime[MET_TIME_LENGTH];
        unsigned long long oldTimeVal = 0ULL;
        unsigned long long newTimeVal = 0ULL;
        long long diff = 0LL;
        long long oldEpoch = epoch;
        unsigned short timeSync;
        char printBuf[64];
        getCurrentTime();
        memcpy(oldTime, &sTimeInfoData->timeInfoBuf[sTimeInfoData->currentTime_index], MET_TIME_LENGTH); // destination, source, length
        memcpy(newTime, setTime, MET_TIME_LENGTH);
        // check that formats are good
        memset(printBuf, 0, 64);
        NRF_LOG_INFO("Set Time received: %s", (uint32_t)byteToHex(newTime, printBuf, " ", MET_TIME_LENGTH));
        NRF_LOG_FLUSH();
        updateCurrentTimeFromSetTime(&sTimeInfoData, newTime);
        NRF_LOG_INFO("New current time: %s", (uint32_t)byteToHex(sTimeInfoData->timeInfoBuf, printBuf, " ", MET_TIME_LENGTH));
        NRF_LOG_FLUSH();
        for (i = 0; i < 6; i++)
        {
             oldTimeVal = (oldTimeVal << 8) + (((unsigned long long)oldTime[5 - i]) & 0xFF);  // epoch is located at start
             newTimeVal = (newTimeVal << 8) + (((unsigned long long)newTime[5 - i]) & 0xFF);
        }
        diff = newTimeVal - oldTimeVal;
        epoch = epoch + diff; // Adjust start epoch by difference too. This allows us to get new time by adding current ticks to new start epoch
                                                             // How to add an unsigned int to a signed int
        NRF_LOG_DEBUG("old time: %llx  new time: %llx diff %llx  old epoch: %llx, new epoch: %llx", oldTimeVal, newTimeVal, diff, oldEpoch, epoch);
        timeSync = (((unsigned short) newTime[MET_TIME_LENGTH - 2]) & 0xFF) + (newTime[MET_TIME_LENGTH -1 ] << 8);
        sMetTime->timeSync = timeSync;      // Update the time sync of our 'base' current time
        // Handles date-time adjustments and updating the time sync in the group data array msmt time stamps
        handleSpecializationsOnSetTime(numberOfStoredMsmtGroups, diff, timeSync);
    }
#endif

static bool handleCommonEvents(ble_evt_t* p_ble_evt)
{
    ret_code_t err_code;
    bool used = false;   // just indicates an event was handled when set to true

    switch (p_ble_evt->header.evt_id)
    {
        /*  From the Dev Zone:
        244 bytes is the maximum amount of attribute data that can be sent in a single over the air packet over BLE. The actual data length in this case
        is 251 bytes, since the ATT protocol adds a 3 byte header and the link layer adds another 4 byte header on top of the attribute data. 
        The reason the NRF_SDH_BLE_GATT_MAX_MTU_SIZE is set to 247 is to make room for the 3 byte ATT header while still allowing you to send the maximum
        244 bytes of attribute data. 

        According to the Bluetooth specification an attribute can not be larger than 512 bytes (not including the 3 byte header), so if you need to
        send more data than this you need to split it into multiple packets yourself. 

        The most efficient way to transmit data over BLE is to split it into 244 byte chunks, since this will utilize the maximum capacity of the
        BLE packets and minimize the overhead. 
        */
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            mtu_size = p_ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;
            NRF_LOG_DEBUG("New MTU size requested %d", mtu_size);
            if (mtu_size > NRF_SDH_BLE_GATT_MAX_MTU_SIZE)   // Set in sdk_config.h
            {
                mtu_size = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;
                NRF_LOG_DEBUG("Requested MTU size exceeds maximum; reset to maximum size of %d", mtu_size);
            }
            err_code = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle, mtu_size);

            if (err_code != NRF_SUCCESS)
            {
                mtu_size = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;
                NRF_LOG_DEBUG("MTU exchange request reply failed. Error code: 0x%02X", err_code);
            }
            global_send.chunk_size = (mtu_size - OPCODE_LENGTH - HANDLE_LENGTH);
            used = true;
            break;

        #if defined (S140) || defined (S113)
            case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
            {
                ble_gap_data_length_limitation_t limitation;
                ble_gap_data_length_params_t server;
                NRF_LOG_DEBUG("Data length update request: max_rx_octets %d, max_rx_time %d, "
                    "max_tx_octets %d, max_tx_time %d",
                    p_ble_evt->evt.gap_evt.params.data_length_update_request.peer_params.max_rx_octets,
                    p_ble_evt->evt.gap_evt.params.data_length_update_request.peer_params.max_rx_time_us,
                    p_ble_evt->evt.gap_evt.params.data_length_update_request.peer_params.max_tx_octets,
                    p_ble_evt->evt.gap_evt.params.data_length_update_request.peer_params.max_tx_time_us);
                unsigned short tx_size = p_ble_evt->evt.gap_evt.params.data_length_update_request.peer_params.max_rx_octets;
                server.max_rx_octets = p_ble_evt->evt.gap_evt.params.data_length_update_request.peer_params.max_tx_octets;
                server.max_rx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
                server.max_tx_octets = (tx_size < NRF_SDH_BLE_GAP_DATA_LENGTH) ? tx_size : NRF_SDH_BLE_GAP_DATA_LENGTH;  // Set in sdk_config.h
                server.max_tx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
                uint32_t result = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &server, &limitation);
                NRF_LOG_DEBUG("sd_ble_gap_data_length_update Result %d", result);
                if (result != NRF_SUCCESS)
                {
                    NRF_LOG_DEBUG("RX limit %d  tx limit %d", limitation.rx_payload_limited_octets, limitation.tx_payload_limited_octets);
                }
                used = true;
            }
            break;

            case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
            {
                NRF_LOG_DEBUG("Data length update: max_rx_octets %d, max_rx_time %d, "
                    "max_tx_octets %d, max_tx_time %d",
                    p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_rx_octets,
                    p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_rx_time_us,
                    p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets,
                    p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_tx_time_us);
                data_length = p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets;
            }
            used = true;
            break;
        #endif
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("Physical update request: rx phys %d, tx phys %d",
                p_ble_evt->evt.gap_evt.params.phy_update_request.peer_preferred_phys.rx_phys,
                p_ble_evt->evt.gap_evt.params.phy_update_request.peer_preferred_phys.tx_phys);
            const ble_gap_phys_t* phys = &p_ble_evt->evt.gap_evt.params.phy_update_request.peer_preferred_phys;
            uint32_t result = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, phys);
            NRF_LOG_DEBUG("Result %d", result);
            used = true;
        }

        case BLE_GAP_EVT_PHY_UPDATE:
        {
            NRF_LOG_DEBUG("Physical update update:rx phys %d, tx phys %d,",
                p_ble_evt->evt.gap_evt.params.phy_update.rx_phy,
                p_ble_evt->evt.gap_evt.params.phy_update.tx_phy);
            used = true;
        }
        break;
    }
    return used;
}

static void handle_response_char_done()
{
    if (global_send.current_command == COMMAND_GET_ALL_STORED_RECORDS ||
        global_send.current_command == COMMAND_SEND_LIVE_DATA)
    {
        if (global_send.continuous_stage == CONT_RECORD_SEND)
        {
            NRF_LOG_DEBUG("----> Send Notification Record done");
            global_send.continuous_stage = CONT_RECORD_DONE;
            createContinuousRecordDone();
        }
        else if (global_send.continuous_stage == CONT_RECORD_DONE)
        {
            NRF_LOG_DEBUG("----> Notification Record done sent");
            global_send.handle = 0;
            return;
        }
        else
        {
            NRF_LOG_DEBUG("----> Send Indication Record done");
            createCpResponse(METCP_COMMAND_RECORD_DONE, NULL);
        }
    }
    else
    {
        createCpResponse(METCP_COMMAND_DONE, NULL);
    }
    send_flag = true;
}

static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    uint32_t              err_code;
    ble_gap_conn_params_t connParams;

    if (p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connection event received at time %u.", getTicks());
            m_connection_handle = p_ble_evt->evt.gap_evt.conn_handle;
            global_send.chunk_size = (mtu_size - OPCODE_LENGTH - HANDLE_LENGTH);
            if (saveDataBuffer != NULL)
            {
                // Calling sd_ble_gatts_sys_attr_set with CCCD info
                err_code = sd_ble_gatts_sys_attr_set(p_ble_evt->evt.gap_evt.conn_handle, saveDataBuffer,
                    saveDataLength, BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_DEBUG("Failed updating persistent sys attr info. Error code: 0x%02X", err_code);
                    free(saveDataBuffer);
                    saveDataBuffer = NULL;
                    saveDataLength = 0;
                }
            }
            #if defined (S140) || defined (S113)
                // Request update of data length
                ble_gap_data_length_limitation_t limitation;
                ble_gap_data_length_params_t server;
                server.max_rx_octets = 27;
                server.max_rx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
                server.max_tx_octets = NRF_SDH_BLE_GAP_DATA_LENGTH; // Set in sdk_config.h
                server.max_tx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
                err_code = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &server, &limitation);
                NRF_LOG_DEBUG("sd_ble_gap_data_length_update attempt Result %d", err_code);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_DEBUG("RX limit %d  tx limit %d", limitation.rx_payload_limited_octets, limitation.tx_payload_limited_octets);
                }
            #endif
            bsp_board_led_off(ADVERTISING_LED);
            bsp_board_led_on(CONNECTED_LED);

            break;

        // Service change indication acknolwedged
        case BLE_GATTS_EVT_SC_CONFIRM:
            NRF_LOG_DEBUG("Service change acknowledged at time %u", getTicks());
            break;

        // Added just for info
        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
            connParams = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params;
            NRF_LOG_DEBUG("Connection parameters update at time %u: min: %d max: %d supervision timeout: %d slave latency: %d ",
                getTicks(),
                connParams.min_conn_interval, connParams.max_conn_interval, 
                connParams.conn_sup_timeout, connParams.slave_latency);
            break;
                
            case BLE_GATTC_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTC_EVT_TIMEOUT

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_TIMEOUT

        case BLE_GAP_EVT_DISCONNECTED:
        {
            NRF_LOG_INFO("Disconnected at time %u", getTicks());
            app_timer_stop(m_met_live_data_timer_id);
            uint16_t currentSysDataLength;
            unsigned char *currentSysDataBuffer;
            bsp_board_led_off(ADVERTISING_LED);
            bsp_board_led_off(CONNECTED_LED);
            bsp_board_led_off(MSMT_DATA_LED);
            bsp_board_led_on(DISCONNECTED_LED);
            met_abort = true;

            // call twice; once to get the size of the data
            // create the buffer,
            // call a second time to load the data into the buffer
            sd_ble_gatts_sys_attr_get(p_ble_evt->evt.gap_evt.conn_handle, NULL, &currentSysDataLength,
                BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
            // Do we need to upDate flash?
            if (currentSysDataLength > 0) // This should always have something!!
            {
                // Lets get the current system info
                currentSysDataBuffer = calloc(1, currentSysDataLength * sizeof(uint8_t));
                err_code = sd_ble_gatts_sys_attr_get(p_ble_evt->evt.gap_evt.conn_handle, currentSysDataBuffer, &currentSysDataLength,
                    BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_DEBUG("Failed getting persistent sys attr info. Error code: 0x%02X", err_code);
                    // TODO  do something here
                }
                bool isEqual = ((currentSysDataLength == saveDataLength)
                                && (saveDataLength > 0 && saveDataBuffer != NULL)
                                && (memcmp(saveDataBuffer, currentSysDataBuffer, saveDataLength) == 0)
                                && stored_data_same);
                if (isEqual)
                {
                    NRF_LOG_INFO("Flash write not needed; data is equal to what is already in flash\n\n\n\n");
                    
                }
                else
                {
                    if (saveDataBuffer != NULL)
                    {
                        free (saveDataBuffer);
                    }
                    saveDataBuffer = calloc (1, currentSysDataLength *sizeof(uint8_t));
                    saveDataLength = currentSysDataLength;
                    memcpy(saveDataBuffer, currentSysDataBuffer, saveDataLength);
                }
                free(currentSysDataBuffer);
                flash_write_needed = !isEqual;
            }
            m_connection_handle = BLE_CONN_HANDLE_INVALID;
            if (flash_write_needed)
            {
                // this will disable soft device causing app to reset after flash is written
                latestTimeStamp = getRtcTicks();
                saveKeysToFlash(&keys, &saveDataBuffer, &saveDataLength, cccdSet, &noOfCccds);
                stored_data_same = true;
                // Now LEDs wont work - everything is dead due to the sd_softdevice_disable.
                break;
            }
            reset_specializations();
            #if (USE_DK == 0)
                restartAdv = true;
            #endif
        }
        break;

        case BLE_GAP_EVT_TIMEOUT:
            NRF_LOG_DEBUG("Advertisement timed out at time %u", getTicks());
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            NRF_LOG_DEBUG("Saved info being requested at time %u", getTicks());
            err_code = sd_ble_gatts_sys_attr_set(p_ble_evt->evt.gap_evt.conn_handle, saveDataBuffer,
                saveDataLength, BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
            if (err_code != NRF_SUCCESS)
            {
                if (saveDataBuffer != NULL)
                {
                    free (saveDataBuffer);
                    saveDataBuffer = NULL;
                    saveDataLength = 0;
                }
                NRF_LOG_DEBUG("Failed updating persistent sys attr info. Error code: 0x%02X", err_code);
            }
            break;

        /*  Pairing - encryption sequence:
         *  If not paired:
         *      BLE_GAP_EVT_SEC_PARAMS_REQUEST
         *      BLE_GAP_EVT_CONN_SEC_UPDATE
         *      BLE_GAP_EVT_AUTH_STATUS
         *  If paired:
         *      BLE_GAP_EVT_SEC_INFO_REQUEST
         *      BLE_GAP_EVT_CONN_SEC_UPDATE
         */

        // Pairing request from the PHG
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            #if (SUPPORT_PAIRING > 0)
                bsp_board_led_on(BSP_BOARD_BUTTON_3);
                NRF_LOG_INFO("Pairing requested");
                if (keys.keys_own.p_enc_key->enc_info.ltk_len != 0)     // Pairing request from a different PHG or PHG lost its bonding info.
                {
                    NRF_LOG_DEBUG("MET is paired with some device but not the one that has connected!");
                    if (saveDataBuffer != NULL)
                    {
                        free (saveDataBuffer);
                        saveDataLength = 0;
                        saveDataBuffer = NULL;
                    }
                }
                memcpy(&peer_sec_params, &p_ble_evt->evt.gap_evt.params.sec_params_request.peer_params, sizeof(ble_gap_sec_params_t));  // destination, source, length
                err_code = sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_SUCCESS, &own_sec_params, &keys);
                APP_ERROR_CHECK(err_code);

            #else
                NRF_LOG_DEBUG("Sending pairing not supported!");
                err_code = sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
                APP_ERROR_CHECK(err_code);
            #endif
            break;

        #if (SUPPORT_PAIRING > 0)
            // Encryption has been established
            case BLE_GAP_EVT_CONN_SEC_UPDATE:
            {
                NRF_LOG_INFO("Encryption established");
                hasEncrypted = true;
            }
            break;

            /* This event is signaled when pairing has completed. All the key information is
             * placed in the 'keys' struct passed into the sd_ble_gap_sec_params_reply() method.
             * The next event to be signaled will be encryption.
             * If already paired, this event will not be signaled
             */
            case BLE_GAP_EVT_AUTH_STATUS:
            {
                bsp_board_led_off(BSP_BOARD_BUTTON_3);
                NRF_LOG_INFO("Pairing completed");
            }
            break;
        
            // For requesting pass keys - should not happen here.
            case BLE_GAP_EVT_AUTH_KEY_REQUEST:

                err_code = sd_ble_gap_auth_key_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_AUTH_KEY_TYPE_NONE, NULL);
                break;

            /* This event is signaled when encryption is being requested. Should only happen on a bonded reconnect. 
             * Here the keys that are needed are those of the peripheral. The keys of the slave device are used */
            case BLE_GAP_EVT_SEC_INFO_REQUEST:
            {
                NRF_LOG_INFO("Encryption request received");
                flash_write_needed = false; // We already have our data.
                ble_gap_evt_sec_info_request_t sec_info = p_ble_evt->evt.gap_evt.params.sec_info_request;
                ble_gap_enc_info_t* enc_info = NULL;
                ble_gap_irk_t* irk_info = NULL;

                if (sec_info.enc_info) // Set if peer is asked for the encryption info
                {
                    enc_info = &keys.keys_own.p_enc_key->enc_info; // Keys created by the peripheral are used
                }
                if (sec_info.id_info) // Set if peer is asking for the IRK
                {
                    irk_info = &keys.keys_own.p_id_key->id_info;
                }
                err_code = sd_ble_gap_sec_info_reply(p_ble_evt->evt.gap_evt.conn_handle, enc_info, irk_info, NULL);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("PAIRING INCONSISTENCY!! Client thinks it is paired with us but we have lost that pairing data. Error code: 0x%02X", err_code);
                }
            }
            break;
       #endif


        case BLE_GATTS_EVT_HVC:
            global_send.chunks_outstanding--;
            if (global_send.offset >= global_send.data_length && global_send.offset > 0)  // Have all segments been indicated?
            {
                NRF_LOG_INFO("----> Indications for command %u complete at time %u, connection handle 0x%04X", global_send.current_command, getTicks(), m_connection_handle);
                global_send.data_length = 0;
                global_send.offset = 0;
                // If the response characteristic, the task is done but has not been confirmed.
                if (global_send.handle == m_met_response_handle.value_handle)
                {
                    NRF_LOG_INFO("----> Indication was for the response characteristic, not the control point");
                    // This will set up the confirmation
                    handle_response_char_done();
                    break;
                }
                // When on control point, the confirmation is done. Clear the handle and prepare for the next action
                global_send.handle = 0;
                if (global_send.current_command == COMMAND_SEND_LIVE_DATA) // If this is a record done confirmation for live data
                {
                    global_send.data_length = 0;
                    #if (SEND_OPTIMIZED == 1)        // When we send optimized, the first send is the full package. After that it is optimized.
                        first_cont_sent = true;
                    #endif
                }
                #if (USES_STORED_DATA == 1)
                    else if (global_send.current_command == COMMAND_GET_ALL_STORED_RECORDS)
                    {
                        global_send.number_of_groups--;
                        if (global_send.number_of_groups > 0 && global_send.number_of_groups <= NUMBER_OF_STORED_MSMTS)
                        {
                            NRF_LOG_DEBUG("----> Sending stored data element %u", (numberOfStoredMsmtGroups - global_send.number_of_groups));
                            sendStoredMeasurements(numberOfStoredMsmtGroups - global_send.number_of_groups);
                        }
                        else if (!stored_data_done_sent)
                        {
                            NRF_LOG_INFO("----> All stored data sent");
                            createCpResponse(METCP_COMMAND_DONE, NULL);
                            stored_data_done_sent = true;
                            send_flag = true;
                        }
                    }
                #endif
                break;
            }
            else
            {
                NRF_LOG_DEBUG("----> Indication of hunk complete at time %u, connection handle 0x%04X", getTicks(), m_connection_handle);
                send_flag = true;
            }
            break;

        // By the time this is signaled, the write response has already been sent - so one is free to do whatever.
        case BLE_GATTS_EVT_WRITE:
            // Enable IND on CP
            if (p_ble_evt->evt.gatts_evt.params.write.handle == m_met_cp_handle.cccd_handle)
            {
                uint8_t write_data = p_ble_evt->evt.gatts_evt.params.write.data[0];
                cccdSet[COMMAND_CHAR] = (write_data == BLE_GATT_HVX_INDICATION);
                NRF_LOG_INFO("Enabling MET CP CCCD with %d at time %u", write_data, getTicks());
            }
            // Enable IND or NOT Response
            else if (p_ble_evt->evt.gatts_evt.params.write.handle == m_met_response_handle.cccd_handle)
            {
                uint8_t write_data = p_ble_evt->evt.gatts_evt.params.write.data[0];
                cccdSet[RESPONSE_CHAR] = (write_data == 3) ? BLE_GATT_HVX_INDICATION : (write_data & 3);
                NRF_LOG_INFO("Enabling MET Response CCCD with %d at time %u", write_data, getTicks());
            }
            break;

        // Turns out for notifications one sends in a loop and you may get one event per N packets. Could be one event per notification
        // or one event per N notifications. So I need to keep track of outstanding events. global_send.chunks_outstanding is incremented
        // every notification, and decremented by p_ble_evt->evt.common_evt.params.tx_complete.count every event. When 0, the notification
        // sequence is done.
        case BLE_GATTS_EVT_HVN_TX_COMPLETE:  // This is the best we get for notifications
            global_send.chunks_outstanding = global_send.chunks_outstanding - p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count;
            NRF_LOG_DEBUG("----> Notification TX done event received. Packets sent and not evented %u", global_send.chunks_outstanding);
            if (global_send.chunks_outstanding > 0) // Have not received all events from notifications
            {
                if (global_send.offset < global_send.data_length)
                {
                    send_flag = true;
                }
                break;
            }
            if (global_send.offset >= global_send.data_length && global_send.offset > 0)  // Have all segments been notified?
            {
                NRF_LOG_INFO("----> Notification(s) complete at time %u, connection handle 0x%04X", getTicks(), m_connection_handle);
                global_send.data_length = 0;
                global_send.offset = 0;
                if (global_send.handle == m_met_response_handle.value_handle)      // Notification was for response char
                {
                    handle_response_char_done();
                }
                break;
            }
            else
            {
                NRF_LOG_DEBUG("----> Notification of hunk complete at time %u, connection handle 0x%04X", getTicks(), m_connection_handle);
                send_flag = true;
            }
            break;


        default:
            if (on_generic_ble_evt(p_ble_evt))
            {
                break;
            }
            if (handleCommonEvents(p_ble_evt)) break;
            NRF_LOG_DEBUG("Unhandled Event %d at time %u", p_ble_evt->header.evt_id, getTicks());
            break;
        }
}

static void write_flash(void * p_context)
{
    saveKeysToFlash(&keys, &saveDataBuffer, &saveDataLength, cccdSet, &noOfCccds);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
/*
static void sys_evt_dispatch(uint32_t sys_evt)
{
    // Dispatch the system event to the fstorage module, where it will be
    // dispatched to the Flash Data Storage (FDS) module.
    // fs_sys_event_handler(sys_evt);

    // Dispatch to the Advertising module last, since it will check if there are any
    // pending flash operations in fstorage. Let fstorage process system events first,
    // so that it can report correctly to the Advertising module.
    // ble_advertising_on_sys_evt(sys_evt);
}
*/

#define RAM_START       0x20000000
static uint32_t ram_end_address_get(void)
{
    uint32_t ram_total_size;

#ifdef NRF51
    uint32_t block_size = NRF_FICR->SIZERAMBLOCKS;
    ram_total_size      = block_size * NRF_FICR->NUMRAMBLOCK;
#else
    ram_total_size      = NRF_FICR->INFO.RAM * 1024;
#endif

    return RAM_START + ram_total_size;
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */

static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    uint32_t const app_ram_start_link = ram_start;
    err_code = sd_ble_enable(&ram_start); // This can change the ram start I guess.
    if (ram_start > app_ram_start_link)
    {
        NRF_LOG_WARNING("Insufficient RAM allocated for the SoftDevice.");

        NRF_LOG_WARNING("Change the RAM start location from 0x%x to 0x%x.",
                        app_ram_start_link, ram_start);
        NRF_LOG_WARNING("Maximum RAM size for application is 0x%x.",
                        ram_end_address_get() - (ram_start));
    }
    else
    {
        NRF_LOG_DEBUG("RAM starts at 0x%x", app_ram_start_link);
        if (ram_start != app_ram_start_link)
        {
            NRF_LOG_DEBUG("RAM start location can be adjusted to 0x%x.", ram_start);

            NRF_LOG_DEBUG("RAM size for application can be adjusted to 0x%x.",
                          ram_end_address_get() - (ram_start));
        }
    }

    // Enable BLE stack.
//     err_code = nrf_sdh_ble_enable(&ram_start);
//     APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
//     NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_dispatch , NULL);
}

// Not sure what this used for
static void gpiote_init_new(void)
{
   // APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}

// This method is called only once after the final measurement is received from the
// sensor module. At this point we start advertising, connect and send the obtained 
// final measurements
static void bring_up_adver(void)
{
    if (m_connection_handle  == BLE_CONN_HANDLE_INVALID)  // Should always be invalid
    {
        emptyQueue(queue);
        // We auto-add a stored msmt when there is no button push
        #if (USE_DK == 0)
            numberOfStoredMsmtGroups = 0;
            if (generateAndAddStoredMsmt((unsigned long long)getRtcTicks(), getTicks(), numberOfStoredMsmtGroups))
            {
                numberOfStoredMsmtGroups++;
            }
            NRF_LOG_DEBUG("Waiting 4 seconds before restarting advertisments");
            nrf_delay_ms(4000); // Give us a chance to exit PHG
        #endif
        #if (SPIROMETER == 1)
            spiro_sequence = 0;
        #endif
        #if (SCALE == 1)
            scale_sequence = 0;
        #endif
        msmt_id = 1;
        send_live_data = false;
        live_data_count = 0;
        first_cont_sent = false;
        global_send.continuous_stage = CONT_NONE;
        hasEncrypted = (pairing > 0) ? false : true;    // Set to true if pairing is not supported
        met_abort = false;
        flash_write_needed = true;
        bsp_board_led_on(ADVERTISING_LED);
        app_timer_start(m_met_live_data_timer_id, MET_LIVE_DELAY, NULL); // If already running this call does nothing
        advertising_start();
    }
}

/*
static void receive_new_measurement(uint8_t * p_data)
{
    uint8_t     pulse;
    uint8_t     spo;
    unsigned long dev_status;
}
*/

#if (USE_DK == 1)
    /*  We only use on the DK in place of no real sensor. To use this code we have to
        bring back the Board Support files into the project. */
    static void bsp_event_handler(bsp_event_t event)
    {
        switch (event)
        {
            case BSP_EVENT_SLEEP:
                break;

            case BSP_EVENT_KEY_3:
            {
                #if (USES_STORED_DATA == 1 && USES_TIMESTAMP == 1)
                    if ( numberOfStoredMsmtGroups >= NUMBER_OF_STORED_MSMTS)
                    {
                        NRF_LOG_DEBUG("Stored data buffer full, skipping");
                        return;
                    }
                    NRF_LOG_DEBUG("Stored data generator called at timestamp32 %lu", getTicks());
                    if (generateAndAddStoredMsmt(getRtcTicks(), getTicks(), numberOfStoredMsmtGroups))
                    {
                        numberOfStoredMsmtGroups++;
                        stored_data_same = false;

                    }
                #endif
            }
            break;

            case BSP_EVENT_KEY_1:
                if (m_connection_handle != BLE_CONN_HANDLE_INVALID)
                {
                    NRF_LOG_INFO("Sending disconnect");
                    met_abort = false;
                    app_timer_start(m_met_disconnect_timer_id, MET_COMMAND_DELAY, NULL);
                }
                else
                {
                    NRF_LOG_INFO("Clearing pairing and stored data");
                    numberOfStoredMsmtGroups = 0;
                    #if(USES_STORED_DATA == 1)
                        deleteStoredSpecializationMsmts();
                    #endif
                    clearSecurityKeys(&keys);
                }
                break;

            case BSP_EVENT_KEY_2:
            {
                NRF_LOG_INFO("Button press done to start advertising");
                bring_up_adver();
            }
            break;

            default:
                break;
        }
    }

    static void buttons_leds_init(void)
    {
        bsp_event_t startup_event;

        ret_code_t err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);

        APP_ERROR_CHECK(err_code);

        err_code = bsp_btn_ble_init(NULL, &startup_event);
        APP_ERROR_CHECK(err_code);
    }
#else
    /**@brief Function for the LEDs initialization.
     *
     * @details Initializes all LEDs used by the application.
     */
    static void buttons_leds_init(void)
    {
        bsp_board_init(BSP_INIT_LEDS);
    }
#endif

/*
void UART0_IRQHandler_New(app_uart_evt_t * p_app_uart_event)
{

    static uint8_t new_data = p_app_uart_event->data.value;

    // @snippet [Handling the data received over UART]
    // Map it to a user-defined structure that will them be used to populate the
    // update methods.

}
*/


/*
static void uart_init(void)
{
    ret_code_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,        // Use parity
          UART_BAUDRATE_BAUDRATE_Baud115200
      };
    err_code = app_uart_init(&comm_params,
                       NULL,
                       UART0_IRQHandler_New,
                       APP_IRQ_PRIORITY_LOW);

    APP_ERROR_CHECK(err_code);
}
*/

static void initializeBluetooth()
{
    ret_code_t err_code;

    unsigned char cccds[2];

    memset(cccds, 0, noOfCccds);
    loadKeysFromFlash(&keys, &saveDataBuffer, &saveDataLength, cccds, &noOfCccds);
    stored_data_same = true;
    NRF_LOG_DEBUG("Number of saved stored measurements in flash %u", numberOfStoredMsmtGroups);
    memcpy(cccdSet, cccds, noOfCccds);  // destination, source, length

    #if (USES_STORED_DATA == 1 && USES_TIMESTAMP == 1)
    if (numberOfStoredMsmtGroups > 0)
    {
        NRF_LOG_DEBUG("Factory timer reset: Current tick %u  Saved time of flash write %u", getRtcTicks(), latestTimeStamp);
        if (getRtcTicks() < latestTimeStamp)  // Time fault - inspect stored data and set flag on different time line
        {
            setNotOnCurrentTimeline();
        }
    }
    #endif


        // This also provides the callback method to receive events.
    ble_stack_init();
    sd_mutex_new(&p_mutex);
    sd_mutex_new(&q_mutex);

    // Set device address
    ble_gap_addr_t addrStruct;
    addrStruct.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
    memcpy(addrStruct.addr, getBtAddress(), 6);  // destination, source, length
    err_code = sd_ble_gap_addr_set(&addrStruct);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Could not set the Bluetooth Address. Error code %d", err_code);
        APP_ERROR_CHECK(err_code);
    }
    // Sets max and min connection intervals, slave latency, and the GAP characteristic entries
    // for the friendly name and appearance.
    gap_params_init();
    // Creates the advertisement and scan response data arrays
    err_code = advertisement_met_set(&m_adv_handle);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Could not configure the advertisements");
        APP_ERROR_CHECK(err_code);
    }
    // ===================================== Create the MET service
    err_code = createPrimaryService(&m_met_service_handle, BTLE_MET_SERVICE);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Could not create MET service");
        APP_ERROR_CHECK(err_code);
    }
    // Create the MET control point characteristic
    err_code = createStandardCharacteristic(m_met_service_handle,
        &m_met_cp_handle,
        BTLE_MET_CP_CHAR,
        true,   // Has CCCD
        BLE_GATT_HVX_INDICATION,      // This will cause indicate when CCCD is set to true
        18,
        cp_write,
        TRAP_WRITE,
        (ble_gap_conn_sec_mode_t) {1, (SUPPORT_PAIRING + 1) },   // [CCCD write is secured]
        (ble_gap_conn_sec_mode_t) {0, 0},   // Reading characteristic value is forbidden
        (ble_gap_conn_sec_mode_t) {1, (SUPPORT_PAIRING + 1)},  // [Writing characteristic is secured]
        false); // Not static
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Could not create MET control point characteristic");
        APP_ERROR_CHECK(err_code);
    }
    // ===================================== Create the MET response characteristic
    err_code = createStandardCharacteristic(m_met_service_handle,
        &m_met_response_handle,
        BTLE_MET_RESPONSE_CHAR,
        true,   // Has CCCD
        (BLE_GATT_HVX_NOTIFICATION | BLE_GATT_HVX_INDICATION),  // This will cause indicate and notify if CCCD is set to true
        TRAP_NONE,
        NULL,
        false,
        (ble_gap_conn_sec_mode_t)
        {
            1, (SUPPORT_PAIRING + 1)     // CCCD write is secured
        },
        
        (ble_gap_conn_sec_mode_t)
        {
            0, 0    // Reading characteristic value is forbidden
        },
        
        (ble_gap_conn_sec_mode_t)
        {
            0, 0    // Writing characteristic is forbidden
        },
        false); // Not static
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Could not create MET response characteristic");
        APP_ERROR_CHECK(err_code);
    }
    

    // Not sure about this yet. I think it allows one to put up a fight to argue 
    // over connection parameters and sets up responses to connection parameter
    // change requests.
    //conn_params_init();

    // Start execution.
    NRF_LOG_INFO("MET Start at time %u!\r", getTicks());
    #if (USE_DK == 0)
        bring_up_adver();
    #endif
}

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    // I do not know where this is established. I don't know what
    // the 'app' event is that triggers this or where it setup.
    ret_code_t err_code = sd_app_evt_wait();
    
    if (err_code == NRF_SUCCESS)
    {
        //uint32_t evt_id;
        // Pull event from SOC.
        //err_code = sd_evt_get(&evt_id);
        //NRF_LOG_DEBUG("Soc event # %u", evt_id);
        return;
    }
    NRF_LOG_DEBUG("sd app evt wait error %u", err_code);
    APP_ERROR_CHECK(err_code);
}

void main_wait(void)
{
    if (NRF_LOG_PROCESS() == true)
    {
        return;
    }
    power_manage();
}

/**@brief Function for the main 'embedded' loop. The Nordic SoftDevice is event driven.
* Any method prefixed by sd_* is a SoftDevice method. SoftDevice methods consist of just
* your basic GATT and GAP level calls. The Nordic SDK provides methods to simplify certain
* processes but they all call these sd_* methods. Here we do not use the SDK but only sd_*.

* The key method in the wait loop is sd_app_evt_wait(). This method waits until a Bluetooth
* or system event is triggered. When that happens, the function returns. The application must
* then use the method sd_ble_evt_get() to get the Bluetooth Event that just occurred. It could
* be a read, write, connection, disconnection, etc. The application then must either handle
* the event or ignore it. In this application the event handler is ble_evt_dispatch(). Note
* that after the event is handled one has to call sd_ble_evt_get() again until the method
* returns that there are no more events. One then calls sd_app_evt_wait() and the process 
* repeats. But we extend that functionality.

* Note that the loop also contains a call to the application method send_data(). It is poorly
* named as it does both indications and notifications. What this method does is check if a flag
* has been set that data needs to be indicated/notified. If it has, the flag will be cleared and
* a hunk of data will be indicated/notified. If it is an indication, the app must wait until
* the indication is confirmed so the data is sent and the position in the buffer is updated by
* the amount of data sent. Then the method returns, and we wait for the confirmation event
* BLE_GATTS_EVT_HVC to occur. If all the segments have been sent, the next procedure occurs. If
* not, the indicate flag is set to true and the next segment is sent.

* If the data is notified, hunks can be sent one after the other without waiting for an event
* until the notification buffers are depleted. When that happens, the application must wait for
* the BLE_EVT_TX_COMPLETE. At that point we can continue. This loopy way of handling sending
* data segments would be equivalent to waiting on a semaphore in a real time OS.

* Recall that the MET works as follows:
*   1. receive a command on the Control Point
*   2. if the command is to send data (current time, system info, config info, get stored records,
*      send live data) the data is notified on the response characteristic until all segments are
*      sent. 
*      Now if the data being sent is measurements, there may be several records, especially when
*      transferring stored data. A record consists of all the measurements with a common time
*      stamp. When a record is completed, the PHD server sends an indication on the CP saying a record
*      has completed. The PHG client shall not send another command yet. The PHD must send a second
*      indication on the CP sayong that all records have been sent. Then the PHG can send another
*      command.
*      The system info, current time info, and config info contain only one long entry. There is
*      no record in that case.
*   3. if the PHD does not support the command or the command is to get the number of stored
*      records or delete stored data, there is only an indication on the CP with the result or DONE.

* Pairing/bonding. This experimental protoype requires that the server put in the advertisment whether or not is
* requires pairing. It also requires that if the server requires pairing that is secures writing to
* the control point and response descriptors. The server shall not send a security request. The client
* checks the advertisement and if pairing is required, on platforms that allow the application to initiate
* bonding (like ANdroid), the application can start the bonding. On those that don't (like iOS) the client
* knows pairing will happen when it tries to enable one of the descriptors.

* We do this to keep the agony of pairing as user-friendly as possible as well as synchronous.
* The client will never have to worry about getting a security request or insufficient autherntication
* error at an unknown time. This GREATLY simplifies client code by not having to check for this crap.
* In addition, the user does not have to know if the PHD is pairable or unpairable; the user only has
* to discover and connect. Pairing has proved to be a user nightmare in the field.

* Clearly the command values and response codes are all up in the air. To just make this work
* a set of minimal values were chosen.
*/

static void main_loop(void)
{
    uint8_t enabled;
    uint16_t len;
    ret_code_t result;
    for (;;)
    {
        send_data();        // when flag is set, a set of data is indicated or notified depending upon setup. 
                                // Flag is reset in method
        main_wait();            // Contains the sd_app_evt_wait()
        if (!isEmpty(queue))    // Anything to send?
        {
            void *data = front(queue);
            if (encodeMsmtData(data))
            {
                NRF_LOG_DEBUG("Measurement taken from queue");
                send_flag = true;
                if(sd_mutex_acquire(&q_mutex) != NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN)
                {
                    dequeue(queue);
                    sd_mutex_release(&q_mutex);
                }
            }
        }
        if (restartAdv)     // Only called when USE_DK = 0
        {
            restartAdv = false;
            bsp_board_led_off(DISCONNECTED_LED);
            bsp_board_led_on(MSMT_DATA_LED);
            bring_up_adver();
        }
        while(true)
        {
            // uint32_t evt_id;

            // Don't do this if disabled, for example when writing flash at the end
            sd_softdevice_is_enabled(&enabled); // Don't do this if disabled, for example when writing flash at the end
            if (enabled != 1)
            {
                NRF_LOG_DEBUG("Restarting application at time %lu", getTicks());
                initializeBluetooth();
                NRF_LOG_DEBUG("Current Epoch time %llu  Current ticks %lu", (getRtcTicks() + epoch), getTicks());
               // while(NRF_LOG_PROCESS());
               // NRF_POWER->RESETREAS = 0;
               // NVIC_SystemReset();
               // return;
            }

            result = sd_ble_evt_get(NULL, &len);    // Get size of event
            if (result == NRF_ERROR_NOT_FOUND)      // If there aren't any, go back to wait
            {
                break;
            }
            evt_buf = (uint8_t *)calloc(1, len);                // Make space for event. evt_buf is 4-byte aligned
            result = sd_ble_evt_get((uint8_t *)evt_buf, &len);  // get the event
            if (result == NRF_SUCCESS)
            {
                ble_evt_t *evt = (ble_evt_t *)evt_buf;
                ble_evt_dispatch(evt);                          // dispatch event to handler. This handles only BTLE related events
            }
            else                                                // Hopefully no error but just in case log it.
            {                                                   // Should I do an NVIC_SystemReset() here?
                NRF_LOG_DEBUG("PENDING BLE Event return error: %u", result);
                free(evt_buf);  // Clean up
                break;          // back to wait
            }
            free(evt_buf);      // clean up and get the next event

        }
    }
}

/**@brief Function for application main entry.
 */
int main(void)
{
    ret_code_t err_code;
    NRF_LOG_DEBUG("Power on");
   // unsigned char cccds[2];
    m_adv_handle = 0;
    queue = initializeQueue(10);
    if (queue == NULL)
    {
        NRF_LOG_DEBUG("Could not allocate memory for the message and indication queue. Quitting");
        return 0;
    }
    numberOfStoredMsmtGroups = 0;

    // Initialize.
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_DEBUG("Main start MET");

    // Allocate memory for the security keys
    allocateMemoryForSecurityKeys(&keys);
    sec_params_init();
    timers_init();

    memset(&m_met_cp_handle, 0, sizeof(m_met_cp_handle));
    memset(&m_met_response_handle, 0, sizeof(m_met_response_handle));

    buttons_leds_init();

    gpiote_init_new();
    configureSpecializations();
    err_code = app_timer_start(m_app_dummy_timer_id, CMD_SENSOR_TIME, NULL);
    APP_ERROR_CHECK(err_code);
    elapsedTimeStart = app_timer_cnt_get();
    #if (USES_TIMESTAMP == 1)
        if (sMetTime->clockType == MET_TIME_FLAGS_RELATIVE_TIME)
        {
           epoch = 0;
        }
        else
        {
            epoch = MET_TIME_EPOCH_DEFAULT;
            epoch = epoch * factor;
        }
        sMetTime->epoch = getRtcTicks() + epoch;
        NRF_LOG_DEBUG("Initial Epoch time %llu", sMetTime->epoch);
    #endif
    initializeBluetooth();
    //bring_up_adver();

    //=============================================================================== Start MET
    // Enter main loop.
    main_loop();
    if (saveDataBuffer != NULL)
    {
        free(saveDataBuffer);
    }
    cleanUpQueue(queue);
    cleanUpSpecializations();
}


/**
 * @}
 */
