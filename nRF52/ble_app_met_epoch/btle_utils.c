
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "btle_utils.h"
#include "nomenclature.h"

#ifdef _WIN32
    #define NRF_LOG_DEBUG printf
    #include <windows.h>
#else
    #include "nrf_sdh.h"
    #include "nrf_log_ctrl.h"
    #include "handleSpecializations.h"
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
char* xstrcat(char* dest, const char* src)
{
    while (*dest) dest++;       // Increment the contents until it is 0 (string end)
    while (*dest++ = *src++);   // contents of src extracted, src incremented, place into contents of dest, dest incremented
    return --dest;
}

char* byteToHex(unsigned char* bytes, char* buffer, char* separator, unsigned long count)
{
    char* cptr = buffer;
    unsigned long i;
    char buf[4];
    *cptr = 0;
    for (i = 0; i < count - 1; i++)
    {
        sprintf(buf, "%02X", bytes[i]);
        cptr = xstrcat(cptr, buf);
        cptr = xstrcat(cptr, separator);
    }
    sprintf(buf, "%02X", bytes[i]);
    cptr = xstrcat(cptr, buf);
    return buffer;
}

int twoByteEncode(unsigned char* msmtBuf, int index, unsigned short value)
{
    msmtBuf[index++] = (unsigned char)(value & 0xFF);
    msmtBuf[index++] = (unsigned char)((value >> 8) & 0xFF);
    return index;
}

int fourByteEncode(unsigned char* msmtBuf, int index, unsigned long value)
{
    int m;
    for (m = 0; m < 4; m++)
    {
        msmtBuf[index++] = (unsigned char)(value & 0xFF);
        value = (value >> 8);
    }
    return index;
}

int sixByteEncode(unsigned char* msmtBuf, int index, unsigned long long value)
{
    int m;
    for (m = 0; m < 6; m++)
    {
        msmtBuf[index++] = (unsigned char)(value & 0xFF);
        value = (value >> 8);
    }
    return index;
}

bool hexToLittleEndianByte(char* hexString, unsigned char* byteArray)
{

    if ((hexString == NULL) || (byteArray == NULL))
    {
        NRF_LOG_DEBUG("One or both of the input parameters are NULL");
        return false;
    }
    size_t length = strlen(hexString);
    if ((length & 1) == 1)
    {
        NRF_LOG_DEBUG("Hex String length is not even\n");
        return false;
    }
    for (int i = 0; i < length; i++)
    {
        if ((hexString[i] >= 0x30) && hexString[i] <= 0x39)
        {
            continue;
        }
        if ((hexString[i] >= 'A') && hexString[i] <= 'F')
        {
            continue;
        }
        if ((hexString[i] >= 'a') && hexString[i] <= 'f')
        {
            continue;
        }
        NRF_LOG_DEBUG("Not a legal HEX string");
        return false;
    }
    const char* pos = hexString;
    length = (length >> 1);

    for (size_t count = 1; count <= length; count++)
    {
        if (sscanf(pos, "%2hhx", &byteArray[length - count]) < 0)
        {
            NRF_LOG_DEBUG("Error converting the HEX string element at %d to a byte", (pos - hexString));
            return false;
        }
        pos += 2;
    }

    NRF_LOG_DEBUG("0x");
    for (size_t count = 0; count < length; count++)
    {
        NRF_LOG_DEBUG("%02x ", byteArray[count]);
    }
    NRF_LOG_DEBUG("");

    return true;
}

