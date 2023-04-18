/*
Copyright (c) 2020 - 2024, Brian Reinhold

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef GHS_BTLE_UTILS_H__
#define GHS_BTLE_UTILS_H__

#ifndef _WIN32
#include "nrf_log.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "ble.h"
#include "ble_srv_common.h"
#endif

#include "GhsControlStructs.h"

#if (HAS_ABS_TIMESTAMP == 1)
    /**
      Helper method to convert an absolute time to an 'epoch' where 0 is 01 01 2000.
      This method is only valid between the year 2000 to 2100.
     */
    unsigned long long date_time_to_epoch(s_Abs* date_time);

    /**
      Helper method to convert an 'epoch' where 0 is 01 01 2000 to an absolute time.
      This method is only valid between the year 2000 to 2100.
     */
    void epoch_to_date_time(s_Abs* date_time, unsigned long long epoch);

    /**
     * helper to convert an array of abs time bytes to an s_Abs struct
     */
    bool absBytesToAbsStruct(s_Abs **absPtr, unsigned char *timeBytesIn);

    /**
     * helper to convert an abs time struct to an abs time byte array
     */
    bool absStructToAbsBytes(unsigned char **timeBytesPtr, s_Abs *absIn);

    /*
      This method handles 'date time adjustments' so the PHG never has to. It is a lot easier
      for the PHD to do this task as it has all the information needed, and it does not have to
      pass all the information to the PHG so it can do it.

      This method is called after the PHG sets the time. The current time line of the PHD is
      passed in oldTimeIn and the new one written by the PHG is newTimeIn. This method takes
      the difference between the two, and adds that difference to the time stamps of all stored
      data time stamps (stored data must have time stamps). Setting the time is only an option
      for absolute and base offset times.
    */
    void adjustTimeStamps(unsigned char* oldTimeIn, unsigned char* newTimeIn, s_Abs **absTimePtr);
#endif

#ifndef _WIN32
/**@brief Function for initializing services that will be used by the application.
 *
 * Create the specified primary standard BTLE service and add it to GATT
 *
 * @param [IN] serviceHandle pointer to a variable to take the created service's handle
 * @param the 16-bit service value, so it needs to be a BTLE standardized service
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t createPrimaryService(unsigned short* serviceHandle, unsigned short serviceUuid);

/**@brief Function for adding the a standard BTLE characteristic [one identified by a standard 16-bit UUID value defined by BT SIG).
 *
 * @param serviceHandle: the handle of the service to which the characteristic belongs
 * @param charHandles: a pointer to a struct that takes the handles of the created characteristic (declaration, value, and descriptor - if present)
 * @param uuid: the 16-bit UUID of the characteristic
 * @param hasCCCD: true if the characteristic is to include a CCCD
 * @param indicate: set to indicate, notify or both. Ignored if hasCCCD is false
 * @param valueLength: length of the initial characteristic value. Ignored if 0 or value is NULL
 * @param value: initial value. Ignored if NULL or valueLength is 0
 * @param trapReadWrite TRAP_READ or TRAP_WRITE or BOTH, a read andor write of the attribute will signal the BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST event
                        if NEITHER it will be a normal read or write operation
 * @param cccdWritePermission: the security level of the CCCD write operation (ignored if no CCCD)
 * @param attrReadPermission: the security level of the characteristic value read operation. If security mode is 0 no read at all.
 * @param attrWritePermission: the security level of the characteristic value write operation. If security mode is 0 no write at all.
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t createStandardCharacteristic(
    uint16_t serviceHandle,
    ble_gatts_char_handles_t* charHandles,
    uint16_t uuid,
    bool hasCCCD,
    unsigned char indicate,
    uint16_t valueLength,
    uint8_t *value,
    uint8_t trapRead,
    ble_gap_conn_sec_mode_t cccdWritePermission,
    ble_gap_conn_sec_mode_t attrReadPermission,
    ble_gap_conn_sec_mode_t attrWritePermission,
    bool isStatic);
    
ret_code_t createDeviceInformationService(unsigned short int *serviceHandle, s_SystemInfo *systemInfo);
ret_code_t createBatteryService(unsigned short int* serviceHandle, unsigned char* batteryCharValue);

/**
 * Method saves bonding data to a file
 * @param keys of the peripheral
 * @param saveDataBuffer a buffer to hold the data obtained from the sd_ble_gatts_sys_attr_get() method
 * @param saveDataLength the length of the data obtained from the sd_ble_gatts_sys_attr_get()
 * @param cccdSet a pointer to the list of enabled states of the characteristics
 * @param noOfCccds a pointer to the number of Cccds that can be enabled in the specialization
 */
void saveKeysToFlash(ble_gap_sec_keyset_t* keys,
    unsigned char **saveDataBuffer, unsigned short int *saveDataLength,
    unsigned char* cccdSet, unsigned short* noOfCccds);

/**
 * Method loads saved bonding data from a file
 * @param keys the peripheral
 * @param saveDataBuffer a buffer to hold the data obtained from the sd_ble_gatts_sys_attr_get() method
 * @param saveDataLength the length of the data obtained from the sd_ble_gatts_sys_attr_get()
 * @param cccdState a pointer to the list of enabled state of the characteristics
 * @param noOfCccds a pointer to the number of Cccds that can be enabled in the specialization
 */
void loadKeysFromFlash(ble_gap_sec_keyset_t* keys, 
    unsigned char** saveDataBuffer, unsigned short int* saveDataLength, unsigned char* cccdSet,
    unsigned short *noOfCccds);

void allocateMemoryForSecurityKeys(ble_gap_sec_keyset_t* keys);
void clearSecurityKeys(ble_gap_sec_keyset_t* keys);
void freeMemoryForSecurityKeys(ble_gap_sec_keyset_t* keys);
#endif

/* Usage:
 *  char bigString[1000];
 *  char *p = bigString;
 *  bigString[0] = '\0';
 *  p = xstrcat(p,"John, ");
 *  p = xstrcat(p,"Paul, ");
 *  p = xstrcat(p,"George, ");
 *  p = xstrcat(p,"Joel ");
 */
char* xstrcat(char* dest, const char* src);
char* byteToHex(unsigned char* bytes, char* buffer, char* separator, unsigned long count);
int twoByteEncode(unsigned char* msmtBuf, int index, unsigned short value);
int fourByteEncode(unsigned char* msmtBuf, int index, unsigned long value);
bool hexToLittleEndianByte(char* hexString, unsigned char* byteArray);
#endif