#if (HAS_ABS_TIMESTAMP)
    /**
      Helper array for absolute time to epoch converter.
      Raw date to 'epoch' converter. day = 0 at 01 01 2000. Good for years between 2000 to 2100
    */
    const static unsigned short days[4][12] =
    {
        {   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
        { 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},
        { 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
        {1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
    };

    /**
      Helper method to convert an absolute time to an 'epoch' where 0 is 01 01 2000.
      This method is only valid between the year 2000 to 2100.
     */
    unsigned long long date_time_to_epoch(s_Abs* date_time)
    {
        unsigned long hundredths = date_time->hundredth;// 0-99
        unsigned long second = date_time->sec;          // 0-59
        unsigned long minute = date_time->min;          // 0-59
        unsigned long hour   = date_time->hour;         // 0-23
        unsigned long day    = date_time->day - 1;      // 0-30
        unsigned long month  = date_time->month - 1;    // 0-11
        unsigned long year   = date_time->year;         // 0-99
        unsigned long long epoch = (year / 4) * (365 * 4 + 1);
        epoch = epoch + days[year % 4][month] + day;
        epoch = epoch * 24;
        epoch = (epoch + hour) * 60;
        epoch = (epoch + minute) * 60;
        epoch = (epoch + second) * 1000;
        epoch = epoch + hundredths * 10;
        return epoch;
    }

    void epoch_to_date_time(s_Abs* date_time, unsigned long long epoch)
    {
        date_time->hundredth = (unsigned char)((epoch % 1000) / 10);
        epoch = epoch / 1000;
        date_time->sec = epoch % 60; 
        epoch /= 60;
        date_time->min = epoch % 60; 
        epoch /= 60;
        date_time->hour   = epoch%24; 
        epoch /= 24;

        const unsigned int years = (unsigned int)(epoch/( 365 * 4 + 1) * 4);
        epoch %= 365 * 4 + 1;

        unsigned int year;
        for (year = 3; year > 0; year--)
        {
            if (epoch >= days[year][0])
                break;
        }

        unsigned int month;
        for (month = 11; month > 0; month--)
        {
            if (epoch >= days[year][month])
                break;
        }
        date_time->century = 20;
        date_time->year  = years + year;
        date_time->month = month + 1;
        date_time->day   = (unsigned char)(epoch - days[year][month] + 1);
    }

    bool absBytesToAbsStruct(s_Abs **absPtr, unsigned char *timeBytesIn)
    {
        if (absPtr == NULL || (*absPtr) == NULL || timeBytesIn == NULL)
        {
            NRF_LOG_DEBUG("At least one of the input parameters was NULL");
            return false;
        }
        s_Abs *abs = (*absPtr);
        abs->century =timeBytesIn[0];
        abs->year = timeBytesIn[1];
        abs->month = timeBytesIn[2];
        abs->day = timeBytesIn[3];
        abs->hour = timeBytesIn[4];
        abs->min = timeBytesIn[5];
        abs->sec = timeBytesIn[6];
        abs->hundredth = timeBytesIn[7];
        *absPtr = abs;
        return true;
    }

    bool absStructToAbsBytes(unsigned char **timeBytesPtr, s_Abs *absIn)
    {
        if (timeBytesPtr == NULL || (*timeBytesPtr) == NULL || absIn == NULL)
        {
            NRF_LOG_DEBUG("At least one of the input parameters was NULL");
            return false;
        }
        unsigned char *timeBuf = *timeBytesPtr;
        timeBuf[0] = absIn->century;
        timeBuf[1] = absIn->year;
        timeBuf[2] = absIn->month;
        timeBuf[3] = absIn->day;
        timeBuf[4] = absIn->hour;
        timeBuf[5] = absIn->min;
        timeBuf[6] = absIn->sec;
        timeBuf[7] = absIn->hundredth;
        *timeBytesPtr = timeBuf;
        return true;
    }


    /*
      This method handles 'date time adjustments' so the PHG never has to. It is a lot easier
      for th ePHD to do this task as it has all the information needed, and it does not have to
      pass all the information to the PHG so it can do it.

      This method is called after the PHG sets the time. The current time line of the PHD is
      passed in oldTimeIn and the new one written by the PHG is newTimeIn. This method takes
      the difference between the two, and adds that difference to the time stamps of all stored
      data time stamps (stored data must have time stamps). Setting the time is only an option
      for absolute and base offset times.
    */
    void adjustTimeStamps(unsigned char* oldTimeIn, unsigned char* newTimeIn, s_Abs **absTimePtr)
    {
        s_Abs oldTime;
        s_Abs newTime;
        s_Abs *oldTimePtr = &oldTime;
        s_Abs *newTimePtr = &newTime;
        s_Abs *absTime = *absTimePtr;

        absBytesToAbsStruct(&oldTimePtr, oldTimeIn); // Convert bytes to struct
        absBytesToAbsStruct(&newTimePtr, newTimeIn); // convert bytes to struct

        unsigned long long timeSinceEpochOld = date_time_to_epoch(&oldTime);
        unsigned long long timeSinceEpochNew = date_time_to_epoch(&newTime);
        unsigned long long diff = timeSinceEpochNew - timeSinceEpochOld;
        if (diff == 0)
        {
            return;
        }
        char buf[28];

        memset(buf, 0, 28);
        unsigned char timeBuf[8];
        unsigned char *timeBufPtr = timeBuf;
    
        // Only needed for diagnostics to show time before adjustment
        //absStructToAbsBytes(&timeBuf, absTime); // Convert struct to bytes
        //NRF_LOG_DEBUG("Time before adjustment %s", (uint32_t)byteToHex(timeBuf, buf, " ", 8));

        unsigned long long epoch = date_time_to_epoch(absTime) + diff;
        epoch_to_date_time(absTime, epoch);
        absStructToAbsBytes(&timeBufPtr, absTime); // Convert struct to bytes

          //  NRF_LOG_DEBUG("Time after adjustment %s", (uint32_t)byteToHex(timeBuf, buf, " ", 8));
        *absTimePtr = absTime;
    }
#endif

#ifndef _WIN32
// Generic method to create a primary service
ret_code_t createPrimaryService(unsigned short* serviceHandle, unsigned short serviceUuid)
{
    ret_code_t    error_code;
    ble_uuid_t  ble_uuid;

    BLE_UUID_BLE_ASSIGN(ble_uuid, serviceUuid);

    error_code = sd_ble_gatts_service_add(
        BLE_GATTS_SRVC_TYPE_PRIMARY,
        &ble_uuid,
        serviceHandle);

    if (error_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Failed to initialize service. Error code: 0x%02X", error_code);
        return error_code;
    }
    NRF_LOG_DEBUG("Service initiated");
    return NRF_SUCCESS;
}

// Generic method to create a characteristic and add it to a service
ret_code_t createStandardCharacteristic(
    uint16_t serviceHandle,
    ble_gatts_char_handles_t* charHandles,
    uint16_t uuid,
    bool hasCCCD,
    uint8_t indicate,
    uint16_t valueLength,
    uint8_t *value,
    uint8_t trapReadWrite,
    ble_gap_conn_sec_mode_t cccdWritePermission,
    ble_gap_conn_sec_mode_t attrReadPermission,
    ble_gap_conn_sec_mode_t attrWritePermission,
    bool isStatic)
{
    ret_code_t           error_code;
    ble_gatts_char_md_t  char_md;
    ble_gatts_attr_md_t  cccd_md;
    ble_gatts_attr_md_t* cccd_md_ptr = NULL;
    ble_gatts_attr_t     attr_char_value;
    ble_uuid_t           ble_uuid;
    ble_gatts_attr_md_t  attr_md;
    uint8_t              dummy = 1;

    if (hasCCCD)
    {
        memset(&cccd_md, 0, sizeof(cccd_md));

        cccd_md.rd_auth = 0;
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
        cccd_md.write_perm.lv = cccdWritePermission.lv;
        cccd_md.write_perm.sm = cccdWritePermission.sm;
        cccd_md.vloc = BLE_GATTS_VLOC_STACK;
        cccd_md_ptr = &cccd_md;
    }

    memset(&char_md, 0, sizeof(char_md));

    if (hasCCCD)
    {
        if (indicate & BLE_GATT_HVX_INDICATION)
        {
            char_md.char_props.indicate = 1;
        }
        if (indicate & BLE_GATT_HVX_NOTIFICATION)
        {
            char_md.char_props.notify = 1;
        }
    }
    char_md.char_props.read = ((attrReadPermission.sm == 0) ? 0 : 1);
    char_md.char_props.write = ((attrWritePermission.sm == 0) ? 0 : 1);
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = cccd_md_ptr;
    char_md.p_sccd_md = NULL;

    BLE_UUID_BLE_ASSIGN(ble_uuid, uuid);

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm.sm = attrReadPermission.sm;
    attr_md.read_perm.lv = attrReadPermission.lv;
    attr_md.write_perm.sm = attrWritePermission.sm;
    attr_md.write_perm.lv = attrWritePermission.lv;

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = ((TRAP_BOTH == trapReadWrite) || (TRAP_READ == trapReadWrite)) ? 1 : 0;
    attr_md.wr_auth = ((TRAP_BOTH == trapReadWrite) || (TRAP_WRITE == trapReadWrite)) ? 1 : 0;
    attr_md.vlen = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = (valueLength == 0) ? 1 : valueLength; //attr_char_value_init_len;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = isStatic? (valueLength + OPCODE_LENGTH + HANDLE_LENGTH) : (NRF_SDH_BLE_GAP_DATA_LENGTH - 4 - OPCODE_LENGTH - HANDLE_LENGTH);
    if ((value == NULL) || (valueLength == 0))
    {
        attr_char_value.p_value = &dummy;    // Fill with any garbage
    }
    else
    {
        NRF_LOG_DEBUG("char val length %u", valueLength);
        attr_char_value.p_value = value;
    }

    error_code = sd_ble_gatts_characteristic_add(serviceHandle,
        &char_md,
        &attr_char_value,
        charHandles);

    if (error_code != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Failed to initialize characteristics. Error code: 0x%02X", error_code);
        return error_code;
    }
    NRF_LOG_DEBUG("Characteristic 0x%04X initiated", uuid);

    return NRF_SUCCESS;
}

extern unsigned short SPECIALIZATION;
extern unsigned short numberOfStoredMsmtGroups;
extern const char nameKey[10];
#if (HEART_RATE != 1 && SPIROMETER != 1)
    extern s_MsmtData storedMsmts[];
#endif


extern unsigned long prevCount; // Needed on softdevice disable. Counter is reset to 0
extern unsigned long long accumulatedCycleCount;
unsigned long long getRtcCount(void);
void saveKeysToFlash(ble_gap_sec_keyset_t* keys,
    unsigned char **saveDataBuffer, unsigned short int *saveDataLength,
    unsigned char* cccdSet, unsigned short* noOfCccds)
{
    int i;
    unsigned char *keysDataBuffer = NULL;
    char *buffer = calloc(1, 256);
    ret_code_t err_code;

    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pg_num  = NRF_FICR->CODESIZE - (NRF_FICR->CODESIZE * pg_size - 0xE0000) / pg_size;
    
    // Okay, what are we doing here. On the dongle, the bootloader occupies flash above 0xE0000. So I have to find how big flash is,
    // subtract 0xE0000 from it, find the number of pages there, and then subtract that number of pages from the total. THAT gives the
    // page where the boot record starts (0xE0000). Now I have to drop a few pages to where I can start. I drop two pages here
    // I still don't know how to dynamically find the page at the top of MY app which gives the
    // area of free flash between my app and the bootloader at the top. The DK, by the way, has no bootloader so I don't need all this.

    NRF_LOG_INFO("Total pages is %u, pg_num is %u pg_size is %u", NRF_FICR->CODESIZE, pg_num, NRF_FICR->CODEPAGESIZE);
    pg_num = pg_num - 2;
    
    uint32_t *addr;
    NRF_LOG_DEBUG("Saving data to flash.");

    int size = sizeof(nameKey) +
               sizeof(ble_gap_enc_key_t) +
               sizeof(ble_gap_id_key_t) +
               sizeof(ble_gap_enc_key_t) +
               sizeof(ble_gap_id_key_t) +
               sizeof(unsigned short) +      // number of CCCDs
               sizeof(bool) * (*noOfCccds) + // the cccdSet[]
               sizeof(unsigned short) +      // saveDataLength
               ((*saveDataBuffer != NULL) ? *saveDataLength * sizeof(unsigned char) : 0) +  // the saveData buffer
               sizeof(unsigned short) +      // specialization
               sizeof(unsigned short) +      // number of stored data
               sizeof(unsigned long long) +  // latest time count (for time line check)
               #if (HEART_RATE == 1 || SPIROMETER == 1 || USES_STORED_DATA == 0)
                   0;
               #else
                   (sizeof(s_MsmtData) * numberOfStoredMsmtGroups );
               #endif

    // The writes are done in 4-byte hunks so we have to even out the length
    size = 4 + ((size >> 2) << 2);
    keysDataBuffer = calloc(1, size);
    
    // Now load all the data we want to save into this buffer
    uint8_t *ptr = keysDataBuffer;
    memcpy(ptr, nameKey, sizeof(nameKey));          // Load identifier
    ptr = ptr + sizeof(nameKey);
    memcpy(ptr, keys->keys_own.p_enc_key, sizeof(ble_gap_enc_key_t)); // load our LTK
    ptr = ptr + sizeof(ble_gap_enc_key_t);
    memcpy(ptr, keys->keys_own.p_id_key, sizeof(ble_gap_id_key_t));   // load our LTK id
    ptr = ptr + sizeof(ble_gap_id_key_t);
    memcpy(ptr, keys->keys_peer.p_enc_key, sizeof(ble_gap_enc_key_t)); // load peer LTK
    ptr = ptr + sizeof(ble_gap_enc_key_t);
    memcpy(ptr, keys->keys_peer.p_id_key, sizeof(ble_gap_id_key_t));   // load peer LTK id
    ptr = ptr + sizeof(ble_gap_id_key_t);
    memcpy(ptr, noOfCccds, sizeof(unsigned short));             // Load number of CCCDs
    ptr = ptr + sizeof(unsigned short);
    memcpy(ptr, cccdSet, sizeof(unsigned char) * (*noOfCccds)); // Load the cccdSet[]
    ptr = ptr + sizeof(unsigned char) * (*noOfCccds);
    memcpy(ptr, saveDataLength, sizeof(unsigned short));        // Load saveDataLength
    ptr = ptr + sizeof(unsigned short);
    if (*saveDataBuffer != NULL)
    {
        memcpy(ptr, *saveDataBuffer, *saveDataLength * sizeof(unsigned char));   // Load the saveDataBuffer
        ptr = ptr + *saveDataLength * sizeof(unsigned char);
    }
    memcpy(ptr, &SPECIALIZATION, sizeof(unsigned short));               // Load specialization
    ptr = ptr + sizeof(unsigned short);
    memcpy(ptr, &numberOfStoredMsmtGroups, sizeof(unsigned short));     // Load number of stored measurements
    ptr = ptr + sizeof(unsigned short);
    memcpy(ptr, &latestTimeStamp, sizeof(unsigned long long));          // Load the latest time stamp for time line change check
    ptr = ptr + sizeof(unsigned long long);
    #if(USES_STORED_DATA == 1)
        if (numberOfStoredMsmtGroups > 0 && numberOfStoredMsmtGroups <= NUMBER_OF_STORED_MSMTS)
        {
            #if (HEART_RATE != 1 && SPIROMETER != 1)
                char buffer[512];
                memset(buffer, 0, 512);
                NRF_LOG_INFO("Stored data %s", (uint32_t)byteToHex((uint8_t *)storedMsmts, buffer, " ", sizeof(s_MsmtData)));
                NRF_LOG_FLUSH();
                memcpy(ptr, storedMsmts, sizeof(s_MsmtData) * numberOfStoredMsmtGroups); // Load the stored measurements
            #endif
        }
    #endif
    
    // Now we have to write the data in hunks into flash
    // Each page is 1024 bytes, and a write is in 4-byte hunks
    // So we will find the number of 1024 byte pages to write,
    // and then the number of 4-byte hunks left over.

    // The counter appears to hop back to zero causing a recycle adding 512 seconds to counter
    // Setting the previous count to 0 stops that recycle.
    getRtcCount();
    prevCount = 0;

    // Disable soft device so we don't have to deal with events
    //sd_softdevice_disable();
    nrf_sdh_disable_request();
    
    // Buffer to write to flash. Need to write it in four-byte hunks
    uint32_t *ptr32 = (uint32_t *)keysDataBuffer;
    while (true)
    {
        // Where to write
        addr = (uint32_t *)(pg_size * pg_num);
        // Erase page:
        while(true)
        {
            err_code = sd_flash_page_erase(pg_num);
            if (err_code == NRF_SUCCESS)
            {
                break;
            }
            if (err_code != NRF_ERROR_BUSY)
            {
                NRF_LOG_DEBUG("Erasing data returned error %u.", err_code);
                APP_ERROR_CHECK(err_code);
            }
        }
        i = (size >= pg_size) ? (pg_size >> 2) : (size >> 2);     // four-byte hunks to write; size is evenly divisible by four
        while(true)
        {
            err_code = sd_flash_write(addr, ptr32, i);
            if (err_code == NRF_SUCCESS)
            {
                break;
            }
            if (err_code != NRF_ERROR_BUSY)
            {
                NRF_LOG_DEBUG("Writing data returned error %u.", err_code);
                APP_ERROR_CHECK(err_code);
            }
        }
        size = size - pg_size;  // Subtract a page size from the total size
        if (size <= 0)          // if zero or less, all data has been written
        {
            break;
        }
        pg_num++;
        ptr32 = (uint32_t *)(keysDataBuffer + pg_size);
    }
    free(keysDataBuffer);
    NRF_LOG_DEBUG("Flash written\n\n\n\n");
}

void loadKeysFromFlash(ble_gap_sec_keyset_t* keys,
    unsigned char** saveDataBuffer, unsigned short int* saveDataLength,
    unsigned char* cccdSet, unsigned short *noOfCccds)
{
    unsigned short specialization;
    char localNameKey[10];

    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pg_num  = NRF_FICR->CODESIZE - (NRF_FICR->CODESIZE * pg_size - 0xE0000) / pg_size;
    
    // Okay, what are we doing here. On the dongle, the bootloader occupies flash above 0xE0000. So I have to find how big flash is,
    // subtract 0xE0000 from it, find the number of pages there, and then subtract that number of pages from the total. THAT gives the
    // page where the boot record starts (0xE0000). Now I have to drop a few pages to where I can start. I drop two pages here
    // I still don't know how to dynamically find the page at the top of MY app which gives the
    // area of free flash between my app and the bootloader at the top. The DK, by the way, has no bootloader so I don't need all this.

    NRF_LOG_INFO("Total pages is %u, pg_num is %u pg_size is %u", NRF_FICR->CODESIZE, pg_num, NRF_FICR->CODEPAGESIZE);
    pg_num = pg_num - 2;
    uint8_t *addr = (uint8_t *)(pg_size * pg_num);

    memcpy(localNameKey, addr, sizeof(localNameKey));
    if (memcmp(localNameKey, nameKey, sizeof(nameKey)) != 0)
    {
        return;     // nothing written to Flash yet or not what it should be
    }
    addr = addr + sizeof(nameKey);
    memcpy(keys->keys_own.p_enc_key, addr, sizeof(ble_gap_enc_key_t));
    addr = addr + sizeof(ble_gap_enc_key_t);
    memcpy(keys->keys_own.p_id_key, addr, sizeof(ble_gap_id_key_t));
    addr = addr + sizeof(ble_gap_id_key_t);
    memcpy(keys->keys_peer.p_enc_key, addr, sizeof(ble_gap_enc_key_t));
    addr = addr + sizeof(ble_gap_enc_key_t);
    memcpy(keys->keys_peer.p_id_key, addr, sizeof(ble_gap_id_key_t));
    addr = addr + sizeof(ble_gap_id_key_t);
    memcpy(noOfCccds, addr, sizeof(unsigned short));
    addr = addr + sizeof(unsigned short);
    memcpy(cccdSet, addr, sizeof(unsigned char) * (*noOfCccds)); // Load the cccdSet[]
    addr = addr + sizeof(unsigned char) * (*noOfCccds);
    memcpy(saveDataLength, addr, sizeof(unsigned short));        // Load saveDataLength
    addr = addr + sizeof(unsigned short);
    if (*saveDataLength > 0)
    {
        *saveDataBuffer = calloc(1, *saveDataLength * sizeof(unsigned char));
        memcpy(*saveDataBuffer, addr, *saveDataLength * sizeof(unsigned char)); // Load the saveDataBuffer
        addr = addr + *saveDataLength * sizeof(unsigned char);
    }
    memcpy(&specialization, addr, sizeof(unsigned short));       // Load specialization: Only needed to allow switching of device types on same chip
    addr = addr + sizeof(unsigned short);
    if (specialization == SPECIALIZATION)  // Only needed to allow switching of device types on same chip
    {
//        memcpy(&timeSync, addr, sizeof(unsigned short));        // Load timeSync
//        addr = addr + sizeof(unsigned short);
        memcpy(&numberOfStoredMsmtGroups, addr, sizeof(unsigned short));            // Load number of stored measurements
        addr = addr + sizeof(unsigned short);
        memcpy(&latestTimeStamp, addr, sizeof(unsigned long long));
        addr = addr + sizeof(unsigned long long);
        #if(USES_STORED_DATA == 1)
            if (numberOfStoredMsmtGroups > 0 && numberOfStoredMsmtGroups <= NUMBER_OF_STORED_MSMTS)
            {
                #if (HEART_RATE != 1 && SPIROMETER != 1)
                    char buffer[512];
                    memset(buffer, 0, 512);
                    memcpy(storedMsmts, addr, numberOfStoredMsmtGroups * sizeof(s_MsmtData));// Load the stored measurements
                    NRF_LOG_INFO("Stored data %s", (uint32_t)byteToHex((uint8_t *)storedMsmts, buffer, " ", sizeof(s_MsmtData)));
                    NRF_LOG_FLUSH();
                #endif
            }
        #endif
    }
}

void allocateMemoryForSecurityKeys(ble_gap_sec_keyset_t* keys)
{
    keys->keys_own.p_enc_key = calloc(1, sizeof(ble_gap_enc_key_t));
    keys->keys_own.p_id_key = calloc(1, sizeof(ble_gap_id_key_t));
    keys->keys_own.p_sign_key = calloc(1, sizeof(ble_gap_sign_info_t));
    keys->keys_own.p_pk = calloc(1, sizeof(ble_gap_lesc_p256_pk_t));

    keys->keys_peer.p_enc_key = calloc(1, sizeof(ble_gap_enc_key_t));
    keys->keys_peer.p_id_key = calloc(1, sizeof(ble_gap_id_key_t));
    keys->keys_peer.p_sign_key = calloc(1, sizeof(ble_gap_sign_info_t));
    keys->keys_peer.p_pk = calloc(1, sizeof(ble_gap_lesc_p256_pk_t));
}

void clearSecurityKeys(ble_gap_sec_keyset_t* keys)
{
    memset(keys->keys_own.p_enc_key, 0, sizeof(ble_gap_enc_key_t));
    memset(keys->keys_own.p_id_key, 0, sizeof(ble_gap_id_key_t));
    memset(keys->keys_own.p_sign_key, 0, sizeof(ble_gap_sign_info_t));
    memset(keys->keys_own.p_pk, 0, sizeof(ble_gap_lesc_p256_pk_t));

    memset(keys->keys_peer.p_enc_key, 0, sizeof(ble_gap_enc_key_t));
    memset(keys->keys_peer.p_id_key, 0, sizeof(ble_gap_id_key_t));
    memset(keys->keys_peer.p_sign_key, 0, sizeof(ble_gap_sign_info_t));
    memset(keys->keys_peer.p_pk, 0, sizeof(ble_gap_lesc_p256_pk_t));
}

void freeMemoryForSecurityKeys(ble_gap_sec_keyset_t* keys)
{
    if (keys->keys_own.p_enc_key != NULL) free(keys->keys_own.p_enc_key);
    if (keys->keys_own.p_id_key != NULL) free(keys->keys_own.p_id_key);
    if (keys->keys_own.p_sign_key != NULL) free(keys->keys_own.p_sign_key);
    if (keys->keys_own.p_pk != NULL) free(keys->keys_own.p_pk);
    
    if (keys->keys_peer.p_enc_key != NULL) free(keys->keys_peer.p_enc_key);
    if (keys->keys_peer.p_id_key != NULL) free(keys->keys_peer.p_id_key);
    if (keys->keys_peer.p_sign_key != NULL) free(keys->keys_peer.p_sign_key);
    if (keys->keys_peer.p_pk != NULL) free(keys->keys_peer.p_pk);
}
#endif


