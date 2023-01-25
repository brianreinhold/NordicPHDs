/* configEncoder.c */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "btle_utils.h"  // Contains "MetControlStructs.h" which is the only include method needed
#include "nomenclature.h"


#ifdef _WIN32
    #define NRF_LOG_DEBUG printf
    #include <windows.h>
#else
    #include "nrf_log.h"
#endif

#define GROUP_HEADER_LENGTH 6   // commsnd 2 flags 2 length 2
#define MSMT_HEADER_LENGTH 10   // type 4 length 2 flags 2 msmt id 2

#if (USES_SYSTEM_AVAS == 1 || USES_HEADERS_AVAS == 1 || USES_MSMT_AVAS == 1)
    static void cleanUpAvas(s_Avas** avas, unsigned short numberOfAvas)
    {
        if (numberOfAvas > 0 && avas != NULL)
        {
            int i;
            for (i = 0; i < numberOfAvas; i++)
            {
                if (avas[i] != NULL)
                {
                    if (avas[i]->value != NULL)
                    {
                        free(avas[i]->value);
                    }
                    free(avas[i]);
                }
            }
            free(avas);
            avas = NULL;
        }
    }

    static bool allocateAvas(s_Avas*** avasPtr, unsigned short numberOfAvas)
    {
        s_Avas** avas = *avasPtr;
        if (avas != NULL)
        {
            free(avas);
        }
        avas = (s_Avas**)calloc(1, numberOfAvas * sizeof(s_Avas*));
        if (avas == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for %d AVA pointers.\r\n", numberOfAvas);
            *avasPtr = NULL;
            return false;
        }
        *avasPtr = avas;
        return true;
    }

    static bool addAva(s_Avas** avasPtr, s_Avas* avaIn)
    {
        s_Avas* avas = *avasPtr;
        avas = (s_Avas*)calloc(1, sizeof(s_Avas));
        if (avas == NULL)
        {
            NRF_LOG_ERROR("Could not allocate memory for the Ava.\r\n");
            return false;
        }
        avas->value = (unsigned char*)calloc(1, avaIn->length * sizeof(unsigned char));
        if (avas->value == NULL)
        {
            free(avas);
            *avasPtr = NULL;
            NRF_LOG_ERROR("Could not allocate memory for the Ava value field of %d bytes.\r\n", avaIn->length);
            return false;
        }
        avas->attrId = avaIn->attrId;
        avas->length = avaIn->length;
        memcpy(avas->value, avaIn->value, avaIn->length);
        *avasPtr = avas; // put the allocated value back into the passed pointer to pointer.
        return true;
    }
#endif

static bool checkSystemInfo(s_SystemInfo *systemInfo)
{
    if (systemInfo == NULL)
    {
        NRF_LOG_DEBUG("SystemInfo not initialized. Need to call initializeSystemInfo\r\n");
        return false;
    }
    return true;
}

static bool checkMetMsmt(s_MetMsmt *metMsmt)
{
    if (metMsmt == NULL)
    {
        NRF_LOG_DEBUG("s_MetMsmt has not been initialized. Call one of the create measurement methods\r\n");
        return false;
    }
    return true;
}

static bool checkMsmtGroup(s_MsmtGroup *msmtGroup)
{
    if (msmtGroup == NULL)
    {
        NRF_LOG_DEBUG("s_MsmtGroup has not been initialized. Call initializeMsmtGroup()\r\n");
        return false;
    }
    return true;
}

static bool checkMsmtGroupData(s_MsmtGroupData* msmtGroupData)
{
    if (msmtGroupData == NULL)
    {
        NRF_LOG_DEBUG("s_MsmtGroupData has not been generated. Call createMsmtGroupDataArray()\r\n");
        return false;
    }
    return true;
}

//======================= Time Info
#if (USES_TIMESTAMP == 1)
    void cleanUpMetTime(s_MetTime **sMetTimePtr)
    {
        s_MetTime* sMetTime = *sMetTimePtr;
        if (sMetTime != NULL)
        {
            free(sMetTime);
           *sMetTimePtr = NULL;
        }
    }

    void cleanUpTimeInfo(s_TimeInfo** timeInfoPtr)
    {
        s_TimeInfo* timeInfo = *timeInfoPtr;
        if (timeInfo != NULL)
        {
            //if (timeInfo->metTime != NULL)
            //{
            //    free(timeInfo->metTime);
            //    timeInfo->metTime = NULL;
            //}
            free(timeInfo);
            *timeInfoPtr = NULL;
        }
    }

    void cleanUpTimeInfoData(s_TimeInfoData** timeInfoDataPtr)
    {
        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
        if (timeInfoData != NULL)
        {
            if (timeInfoData->timeInfoBuf != NULL)
            {
                free(timeInfoData->timeInfoBuf);
            }
            free(timeInfoData);
            *timeInfoDataPtr = NULL;
        }
    }

    bool createMetTime(s_MetTime **sMetTimePtr, short offsetShift, unsigned short clockType, unsigned short clockResolution, unsigned short timeSync)
    {
        if (sMetTimePtr == NULL)
        {
            NRF_LOG_DEBUG("Input sMetTimePtr was NULL\r\n");
            return false;
        }
        s_MetTime* sMetTime = *sMetTimePtr;
        if (sMetTime != NULL)
        {
            cleanUpMetTime(sMetTimePtr);
        }
        sMetTime = (s_MetTime *)calloc(1, sizeof(s_MetTime));
        if (sMetTime == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for the s_MetTime struct\r\n");
            return false;
        }
        sMetTime->clockType = clockType;
        sMetTime->clockResolution = clockResolution;
        sMetTime->offsetShift = offsetShift;
        sMetTime->timeSync = timeSync;
        *sMetTimePtr = sMetTime;
        return true;
    }

    bool createTimeInfo(s_TimeInfo **sTimeInfoPtr, s_MetTime *sMetTime, bool allowSetTime)
    {
        s_TimeInfo* sTimeInfo = *sTimeInfoPtr;
        if (sTimeInfo != NULL)
        {
            cleanUpTimeInfo(sTimeInfoPtr);
        }
        sTimeInfo = (s_TimeInfo *)calloc(1, sizeof(s_TimeInfo));
        if (sTimeInfo == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for the s_TimeInfo struct\r\n");
            return false;
        }
        if (sMetTime != NULL)
        {
            sTimeInfo->timeFlagsSetTime = allowSetTime ? TIME_FLAGS_SUPPORTS_SET_TIME : 0;
            sTimeInfo->metTime = sMetTime;
        }
        *sTimeInfoPtr = sTimeInfo;
        return true;
    }

    bool createCurrentTimeDataBuffer(s_TimeInfoData** timeInfoDataPtr, s_TimeInfo *sTimeInfo)
    {

        if (sTimeInfo == NULL)
        {
            NRF_LOG_DEBUG("TimeInfo struct has not been initialized\r\n");
            return false;
        }
        unsigned short index = 0;
        unsigned short flags = (unsigned short)(sTimeInfo->timeFlagsSetTime | sTimeInfo->timeFlagsHasAvas);  // AVA support not implemented yet
        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
        if (timeInfoData != NULL)
        {
            cleanUpTimeInfoData(timeInfoDataPtr);
        }
        timeInfoData = (s_TimeInfoData *)calloc(1, sizeof(s_TimeInfoData));
        if (timeInfoData == NULL)
        {
            NRF_LOG_ERROR("Could not allocate memory for time info Data struct\r\n");
            return false;
        }
        // command|flags|length|MET Time|AVAs|  = |2|2|2|MET_TIME_LENGTH|variable
        unsigned short timeInfoLength = 6 + MET_TIME_LENGTH;
        timeInfoData->timeInfoBuf = (unsigned char *)calloc(1, timeInfoLength * sizeof(unsigned char));
        if (timeInfoData->timeInfoBuf == NULL)
        {
            NRF_LOG_ERROR("Could not allocate memory for time info buffer in the Data struct\r\n");
            return false;
        }
        timeInfoData->currentTime_index = -1;
        timeInfoData->dataLength = (unsigned short)timeInfoLength;
        index = twoByteEncode(timeInfoData->timeInfoBuf, index, COMMAND_GET_CURRENT_TIME);  // encode command
        timeInfoData->flags_index = index;
        index = twoByteEncode(timeInfoData->timeInfoBuf, index, flags);                     // encode flags
        index = twoByteEncode(timeInfoData->timeInfoBuf, index, (timeInfoLength - 6));      // encode length


        timeInfoData->currentTime_index = index;
        timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_FLAGS] = (sTimeInfo->metTime->clockResolution | sTimeInfo->metTime->clockType);
        timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_OFFSET] = (unsigned char)(sTimeInfo->metTime->offsetShift & 0xFF);
        index = index + MET_TIME_INDEX_TIME_SYNC;      // Now point to time sync
        index = twoByteEncode(timeInfoData->timeInfoBuf, index, sTimeInfo->metTime->timeSync);

        *timeInfoDataPtr = timeInfoData;
        return true;
    }

    bool updateCurrentTimeFromSetTime(s_TimeInfoData** timeInfoDataPtr, unsigned char *update)
    {
        if (timeInfoDataPtr == NULL || *timeInfoDataPtr == NULL || update == NULL)
        {
            NRF_LOG_DEBUG("TimeInfo struct has not been initialized or timeInfoDataPtr itself was NULL\r\n");
            return false;
        }

        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;

        int index = timeInfoData->currentTime_index;
        memcpy(&timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_EPOCH], &update[MET_TIME_INDEX_EPOCH], 6);   // PHG's Epoch
        // flags field left alone
        if (timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_OFFSET] != MET_TIME_OFFSET_UNSUPPORTED)    // Set to PHG's offset if PHD supports Offset
        {
            timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_OFFSET] = update[MET_TIME_INDEX_OFFSET];
        }
        memcpy(&timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_TIME_SYNC], &update[MET_TIME_INDEX_TIME_SYNC], 2);    // PHG's time sync
        *timeInfoDataPtr = timeInfoData;
        return true;
    }


    static bool checkValid(s_TimeInfoData** timeInfoDataPtr)
    {
        if (timeInfoDataPtr == NULL || *timeInfoDataPtr == NULL || (*timeInfoDataPtr)->dataLength < MET_TIME_LENGTH + 6)
        {
            NRF_LOG_DEBUG("TimeInfo struct has not been initialized or timeInfoDataPtr itself was NULL or time not supported\r\n");
            return false;
        }
        return true;
    }


    bool updateCurrentTimeFromPhdOffset(s_TimeInfoData** timeInfoDataPtr, short offsetShift)
    {
        if (!checkValid(timeInfoDataPtr))
        {
            return false;
        }

        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
        int index = timeInfoData->currentTime_index;
        timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_OFFSET] = (unsigned char)offsetShift;
        *timeInfoDataPtr = timeInfoData;
        return true;
    }

    bool updateCurrentTimeFromPhdTimeSync(s_TimeInfoData** timeInfoDataPtr, unsigned short timeSync)
    {
        if (!checkValid(timeInfoDataPtr))
        {
            return false;
        }

        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
        int index = timeInfoData->currentTime_index + MET_TIME_INDEX_TIME_SYNC;
        timeInfoData->timeInfoBuf[index] = (timeSync & 0xFF);
        timeInfoData->timeInfoBuf[index + 1] = ((timeSync >> 8) & 0xFF);
        *timeInfoDataPtr = timeInfoData;
        return true;
    }

    bool updateCurrentTimeEpoch(s_TimeInfoData** timeInfoDataPtr, unsigned long long epoch)
    {
        if (!checkValid(timeInfoDataPtr))
        {
            return false;
        }

        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
        int index = timeInfoData->currentTime_index;
        int i;
        for (i = 0; i < 6; i++)
        {
            timeInfoData->timeInfoBuf[index + MET_TIME_INDEX_EPOCH + i] = (epoch & 0xFF);
            epoch = (epoch >> 8);
        }
        *timeInfoDataPtr = timeInfoData;
        return true;
    }

    unsigned short getTimeSync(s_TimeInfoData* sTimeInfoData)
    {
        if (sTimeInfoData != NULL && sTimeInfoData->dataLength > 6)
        {
            return sTimeInfoData->timeInfoBuf[MET_TIME_INDEX_TIME_SYNC] + (((unsigned short)sTimeInfoData->timeInfoBuf[MET_TIME_INDEX_TIME_SYNC + 1]) << 8);
        }
        return MDC_TIME_SYNC_NONE;
    }
#endif

// ====================== System Info
void cleanUpSystemInfo(s_SystemInfo** systemInfoPtr)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (systemInfo == NULL) return;
    if (systemInfo->specializations != NULL)
    {
        free(systemInfo->specializations);
    }
    #if (USES_SYSTEM_AVAS == 1)
    cleanUpAvas(systemInfo->avas, systemInfo->numberOfAvas);
    #endif
    free(systemInfo);
    *systemInfoPtr = NULL;
}

void cleanUpSystemInfoData(s_SystemInfoData** systemInfoDataPtr)
{
    s_SystemInfoData* systemInfoData = *systemInfoDataPtr;
    if (systemInfoData != NULL)
    {
        if (systemInfoData->systemInfoBuf != NULL)
        {
            free(systemInfoData->systemInfoBuf);
        }
        free(systemInfoData);
        *systemInfoDataPtr = NULL;
    }
}

bool createSystemInfo(s_SystemInfo **systemInfoPtr, unsigned short numberOfSpecializations)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    systemInfo = (s_SystemInfo *)calloc(1, sizeof(s_SystemInfo));
    if (systemInfo == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for the s_SystemInfo struct\r\n");
        return false;
    }
    systemInfo->currentSpecializationCount = 0;
    systemInfo->numberOfSpecializations = numberOfSpecializations;
    systemInfo->specializations = (s_Specialization *)calloc(1, numberOfSpecializations * sizeof(s_Specialization));
    if (systemInfo->specializations == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for the number of specializations %d\r\n", numberOfSpecializations);
        free(systemInfo);
        *systemInfoPtr = NULL;
        return false;
    }
    *systemInfoPtr = systemInfo;
    return true;
}

bool addSpecialization(s_SystemInfo **systemInfoPtr, unsigned short specializationTermCode, unsigned short version)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;

    if (systemInfo->currentSpecializationCount >= systemInfo->numberOfSpecializations)
    {
        NRF_LOG_DEBUG("Number of specializations exceeded. Cannot add more. Skipping.\r\n");
        return false;
    }
    systemInfo->specializations[systemInfo->currentSpecializationCount].specialization = specializationTermCode;
    systemInfo->specializations[systemInfo->currentSpecializationCount++].version = version;
    return true;
}

bool setRequiredSystemInfoStrings(s_SystemInfo **systemInfoPtr, char *manufacturerName, char *modelNumber)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;

    systemInfo->manufacturer = manufacturerName;
    systemInfo->modelNumber = modelNumber;
    return true;
}

bool setOptionalSystemInfoStrings(s_SystemInfo **systemInfoPtr, char *serialNumber, char *firmware, char *hardware, char *software)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;
    if (serialNumber != NULL)
    {
        systemInfo->serialNo = serialNumber;
        systemInfo->flagsSerialNo = SYSINFO_FLAGS_SERIAL_NO;
    }
    if (firmware != NULL)
    {
        systemInfo->firmware = firmware;
        systemInfo->flagsFirmware = SYSINFO_FLAGS_FIRMWARE;
    }
    if (hardware != NULL)
    {
        systemInfo->hardware = hardware;
        systemInfo->flagsHardware = SYSINFO_FLAGS_HARDWARE;
    }
    if (software != NULL)
    {
        systemInfo->software = software;
        systemInfo->flagsSoftware = SYSINFO_FLAGS_SOFTWARE;
    }
    return true;
}

bool setUdi(s_SystemInfo **systemInfoPtr, char* udi_label, char* udi_dev_id, char* udi_issuer_oid, char* udi_auth_oid)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;

    if (udi_label == NULL && udi_dev_id == NULL && udi_issuer_oid == NULL && udi_auth_oid == NULL)
    {
        return true;
    }
    if (udi_label != NULL)
    {
        systemInfo->udi_label = udi_label;
        systemInfo->flagsUdiLabel = UDI_FLAGS_LABEL;
    }
    if (udi_dev_id != NULL)
    {
        systemInfo->udi_device_identifier = udi_dev_id;
        systemInfo->flagsUdiDevId = UDI_FLAGS_DEV_ID;
    }
    if (udi_issuer_oid != NULL)
    {
        systemInfo->udi_issuer_oid = udi_issuer_oid;
        systemInfo->flagsUdiIssuer = UDI_FLAGS_ISSUER;
    }
    if (udi_auth_oid != NULL)
    {
        systemInfo->udi_authority_oid = udi_auth_oid;
        systemInfo->flagsUdiAuthority = UDI_FLAGS_AUTH;
    }
    return true;
}

/**
 *  The set value 'isRegulated' is only used if 'hasRegulationStatus' is true.
 */
bool setRegulationStatus(s_SystemInfo **systemInfoPtr, bool hasRegulationStatus, bool isRegulated)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;
    
    if (hasRegulationStatus)
    {
        systemInfo->flagsRegStatus = SYSINFO_FLAGS_REG_STATUS;
        systemInfo->regulationStatus = isRegulated ? 0 : 0x8000;
    }
    return true;
}

/**
 * 16-digit HEX string. This method will put the systme id in little endian order as bytes.
 */
bool setSystemIdentifier(s_SystemInfo **systemInfoPtr, char *systemId)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;
    if (!hexToLittleEndianByte(systemId, systemInfo->systemId))
    {
        NRF_LOG_DEBUG("System Id was not properly encoded. Value will be all 0's.\r\n");
        return false;
    }
    return true;
}

bool setSystemIdentifierByte(s_SystemInfo **systemInfoPtr, unsigned char *systemId)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (systemId == NULL) return false;
    memcpy(systemInfo->systemId, systemId, 8);  // destination, source, length
    return true;
}

#if (USES_SYSTEM_AVAS == 1)
bool initializeSystemInfoAvas(s_SystemInfo **systemInfoPtr, unsigned short numberOfAvas)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;

    systemInfo->numberOfAvas = numberOfAvas;
    systemInfo->currentAvaCount = 0;
    if (numberOfAvas > 0)
    {
        if (!allocateAvas(&systemInfo->avas, numberOfAvas))
        {
            systemInfo->numberOfAvas = 0;
        }
    }
    return true;
}

bool addSystemInfoAvas(s_SystemInfo **systemInfoPtr, s_Avas *avaIn)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;
    if (systemInfo->numberOfAvas == 0)
    {
        NRF_LOG_DEBUG("SystemInfo AVAs not initialized or Ava initialization failed.\r\n");
        return false;
    }
    if (systemInfo->currentAvaCount >= systemInfo->numberOfAvas)
    {
        NRF_LOG_DEBUG("Number of System Info Avas have been exceeded those set during initialization. Cannot add any more.\r\n");
        return false;
    }
    if (addAva(&systemInfo->avas[systemInfo->currentAvaCount], avaIn))
    {
        systemInfo->currentAvaCount++;
        systemInfo->flagsAvas = SYSINFO_FLAGS_AVAS;
        return true;
    }
    return false;
}
#endif

int loadString(char *str, unsigned char *systemInfoBuf, int index)
{
    if (str != NULL)
    {
        unsigned char len = (unsigned char)strlen(str);
        systemInfoBuf[index++] = (unsigned char)(len & 0xFF);
        memcpy(&systemInfoBuf[index], str, len);
        return (index + len);
    }
    return index;
}

bool createSystemInfoData(s_SystemInfoData** systemInfoDataPtr, s_SystemInfo *systemInfo)
{
    s_SystemInfoData* systemInfoData = *systemInfoDataPtr;
    if (!checkSystemInfo(systemInfo)) return false;
    
    int i;
    unsigned short systemInfoLength = 14;   // command 2, flags 2, length 2, system id(8)
    unsigned short index = 0;
    systemInfoData = (s_SystemInfoData *)calloc(1, sizeof(s_SystemInfoData));
    if (systemInfoData == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for the System Info Index struct.\r\n");
        return false;
    }
    unsigned short flags = (unsigned short)(systemInfo->flagsAvas | systemInfo->flagsFirmware | systemInfo->flagsHardware 
                            | systemInfo->flagsSoftware | systemInfo->flagsSerialNo | systemInfo->flagsRegStatus
                            | systemInfo->flagsUdiLabel | systemInfo->flagsUdiDevId | systemInfo->flagsUdiIssuer
                            | systemInfo->flagsUdiAuthority);

    systemInfoLength = systemInfoLength + 1 + systemInfo->currentSpecializationCount * 4 +
        ((systemInfo->manufacturer != NULL) ? (1 + (unsigned short)strlen(systemInfo->manufacturer)) : 0) +
        ((systemInfo->modelNumber != NULL) ? (1 + (unsigned short)strlen(systemInfo->modelNumber)) : 0) +
    (((flags & SYSINFO_FLAGS_FIRMWARE) == SYSINFO_FLAGS_FIRMWARE) ? (1 + (unsigned short)strlen(systemInfo->firmware)) : 0) +
    (((flags & SYSINFO_FLAGS_HARDWARE) == SYSINFO_FLAGS_HARDWARE) ? (1 + (unsigned short)strlen(systemInfo->hardware)) : 0) +
    (((flags & SYSINFO_FLAGS_SOFTWARE) == SYSINFO_FLAGS_SOFTWARE) ? (1 + (unsigned short)strlen(systemInfo->software)) : 0) +
    (((flags & SYSINFO_FLAGS_SERIAL_NO) == SYSINFO_FLAGS_SERIAL_NO) ? (1 + (unsigned short)strlen(systemInfo->serialNo)) : 0) +
    (((flags & SYSINFO_FLAGS_REG_STATUS) == SYSINFO_FLAGS_REG_STATUS) ? 2 : 0) +
    (((flags & UDI_FLAGS_LABEL) == UDI_FLAGS_LABEL) ? (1 + (unsigned short)strlen(systemInfo->udi_label)) : 0) +
    (((flags & UDI_FLAGS_DEV_ID) == UDI_FLAGS_DEV_ID) ? (1 + (unsigned short)strlen(systemInfo->udi_device_identifier)) : 0) +
    (((flags & UDI_FLAGS_ISSUER) == UDI_FLAGS_ISSUER) ? (1 + (unsigned short)strlen(systemInfo->udi_issuer_oid)) : 0) +
    (((flags & UDI_FLAGS_AUTH) == UDI_FLAGS_AUTH) ? (1 + (unsigned short)strlen(systemInfo->udi_authority_oid)) : 0);
    #if (USES_SYSTEM_AVAS == 1)
    if (systemInfo->flagsAvas != 0)
    {
        unsigned short j;
        systemInfoLength++; // for the number of AVAs entry
        for (j = 0; j < systemInfo->currentAvaCount; j++)
        {
            systemInfoLength = systemInfoLength + systemInfo->avas[j]->length + 6;  // 4-byte id and 2-byte length + 1 byte # of AVAs
        }
    }
    #endif
    systemInfoData->systemInfoBuf = (unsigned char *)calloc(1, systemInfoLength * sizeof(unsigned char));
    if (systemInfoData->systemInfoBuf == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for the System Info data buffer.\r\n");
        free(systemInfoData);
        return false;
    }
    // Command
    index = twoByteEncode(systemInfoData->systemInfoBuf, index, COMMAND_GET_SYS_INFO);
    // flags
    index = twoByteEncode(systemInfoData->systemInfoBuf, index, flags);
    // PDU length
    systemInfoData->dataLength = (unsigned short)systemInfoLength;
    index = twoByteEncode(systemInfoData->systemInfoBuf, index, (systemInfoLength - 6));
    // System Id
    memcpy(&systemInfoData->systemInfoBuf[index], systemInfo->systemId, 8);
    index = index + 8;
    // Specializations
    systemInfoData->systemInfoBuf[index++] = (unsigned char)(systemInfo->currentSpecializationCount & 0xFF);
    for (i = 0; i < systemInfo->currentSpecializationCount; i++)
    {
        index = twoByteEncode(systemInfoData->systemInfoBuf, index, systemInfo->specializations[i].specialization);
        index = twoByteEncode(systemInfoData->systemInfoBuf, index, systemInfo->specializations[i].version);
    }
    // Manufacturer name
    index = loadString(systemInfo->manufacturer, systemInfoData->systemInfoBuf, index);
    // Model Number
    index = loadString(systemInfo->modelNumber, systemInfoData->systemInfoBuf, index);
    // Reg status
    if (systemInfo->flagsRegStatus != 0)
    {
        systemInfoData->systemInfoBuf[index++] = (systemInfo->regulationStatus & 0xFF);
        systemInfoData->systemInfoBuf[index++] = ((systemInfo->regulationStatus >> 8) & 0xFF);
    }
    // Serial Number
    if (systemInfo->flagsSerialNo != 0)
    {
        index = loadString(systemInfo->serialNo, systemInfoData->systemInfoBuf, index);
    }
    // Firmware
    if (systemInfo->flagsFirmware != 0)
    {
        index = loadString(systemInfo->firmware, systemInfoData->systemInfoBuf, index);
    }
    // Software
    if (systemInfo->flagsSoftware != 0)
    {
        index = loadString(systemInfo->software, systemInfoData->systemInfoBuf, index);
    }
    // Hardware
    if (systemInfo->flagsHardware != 0)
    {
        index = loadString(systemInfo->hardware, systemInfoData->systemInfoBuf, index);
    }
    // UDI Label
    if (systemInfo->udi_label != 0)
    {
        index = loadString(systemInfo->udi_label, systemInfoData->systemInfoBuf, index);
    }
    // UDI Device Identifier
    if (systemInfo->udi_device_identifier != 0)
    {
        index = loadString(systemInfo->udi_device_identifier, systemInfoData->systemInfoBuf, index);
    }
    // UDI Issuer OID
    if (systemInfo->udi_issuer_oid != 0)
    {
        index = loadString(systemInfo->udi_issuer_oid, systemInfoData->systemInfoBuf, index);
    }
    // UDI Authorization OID
    if (systemInfo->udi_authority_oid != 0)
    {
        index = loadString(systemInfo->udi_authority_oid, systemInfoData->systemInfoBuf, index);
    }
    #if (USES_SYSTEM_AVAS == 1)
        if (systemInfo->flagsAvas != 0)
        {
            int k;
            systemInfoData->systemInfoBuf[index++] = (unsigned char)systemInfo->currentAvaCount;
            NRF_LOG_DEBUG("System Info has %u AVAs\r\n", systemInfo->currentAvaCount);
            for (k = 0; k < systemInfo->currentAvaCount; k++)
            {
                index = fourByteEncode(systemInfoData->systemInfoBuf, index, systemInfo->avas[k]->attrId);
                systemInfoData->systemInfoBuf[index++] = (unsigned char)(systemInfo->avas[k]->length & 0xFF);
                systemInfoData->systemInfoBuf[index++] = (unsigned char)((systemInfo->avas[k]->length >> 8) & 0xFF);
                memcpy(&systemInfoData->systemInfoBuf[index], systemInfo->avas[k]->value, (size_t)(systemInfo->avas[k]->length));
                index = index + systemInfo->avas[k]->length;
            }
        }
    #endif
    *systemInfoDataPtr = systemInfoData;
    return true;
}

void cleanUpMetMsmt(s_MetMsmt** metMsmtPtr)
{
    int i;
    s_MetMsmt* metMsmt = *metMsmtPtr;
    #if (USES_MSMT_AVAS == 1)
        cleanUpAvas(metMsmt->avas, metMsmt->numberOfAvas);
    #endif
    metMsmt->avas = NULL;
    if (metMsmt->compoundNumeric != NULL)
    {
        for (i = 0; i < metMsmt->compoundNumeric->numberOfComponents; i++)
        {
            free(metMsmt->compoundNumeric->value[i]);
        }
        free(metMsmt->compoundNumeric->value);
        free(metMsmt->compoundNumeric);
    }
    if (metMsmt->rtsa != NULL)
    {
        free(metMsmt->rtsa);
    }
    if (metMsmt->simpleNumeric != NULL)
    {
        free(metMsmt->simpleNumeric);
    }
    if (metMsmt->codedEnum != NULL)
    {
        free(metMsmt->codedEnum);
    }
    if (metMsmt->bitsEnum != NULL)
    {
        free(metMsmt->bitsEnum);
    }
    free(metMsmt);
    metMsmt = NULL;
    *metMsmtPtr = NULL;
}

static void cleanUpHeader(s_MsmtHeader **headerPtr)
{
    s_MsmtHeader* header = *headerPtr;
    if (header != NULL)
    {
        #if (USES_HEADER_AVAS == 1)
            cleanUpAvas(header->avas, header->numberOfAvas);
            header->avas = NULL;
        #endif
        free(header);
        *headerPtr = NULL;
    }
}

void cleanUpMsmtGroup(s_MsmtGroup** msmtGroupPtr)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (msmtGroup != NULL)
    {
        int i;
        for (i = 0; i < msmtGroup->header->numberOfMsmts; i++)
        {
            if (msmtGroup->metMsmt[i] != NULL)
            {
                cleanUpMetMsmt(&msmtGroup->metMsmt[i]);
                msmtGroup->metMsmt[i] = NULL;
            }
        }
        if (msmtGroup->metMsmt != NULL)
        {
            free(msmtGroup->metMsmt);
            msmtGroup->metMsmt = NULL;
        }
        cleanUpHeader(&msmtGroup->header);
        free(msmtGroup);
        msmtGroup = NULL;
        *msmtGroupPtr = NULL;
    }
}

void cleanUpMsmtGroupData(s_MsmtGroupData** msmtGroupDataPtr)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (msmtGroupData != NULL)
    {
        if (msmtGroupData->sMetMsmtIndex != NULL)
        {
            int i;
            for (i = 0; i < msmtGroupData->currentMetMsmtCount; i++)
            {
                if (msmtGroupData->sMetMsmtIndex[i] != NULL)
                {
                    free(msmtGroupData->sMetMsmtIndex[i]);
                }
            }
            free(msmtGroupData->sMetMsmtIndex);
        }
        if (msmtGroupData->data != NULL)
        {
            free(msmtGroupData->data);
        }
        free(msmtGroupData);
        *msmtGroupDataPtr = NULL;
    }
}

// ======================= Measurements
bool createMsmtGroup(s_MsmtGroup **msmtGroupPtr, bool hasTimeStamp, unsigned char numberOfMetMsmts, unsigned char groupId)
{
    if (numberOfMetMsmts == 0)
    {
        return false;
    }
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (msmtGroup != NULL)
    {
        cleanUpMsmtGroup(msmtGroupPtr);
    }
    msmtGroup = (s_MsmtGroup *)calloc(1, sizeof(s_MsmtGroup));
    if (msmtGroup == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for the s_MsmtGroup.\r\n");
        return false;
    }
    msmtGroup->metMsmt = (s_MetMsmt **)calloc(1, numberOfMetMsmts * sizeof(s_MetMsmt *));
    if (msmtGroup->metMsmt == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for the %d Msmt group s_MetMsmt pointers.\r\n", numberOfMetMsmts);
        free (msmtGroup);
        return false;
    }
    msmtGroup->header = (s_MsmtHeader *)calloc(1, sizeof(s_MsmtHeader));
    if (msmtGroup->header == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for the s_MsmtHeader struct.\r\n");
        free (msmtGroup->metMsmt);
        free (msmtGroup);
        return false;
    }
    msmtGroup->header->groupId = groupId;
    msmtGroup->header->numberOfMsmts = numberOfMetMsmts;
    msmtGroup->header->flagsTimeStamp = hasTimeStamp ? HEADER_FLAGS_TIMESTAMP : 0;
    *msmtGroupPtr = msmtGroup;
    return true;
}

bool setHeaderOptions(s_MsmtGroup** msmtGroupPtr, bool areSettings, bool hasPersonId, unsigned short personId)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    msmtGroup->header->flagsPersonId = hasPersonId ? HEADER_FLAGS_PERSONID : 0;
    if (hasPersonId)
    {
        msmtGroup->header->personId = personId;
    }
    msmtGroup->header->flagsSetting = areSettings ? HEADER_FLAGS_SETTING : 0;
    return true;
}

bool setHeaderSupplementalTypes(s_MsmtGroup **msmtGroupPtr, unsigned short numberOfSupplementalTypes )
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    if (numberOfSupplementalTypes > 0)
    {
        msmtGroup->header->numberOfSuppTypes = numberOfSupplementalTypes;
        msmtGroup->header->flagsSuppTypes = HEADER_FLAGS_SUPP_TYPES;
    }
    return true;
}

bool setHeaderRefs(s_MsmtGroup **msmtGroupPtr, unsigned short numberOfRefs )
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    if (numberOfRefs > 0)
    {
        msmtGroup->header->numberOfRefs = numberOfRefs;
        msmtGroup->header->flagsRefs = HEADER_FLAGS_REFS;
    }
    return true;
}

bool setHeaderDuration(s_MsmtGroup **msmtGroupPtr)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;
    msmtGroup->header->flagsDuration = HEADER_FLAGS_DURATION;
    return true;
}

#if (USES_HEADER_AVAS == 1)
bool initializeHeaderAvas(s_MsmtGroup **msmtGroupPtr, unsigned short numberOfAvas)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    msmtGroup->header->numberOfAvas = numberOfAvas;
    if (numberOfAvas > 0)
    {
        if (!allocateAvas(&msmtGroup->header->avas, numberOfAvas))
        {
            msmtGroup->header->numberOfAvas = 0;
        }
    }
    return true;
}

bool addHeaderAva(s_MsmtGroup **msmtGroupPtr, s_Avas *avaIn)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    if (msmtGroup->header->numberOfAvas == 0)
    {
        NRF_LOG_DEBUG("Header AVAs not initialized or Ava initialization failed.\r\n");
        return false;
    }
    if (msmtGroup->header->currentAvaCount >= msmtGroup->header->numberOfAvas)
    {
        NRF_LOG_DEBUG("Number of Header Avas have been exceeded. Cannot add any more.\r\n");
        return false;
    }
    if (addAva(&msmtGroup->header->avas[msmtGroup->header->currentAvaCount], avaIn))
    {
        msmtGroup->header->currentAvaCount++;
        return true;
    }
    return false;
}
#endif

bool updateTimeStampEpoch(s_MsmtGroupData **msmtGroupDataPtr, unsigned long long epoch)
{
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.\r\n");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[2] & 0xFF + ((msmtGroupData->data[3] << 8) & 0xFF00);
    if ((flags & HEADER_FLAGS_TIMESTAMP) == HEADER_FLAGS_TIMESTAMP)
    {
        unsigned short i;
        for (i = 0; i < 6; i++)
        {
            msmtGroupData->data[i + MET_TIME_INDEX_EPOCH + 6] = (epoch & 0xFF);
            epoch = (epoch >> 8);
        }
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.\r\n");
        return false;
    }
    *msmtGroupDataPtr = msmtGroupData;
    return true;
}

bool updateTimeStampTimeSync(s_MsmtGroupData **msmtGroupDataPtr, unsigned short timeSync)
{
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.\r\n");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[2] & 0xFF + ((msmtGroupData->data[3] << 8) & 0xFF00);
    if ((flags & HEADER_FLAGS_TIMESTAMP) == HEADER_FLAGS_TIMESTAMP)
    {
        twoByteEncode(msmtGroupData->data, MET_TIME_INDEX_TIME_SYNC + 6, timeSync);
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.\r\n");
        return false;
    }
    return true;
}

bool updateTimeStampFlags(s_MsmtGroupData **msmtGroupDataPtr, unsigned short newflags)
{
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.\r\n");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[2] & 0xFF + ((msmtGroupData->data[3] << 8) & 0xFF00);
    if ((flags & HEADER_FLAGS_TIMESTAMP) == HEADER_FLAGS_TIMESTAMP)
    {
        msmtGroupData->data[MET_TIME_INDEX_FLAGS + 6] = newflags;
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.\r\n");
        return false;
    }
    return true;
}


bool updateTimeStampOffset(s_MsmtGroupData **msmtGroupDataPtr, short offsetShift)
{
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.\r\n");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[2] & 0xFF + ((msmtGroupData->data[3] << 8) & 0xFF00);
    if ((flags & HEADER_FLAGS_TIMESTAMP) == HEADER_FLAGS_TIMESTAMP)
    {
        msmtGroupData->data[MET_TIME_INDEX_OFFSET + 6] = (offsetShift & 0xFF);
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.\r\n");
        return false;
    }
    return true;
}

short addMetMsmtToGroup(s_MetMsmt *metMsmt, s_MsmtGroup **msmtGroupPtr)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return -1;
    if (!checkMetMsmt(metMsmt)) return -1;
    if (msmtGroup->header->currentMsmtCount >= msmtGroup->header->numberOfMsmts)
    {
        NRF_LOG_DEBUG("Number of MetMsmts %d exceeds numberOfMetMsmts %d.\r\n", (msmtGroup->header->currentMsmtCount + 1), msmtGroup->header->numberOfMsmts);
        return -1;
    }
    msmtGroup->metMsmt[msmtGroup->header->currentMsmtCount++] = metMsmt;
    *msmtGroupPtr = msmtGroup;
    return (msmtGroup->header->currentMsmtCount - 1);
}

#if (USES_NUMERIC == 1)
    bool createNumericMsmt(s_MetMsmt **metMsmtPtr, unsigned long type, bool isSfloat, unsigned short units)
    {
        s_MetMsmt* metMsmt = *metMsmtPtr;
        if (metMsmt != NULL)
        {
            cleanUpMetMsmt(metMsmtPtr);
        }
        metMsmt = (s_MetMsmt *)calloc(1, sizeof(s_MetMsmt));
        if (metMsmt == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Numeric Met Measurement.\r\n");
            return false;
        }
        metMsmt->simpleNumeric = (s_SimpNumeric *) calloc(1, sizeof(s_SimpNumeric));
        if (metMsmt->simpleNumeric == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Simple Numeric entry.\r\n");
            free (metMsmt);
            return false;
        }
        metMsmt->type = type;
        metMsmt->flagsMsmtType = MSMT_FLAGS_SIMPLE_NUMERIC;
        metMsmt->flagsSfloat = isSfloat ? MSMT_FLAGS_SFLOAT_VAL : 0;
        metMsmt->simpleNumeric->units = units;
        *metMsmtPtr = metMsmt;
        return true;
    }

    bool updateDataNumeric(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat* value, unsigned short msmt_id)
    {
        s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
        if (!checkMsmtGroupData(msmtGroupData)) return false;

        if (value == NULL)
        {
            NRF_LOG_DEBUG("Input value is NULL. Skipping\r\n");
            return false;
        }
        if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
        {
            NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
            return false;
        }
        s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
        if (sMetMsmtIndex->metricType != MSMT_FLAGS_SIMPLE_NUMERIC)
        {
            NRF_LOG_DEBUG("Measurement is not a numeric. Skipping\r\n");
            return false;
        }
        msmtGroupData->data[sMetMsmtIndex->id_index] = (msmt_id & 0xFF);
        msmtGroupData->data[sMetMsmtIndex->id_index + 1] = ((msmt_id >> 8) & 0xFF);
        if (sMetMsmtIndex->isSfloat)
        {
            unsigned short ieeeFloat;
            createIeeeSFloatFromMderFloat(value, &ieeeFloat);
            msmtGroupData->data[sMetMsmtIndex->value_index] = (ieeeFloat & 0xFF);
            msmtGroupData->data[sMetMsmtIndex->value_index + 1] = ((ieeeFloat >> 8) & 0xFF);
        }
        else
        {
            unsigned long ieeeFloat;
            createIeeeFloatFromMderFloat(value, &ieeeFloat);
            fourByteEncode(msmtGroupData->data, sMetMsmtIndex->value_index, ieeeFloat);
        }
        return true;
    }
#endif
#if (USES_COMPOUND == 1)
    static bool createCompoundMsmt(s_MetMsmt** metMsmtPtr, unsigned long type, bool isSfloat, unsigned short units, unsigned short numberOfComponents, s_Compound *compounds,
                                       bool isComplex)
    {
        int i;
        if (compounds == NULL || numberOfComponents == 0)
        {
            NRF_LOG_DEBUG("Compounds entry was NULL or numberOfComponents 0.\r\n");
            return false;
        }
        s_MetMsmt* metMsmt = *metMsmtPtr;
        if (metMsmt != NULL)
        {
            cleanUpMetMsmt(metMsmtPtr);
        }
        metMsmt = (s_MetMsmt *)calloc(1, sizeof(s_MetMsmt));
        if (metMsmt == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Numeric Met Measurement.\r\n");
            return false;
        }
        metMsmt->compoundNumeric = (s_CompoundNumeric *) calloc(1, sizeof(s_CompoundNumeric));
        if (metMsmt->compoundNumeric == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Compound Numeric Entry.\r\n");
            free (metMsmt);
            return false;
        }
        metMsmt->type = type;
        metMsmt->flagsMsmtType = isComplex ? MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC : MSMT_FLAGS_COMPOUND_NUMERIC;
        metMsmt->flagsSfloat = isSfloat ? MSMT_FLAGS_SFLOAT_VAL : 0;
        metMsmt->compoundNumeric->numberOfComponents = numberOfComponents;
        metMsmt->compoundNumeric->value = (s_Compound **)calloc(1, numberOfComponents * sizeof(s_Compound *));
        if (metMsmt->compoundNumeric->value == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Compound Numeric value pointers.\r\n");
            free (metMsmt->compoundNumeric);
            free (metMsmt);
            return false;
        }
        for (i = 0; i < numberOfComponents; i++)
        {
            metMsmt->compoundNumeric->value[i] = (s_Compound *)calloc(1, sizeof(s_Compound));
            if (metMsmt->compoundNumeric->value[i] == NULL)
            {
                NRF_LOG_DEBUG("Unable to allocate memory for Compound Numeric value %d.\r\n", i);
                while (i > 0)
                {
                    free (metMsmt->compoundNumeric->value[i - 1]);
                    i--;
                }
                free (metMsmt->compoundNumeric);
                free (metMsmt);
                return false;
            }
            metMsmt->compoundNumeric->value[i]->subType = compounds[i].subType;
            if (isComplex)
            {
                metMsmt->compoundNumeric->value[i]->subUnits = compounds[i].subUnits;
            }
        }
        if (!isComplex)
        {
            metMsmt->compoundNumeric->units = units;
        }
        *metMsmtPtr = metMsmt;
        return true;
    }

    bool updateDataCompound(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat* value, unsigned short msmt_id)
    {
        s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
        if (!checkMsmtGroupData(msmtGroupData)) return false;

        if (value == NULL)
        {
            NRF_LOG_DEBUG("Input value is NULL. Skipping\r\n");
            return false;
        }
        if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
        {
            NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
            return false;
        }


        s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
        bool isComplex = (sMetMsmtIndex->metricType == MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC);
        if (sMetMsmtIndex->metricType != MSMT_FLAGS_COMPOUND_NUMERIC && !isComplex)
        {
            NRF_LOG_DEBUG("Measurement is not a compound or complex compound. Skipping\r\n");
            return false;
        }
        unsigned short i;
        msmtGroupData->data[sMetMsmtIndex->id_index] = (msmt_id & 0xFF);
        msmtGroupData->data[sMetMsmtIndex->id_index + 1] = ((msmt_id >> 8) & 0xFF);
        unsigned short index = sMetMsmtIndex->value_index;
        bool optimized = (((msmtGroupData->data[2] & 0xFF) + ((msmtGroupData->data[2] << 8) & 0xFF) & HEADER_FLAGS_OPTIMIZED_FOLLOWS) == HEADER_FLAGS_OPTIMIZED_FOLLOWS);
        unsigned short increment = isComplex ? 6 : 4;
        for (i = 0; i < sMetMsmtIndex->numberOfValues; i++)
        {
            if (sMetMsmtIndex->isSfloat)
            {
                unsigned short ieeeFloat;
                createIeeeSFloatFromMderFloat(&value[i], &ieeeFloat);
                msmtGroupData->data[index++] = (ieeeFloat & 0xFF);
                msmtGroupData->data[index++] = ((ieeeFloat >> 8) & 0xFF);
            }
            else
            {
                unsigned long ieeeFloat;
                createIeeeFloatFromMderFloat(&value[i], &ieeeFloat);
                index = fourByteEncode(msmtGroupData->data, index, ieeeFloat);
            }
            if (!optimized)
            {
                index = index + increment;  // increment takes into account the complex part
            }
        }
        return true;
    }

    bool createCompoundNumericMsmt(s_MetMsmt** metMsmtPtr, unsigned long type, bool isSfloat, unsigned short units, unsigned short numberOfComponents, s_Compound *compounds)
    {
        return createCompoundMsmt(metMsmtPtr, type, isSfloat, units, numberOfComponents, compounds, false);
    }

    bool createComplexCompoundNumericMsmt(s_MetMsmt** metMsmtPtr, unsigned long type, bool isSfloat, unsigned short numberOfComponents, s_Compound *compounds)
    {
        return createCompoundMsmt(metMsmtPtr, type, isSfloat, 0, numberOfComponents, compounds, true);
    }
#endif
#if (USES_CODED == 1)
bool createCodedMsmt(s_MetMsmt** metMsmtPtr, unsigned long type)
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (metMsmt != NULL)
    {
        cleanUpMetMsmt(metMsmtPtr);
    }
    metMsmt = (s_MetMsmt *)calloc(1, sizeof(s_MetMsmt));
    if (metMsmt == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for Coded Met Measurement.\r\n");
        return false;
    }
    metMsmt->codedEnum = (s_CodedEnum *) calloc(1, sizeof(s_CodedEnum));
    if (metMsmt->codedEnum == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for Coded Enum Entry.\r\n");
        free (metMsmt);
        return false;
    }
    metMsmt->type = type;
    metMsmt->flagsMsmtType = MSMT_FLAGS_CODED_ENUM;
    *metMsmtPtr = metMsmt;
    return true;
}

bool updateDataCoded(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long code, unsigned short msmt_id)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
        return false;
    }

    s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
    if (sMetMsmtIndex->metricType != MSMT_FLAGS_CODED_ENUM)
    {
        NRF_LOG_DEBUG("Measurement is not a coded enum. Skipping\r\n");
        return false;
    }
    msmtGroupData->data[sMetMsmtIndex->id_index] = (msmt_id & 0xFF);
    msmtGroupData->data[sMetMsmtIndex->id_index + 1] = ((msmt_id >> 8) & 0xFF);
    fourByteEncode(msmtGroupData->data, sMetMsmtIndex->value_index, code);
    return true;
}
#endif

#if (USES_BITS == 1)
bool createBitEnumMsmt(s_MetMsmt** metMsmtPtr, unsigned short byteCount, unsigned long type, unsigned long state, unsigned long support)
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (metMsmt != NULL)
    {
        cleanUpMetMsmt(metMsmtPtr);
    }
    metMsmt = (s_MetMsmt*)calloc(1, sizeof(s_MetMsmt));
    if (metMsmt == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for BITS Met Measurement.\r\n");
        return false;
    }
    metMsmt->type = type;
    metMsmt->bitsEnum = (s_BitsEnum*)calloc(1, sizeof(s_BitsEnum));
    if (metMsmt->bitsEnum == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for BITs Enum Entry.\r\n");
        free(metMsmt);
        return false;
    }
    metMsmt->bitsEnum->byteCount = byteCount;
    metMsmt->bitsEnum->stateEvent = state;
    metMsmt->bitsEnum->supportEvent = support;
    metMsmt->flagsMsmtType = MSMT_FLAGS_BITS_ENUM;
    *metMsmtPtr = metMsmt;
    return true;
}

bool updateDataBits(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long bits, unsigned short msmt_id)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
        return false;
    }

    s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
    if (sMetMsmtIndex->metricType != MSMT_FLAGS_BITS_ENUM)
    {
        NRF_LOG_DEBUG("Measurement is not a BITS enum. Skipping\r\n");
        return false;
    }
    msmtGroupData->data[sMetMsmtIndex->id_index] = (msmt_id & 0xFF);
    msmtGroupData->data[sMetMsmtIndex->id_index + 1] = ((msmt_id >> 8) & 0xFF);
    unsigned short byteCount = msmtGroupData->data[sMetMsmtIndex->value_index]; // number of bytes in BITs msmt
    int i;
    for (i = 0; i < byteCount; i++)
    {
        msmtGroupData->data[sMetMsmtIndex->value_index + i + 1] = (bits & 0xFF);
        bits = (bits >> 8);
    }
    return true;
}
#endif
#if (USES_RTSA == 1)
bool createRtsaMsmt(s_MetMsmt** metMsmtPtr, unsigned long type, unsigned short units, s_MderFloat *period, s_MderFloat *scaleFactor, s_MderFloat *offset, 
    unsigned short numberOfSamples, unsigned char sampleSize)
{
    if (period == NULL || scaleFactor == NULL || offset == NULL)
    {
        NRF_LOG_DEBUG("One or more of the s_MderFloat Input parameters are NULL.\r\n");
        return false;
    }
    if (numberOfSamples == 0 || sampleSize == 0)
    {
        NRF_LOG_DEBUG("Either the sample size or the number of samples is 0.\r\n");
        return false;
    }
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (metMsmt != NULL)
    {
        cleanUpMetMsmt(metMsmtPtr);
    }
    metMsmt = (s_MetMsmt*)calloc(1, sizeof(s_MetMsmt));
    if (metMsmt == NULL)
    {
        NRF_LOG_ERROR("Unable to allocate memory for RTSA Met Measurement.\r\n");
        return false;
    }
    metMsmt->type = type;
    metMsmt->rtsa = (s_Rtsa*)calloc(1, sizeof(s_Rtsa));
    if (metMsmt->rtsa == NULL)
    {
        NRF_LOG_ERROR("Unable to allocate memory for RTSA Entry.\r\n");
        free(metMsmt);
        return false;
    }
    metMsmt->rtsa->offset = *copyMderFloat(&metMsmt->rtsa->offset, offset);
    metMsmt->rtsa->period = *copyMderFloat(&metMsmt->rtsa->period, period);
    metMsmt->rtsa->scaleFactor = *copyMderFloat(&metMsmt->rtsa->scaleFactor, scaleFactor);
    metMsmt->rtsa->units = units;
    metMsmt->rtsa->numberOfSamples = numberOfSamples;
    metMsmt->rtsa->sampleSize = sampleSize;
    metMsmt->flagsMsmtType = MSMT_FLAGS_RTSA;
    *metMsmtPtr = metMsmt;
    return true;
}

bool updateDataRtsa(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned char* samples, unsigned short dataLength, unsigned short msmt_id)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
        return false;
    }

    s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
    if (sMetMsmtIndex->metricType != MSMT_FLAGS_RTSA)
    {
        NRF_LOG_DEBUG("Measurement is not an RTSA. Skipping\r\n");
        return false;
    }
    msmtGroupData->data[sMetMsmtIndex->id_index] = (msmt_id & 0xFF);
    msmtGroupData->data[sMetMsmtIndex->id_index + 1] = ((msmt_id >> 8) & 0xFF);
    memcpy(&msmtGroupData->data[sMetMsmtIndex->value_index], samples, dataLength);
    return true;
}
#endif
bool setMetMsmtSupplementalTypes(s_MetMsmt **metMsmtPtr, unsigned short numberOfSupplementalTypes )
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (!checkMetMsmt(metMsmt)) return false;

    if (numberOfSupplementalTypes > 0)
    {
        metMsmt->numberOfSuppTypes = numberOfSupplementalTypes;
        metMsmt->flagsSuppTypes = MSMT_FLAGS_SUPP_TYPES;
        return true;
    }
    NRF_LOG_DEBUG("Number of supplemental types was zero.\r\n");
    return false;
}

bool setMetMsmtRefs(s_MetMsmt **metMsmtPtr, unsigned short numberOfRefs )
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (!checkMetMsmt(metMsmt)) return false;

    if (numberOfRefs > 0)
    {
        metMsmt->numberOfRefs = numberOfRefs;
        metMsmt->flagsRefs = MSMT_FLAGS_REFS;
        return true;
    }
    NRF_LOG_DEBUG("Number of references was zero.\r\n");
    return false;
}

bool updateDataHeaderSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, unsigned long suppType, unsigned suppType_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (suppType_index >= msmtGroupData->numberOfSuppTypes)
    {
        NRF_LOG_DEBUG("suppTypes_index exceeds number of header supplemental types allocated.\r\n");
        return false;
    }
    fourByteEncode(msmtGroupData->data, msmtGroupData->suppTypes_index + suppType_index * 4, suppType);
    return true;
}

bool updateDataMetMsmtSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long suppType, unsigned suppType_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
    if (suppType_index >= sMetMsmtIndex->numberOfSuppTypes)
    {
        NRF_LOG_DEBUG("suppType_index exceeds number of supplemental types allocated.\r\n");
        return false;
    }
    fourByteEncode(msmtGroupData->data, sMetMsmtIndex->suppTypes_index + suppType_index * 4, suppType);
    return true;
}

bool updateDataHeaderRefs(s_MsmtGroupData** msmtGroupDataPtr, unsigned short ref, unsigned ref_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (ref_index >= msmtGroupData->numberRefs)
    {
        NRF_LOG_DEBUG("ref_index exceeds number of header src handle refs allocated.\r\n");
        return false;
    }
    msmtGroupData->data[msmtGroupData->ref_index + (ref_index << 1)] = (ref & 0xFF);
    msmtGroupData->data[msmtGroupData->ref_index + (ref_index << 1) + 1] = ((ref >> 8) & 0xFF);
    return true;
}

bool updateDataHeaderDuration(s_MsmtGroupData** msmtGroupDataPtr, s_MderFloat *duration)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (duration == NULL)
    {
        NRF_LOG_DEBUG("Duration is NULL. Skipping.\r\n");
        return false;
    }
    unsigned long mder;
    createIeeeFloatFromMderFloat(duration, &mder);
    fourByteEncode(msmtGroupData->data, msmtGroupData->duration_index, mder);
    return true;
}

bool updateDataMetMsmtRefs(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned short ref, unsigned ref_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
        return false;
    }

    s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
    if (ref_index >= sMetMsmtIndex->numberRefs)
    {
        NRF_LOG_DEBUG("ref_index exceeds number of src handle refs allocated.\r\n");
        return false;
    }
    msmtGroupData->data[sMetMsmtIndex->ref_index + (ref_index << 1)] = (ref & 0xFF);
    msmtGroupData->data[sMetMsmtIndex->ref_index + (ref_index << 1) + 1] = ((ref >> 8) & 0xFF);
    return true;
}

bool updateDataMetMsmtDuration(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat *duration)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (duration == NULL)
    {
        NRF_LOG_DEBUG("Duration is NULL. Skipping.\r\n");
        return false;
    }
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentMetMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping\r\n", msmtIndex);
        return false;
    }

    s_MetMsmtIndex* sMetMsmtIndex = msmtGroupData->sMetMsmtIndex[msmtIndex];
    if (sMetMsmtIndex->duration_index == 0)
    {
        NRF_LOG_DEBUG("Duration is not configured. Skipping.\r\n");
        return false;
    }
    unsigned long mder;
    createIeeeFloatFromMderFloat(duration, &mder);
    fourByteEncode(msmtGroupData->data, sMetMsmtIndex->duration_index, mder);
    return true;
}

short getNumberOfMeasurementsDropped(s_MsmtGroupData** msmtGroupDataPtr)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return -1;

    return msmtGroupData->currentMetMsmtCount - (unsigned short)msmtGroupData->data[msmtGroupData->no_of_msmts_index];
}

bool updateDataDropLastMsmt(s_MsmtGroupData** msmtGroupDataPtr)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    unsigned short no_of_msmts = (unsigned short)msmtGroupData->data[msmtGroupData->no_of_msmts_index];
    if (no_of_msmts > 1)
    {
        no_of_msmts = no_of_msmts - 1;
        msmtGroupData->data[msmtGroupData->no_of_msmts_index] = (unsigned char)(no_of_msmts & 0xFF);
        msmtGroupData->dataLength = msmtGroupData->dataLength - msmtGroupData->sMetMsmtIndex[no_of_msmts]->msmt_length;
        twoByteEncode(msmtGroupData->data, 4, (msmtGroupData->dataLength - 6));
        *msmtGroupDataPtr = msmtGroupData;
        return true;
    }
    return false;
}

bool updateDataRestoreLastMsmt(s_MsmtGroupData** msmtGroupDataPtr)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    unsigned short no_of_msmts = (unsigned short)msmtGroupData->data[msmtGroupData->no_of_msmts_index];
    if (no_of_msmts < msmtGroupData->currentMetMsmtCount)
    {
        msmtGroupData->dataLength = msmtGroupData->dataLength + msmtGroupData->sMetMsmtIndex[no_of_msmts]->msmt_length;
        no_of_msmts = no_of_msmts + 1;
        msmtGroupData->data[msmtGroupData->no_of_msmts_index] = no_of_msmts;
        twoByteEncode(msmtGroupData->data, 4, (msmtGroupData->dataLength - 6));
        *msmtGroupDataPtr = msmtGroupData;
        return true;
    }
    return false;
}

bool setMetMsmtDuration(s_MetMsmt **metMsmtPtr)
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (!checkMetMsmt(metMsmt)) return false;
    metMsmt->flagsDuration = MSMT_FLAGS_DURATION;
    return true;
}

#if (USES_MSMT_AVAS == 1)
bool initializeMetMsmtAvas(s_MetMsmt **metMsmtPtr, unsigned short numberOfAvas)
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (!checkMetMsmt(metMsmt)) return false;

    metMsmt->numberOfAvas = numberOfAvas;
    if (numberOfAvas > 0)
    {
        if (!allocateAvas(&metMsmt->avas, numberOfAvas))
        {
            metMsmt->numberOfAvas = 0;
        }
    }
    return true;
}

bool addMetMsmtAva(s_MetMsmt **metMsmtPtr, s_Avas *avaIn)
{
    s_MetMsmt* metMsmt = *metMsmtPtr;
    if (!checkMetMsmt(metMsmt)) return false;

    if (metMsmt->numberOfAvas == 0)
    {
        NRF_LOG_DEBUG("MetMsmt AVAs not initialized or Ava initialization failed.\r\n");
        return false;
    }
    if (metMsmt->currentAvaCount >= metMsmt->numberOfAvas)
    {
        NRF_LOG_DEBUG("Number of Met Msmt Avas have been exceeded. Cannot add any more.\r\n");
        return false;
    }
    if (addAva(&metMsmt->avas[metMsmt->currentAvaCount], avaIn))
    {
        metMsmt->currentAvaCount++;
        metMsmt->flagsAvas = MSMT_FLAGS_AVAS;
        return true;
    }
    return false;
}
#endif

// ===================== Measurement Data Array
static int encodeAlwaysMetBase(int index, s_MetMsmt* metMsmt, unsigned char* msmtBuf, s_MetMsmtIndex **sMetMsmtIndex)
{
    // type
    index = fourByteEncode(msmtBuf, index, metMsmt->type);
    // leave space for length
    index = index + 2;
    // flags
    int flags = (metMsmt->flagsMsmtType
        | metMsmt->flagsAvas
        | metMsmt->flagsSfloat
        | metMsmt->flagsDuration
        | metMsmt->flagsRefs
        | metMsmt->flagsSuppTypes);
    index = twoByteEncode(msmtBuf, index, flags);
    // handle (id)
    (*sMetMsmtIndex)->id_index = index;  // Position of the msmt id
    index = twoByteEncode(msmtBuf, index, metMsmt->id);
    return index;
}

static int encodeOptionals(int index, s_MetMsmt * metMsmt, unsigned char *msmtBuf, unsigned short *msmtLengths, s_MetMsmtIndex** sMetMsmtIndex)
{
    int k;
    // supplemental types
    if (metMsmt->numberOfSuppTypes > 0)
    {
        msmtBuf[index++] = (unsigned char)(metMsmt->numberOfSuppTypes & 0xFF);
        (*sMetMsmtIndex)->suppTypes_index = index;
        (*sMetMsmtIndex)->numberOfSuppTypes = metMsmt->numberOfSuppTypes;
        index = index + metMsmt->numberOfSuppTypes * 4; // reserve space for supplemental types via update method
        *msmtLengths = *msmtLengths + 1 + 4 * metMsmt->numberOfSuppTypes;
    }
    // src-handle ref
    if (metMsmt->flagsRefs != 0)
    {
        msmtBuf[index++] = (unsigned char)(metMsmt->numberOfRefs & 0xFF);
        (*sMetMsmtIndex)->ref_index = index;
        (*sMetMsmtIndex)->numberRefs = metMsmt->numberOfRefs;
        index = index + metMsmt->numberOfRefs * 2; // reserve space for src handle refs via update method
        *msmtLengths = *msmtLengths + 1 + 2 * metMsmt->numberOfRefs;
    }
    // duration
    if (metMsmt->flagsDuration != 0)
    {
        (*sMetMsmtIndex)->duration_index = index;
        index = index + 4;      // reserve space
        *msmtLengths = *msmtLengths + 4;
    }
    #if (USES_MSMT_AVAS == 1)
    if (metMsmt->flagsAvas != 0)
    {
        msmtBuf[index++] = (unsigned char)metMsmt->currentAvaCount;
        *msmtLengths = *msmtLengths + 1;
        NRF_LOG_DEBUG("Msmt has %u AVAs\n\r\n", metMsmt->currentAvaCount);
        for (k = 0; k < metMsmt->currentAvaCount; k++)
        {
            unsigned short len = 6 + metMsmt->avas[k]->length;
            index = fourByteEncode(msmtBuf, index, metMsmt->avas[k]->attrId);
            index = twoByteEncode(msmtBuf, index, metMsmt->avas[k]->length);
            memcpy(&msmtBuf[index], metMsmt->avas[k]->value, (size_t)(metMsmt->avas[k]->length));
            index = index + metMsmt->avas[k]->length;
            *msmtLengths = *msmtLengths + len;
        }
    }
    #endif
    return index;
}

static unsigned short sizeOfMetBase(s_MetMsmt* metMsmt)
{
    unsigned short groupLength = ((metMsmt->flagsDuration != 0) ? 4 : 0) +
                                   ((metMsmt->flagsSuppTypes != 0) ? (1 + metMsmt->numberOfSuppTypes * 4) : 0) +
                                   ((metMsmt->flagsRefs != 0) ? (1 + metMsmt->numberOfRefs * 2) : 0);
    #if (USES_MSMT_AVAS == 1)
    if (metMsmt->flagsAvas != 0)
    {
        unsigned short j;
        groupLength++; // for the number of AVAs entry
        for (j = 0; j < metMsmt->currentAvaCount; j++)
        {
            groupLength = groupLength + metMsmt->avas[j]->length + 6;  // 4-byte id and 2-byte length + 1 byte # of AVAs
        }
    }
    #endif
    return groupLength + 10; // type, length, flags, id
}

static unsigned short computeLengthOfMsmtGroup(s_MsmtGroup *msmtGroup, bool optimized)
{
    unsigned short j;
    unsigned short groupLength;
    if (optimized)
    {
        groupLength = ((msmtGroup->header->flagsTimeStamp != 0) ? GROUP_HEADER_LENGTH + MET_TIME_LENGTH + 1 
                : GROUP_HEADER_LENGTH + 1);  // command(2), flags(2), length(2), [timestamp(10)], group id(1)
    }
    else
    {
        groupLength = ((msmtGroup->header->flagsTimeStamp != 0) ?  GROUP_HEADER_LENGTH + MET_TIME_LENGTH + 2 
              : GROUP_HEADER_LENGTH + 2) + // command(2), flags(2), length(2), [timestamp(10)], group id(1), # of msmts(1)
            ((msmtGroup->header->flagsPersonId != 0) ? 2 : 0) +
            ((msmtGroup->header->flagsDuration != 0) ? 4 : 0) +
            ((msmtGroup->header->flagsSuppTypes != 0) ? (1 + msmtGroup->header->numberOfSuppTypes * 4) : 0) +
            ((msmtGroup->header->flagsRefs != 0) ? (1 + msmtGroup->header->numberOfRefs * 2) : 0);

        if (msmtGroup->header->flagsAvas != 0)
        {
            groupLength++; // for the number of AVAs entry
            for (j = 0; j < msmtGroup->header->currentAvaCount; j++)
            {
                groupLength = groupLength + msmtGroup->header->avas[j]->length + 6 + 1;
            }
        }
    }
    // Find the length of all MetMeasurements
    for (j = 0; j < msmtGroup->header->currentMsmtCount; j++)
    {
        s_MetMsmt* met = msmtGroup->metMsmt[j];
        groupLength = groupLength + (optimized ? 2 : sizeOfMetBase(msmtGroup->metMsmt[j])); // Only the msmt id for the continuous reduced case
        if (met->simpleNumeric != NULL)
        {
            groupLength = groupLength + (optimized ? 0 : 2) + // units for non-optimized case
                (((met->flagsSfloat & MSMT_FLAGS_SFLOAT_VAL) == MSMT_FLAGS_SFLOAT_VAL) ? 2 : 4);
        }
        else if (met->compoundNumeric != NULL)
        {
            if (optimized)
            {
                groupLength = groupLength + met->compoundNumeric->numberOfComponents * ((met->flagsSfloat) ? 2 : 4); // sub values
            }
            else
            {
                if (met->flagsMsmtType == MSMT_FLAGS_COMPOUND_NUMERIC)
                {
                    groupLength = groupLength + 3 + // units, number of components
                        met->compoundNumeric->numberOfComponents * ((met->flagsSfloat) ? 6 : 8); // sub types + sub values
                }
                else  // Complex compound
                {
                    groupLength = groupLength + 1 + // number of components
                            met->compoundNumeric->numberOfComponents * ((met->flagsSfloat) ? 8 : 10); // sub types + sub values
                }
            }
        }
        else if (met->codedEnum != NULL)
        {
            groupLength = groupLength + 4;  // coded enum value
        }
        else if (met->bitsEnum != NULL)
        {
            groupLength = groupLength + (optimized ? met->bitsEnum->byteCount : 3 * met->bitsEnum->byteCount + 1); // bits enum value set
        }
        else if (met->rtsa != NULL)
        {
            groupLength = groupLength + (optimized ? 2 : 17) + (met->rtsa->sampleSize * met->rtsa->numberOfSamples);
        }
    }
    return groupLength;
}

bool createMsmtGroupDataArray(s_MsmtGroupData** msmtGroupDataPtr, s_MsmtGroup *msmtGroup, s_MetTime *sMetTime, unsigned short packetType)
{
    if (msmtGroup == NULL)
    {
        NRF_LOG_ERROR("MsmtGroup not initialized.\r\n");
        return false;
    }
    bool optimized = (packetType == PACKET_TYPE_OPTIMIZED_FOLLOWS);
    unsigned short groupLength = computeLengthOfMsmtGroup(msmtGroup, optimized);

    unsigned char *msmtBuf = (unsigned char*)calloc(1, groupLength);
    if (msmtBuf == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for group measurement buffer\r\n");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (msmtGroupData != NULL)
    {
        cleanUpMsmtGroupData(msmtGroupDataPtr);
    }
    msmtGroupData = (s_MsmtGroupData *)calloc(1, sizeof(s_MsmtGroupData));
    if (msmtGroupData == NULL)
    {
        NRF_LOG_ERROR("Could not allocate memory for group measurement data\r\n");
        free(msmtBuf);
        return false;
    }
    if (msmtGroup->header->currentMsmtCount > 0)
    {
        msmtGroupData->sMetMsmtIndex = (s_MetMsmtIndex**)calloc(1, msmtGroup->header->currentMsmtCount * sizeof(s_MetMsmtIndex*));
        if (msmtGroupData->sMetMsmtIndex == NULL)
        {
            NRF_LOG_ERROR("Could not allocate memory for group measurement index pointers\r\n");
            free(msmtBuf);
            free(msmtGroupData);
            return false;
        }
    }
    msmtGroupData->dataLength = (unsigned short)groupLength;
    msmtGroupData->currentMetMsmtCount = msmtGroup->header->currentMsmtCount;
    
    unsigned short index = 2;  // The first two bytes are reserved for the command
    unsigned short j;

    // Encode the first measurement group
    // header - command

    // header - flags
    unsigned short flags = (unsigned char)(msmtGroup->header->flagsTimeStamp | msmtGroup->header->flagsDuration |
        msmtGroup->header->flagsSuppTypes | msmtGroup->header->flagsRefs |
        msmtGroup->header->flagsPersonId | msmtGroup->header->flagsSetting |
        msmtGroup->header->flagsAvas);
    if (optimized)
    {
        flags = flags | HEADER_FLAGS_OPTIMIZED_FOLLOWS;
    }
    else if (packetType == PACKET_TYPE_OPTIMIZED_FIRST)
    {
        flags = flags | HEADER_FLAGS_OPTIMIZED_FIRST;
    }
    index = twoByteEncode(msmtBuf, index, flags);

    // header - length of PDU is given by the group length - 6
    index = twoByteEncode(msmtBuf, index, (groupLength - 6));

    // header- timestamp if it exists
    if (msmtGroup->header->flagsTimeStamp != 0 && sMetTime != NULL)
    {
        msmtBuf[index + MET_TIME_INDEX_FLAGS] = (sMetTime->clockResolution | sMetTime->clockType);
        msmtBuf[index + MET_TIME_INDEX_OFFSET] = (sMetTime->offsetShift & 0xFF);
        msmtBuf[index + MET_TIME_INDEX_TIME_SYNC] = (sMetTime->timeSync & 0xFF);
        msmtBuf[index + MET_TIME_INDEX_TIME_SYNC + 1] = ((sMetTime->timeSync >> 8) & 0xFF);
        index = index + MET_TIME_LENGTH;  // Epoch, flags, offset, time sync
    }
    if (!optimized)
    {
        if (msmtGroup->header->flagsSuppTypes != 0)
        {
            msmtGroupData->numberOfSuppTypes = msmtGroup->header->numberOfSuppTypes;
            msmtBuf[index++] = (msmtGroup->header->numberOfSuppTypes & 0xFF);
            msmtGroupData->suppTypes_index = index;
            index = index + msmtGroup->header->numberOfSuppTypes * 4;
        }
        if (msmtGroup->header->flagsRefs != 0)
        {
            msmtGroupData->numberRefs = msmtGroup->header->numberOfRefs;
            msmtBuf[index++] = (msmtGroup->header->numberOfRefs & 0xFF);
            msmtGroupData->ref_index = index;
            index = index + msmtGroup->header->numberOfRefs * 2; // reserve space for refs via update method
        }
        if (msmtGroup->header->flagsDuration != 0)
        {
            index = index + 4;
        }
        if (msmtGroup->header->flagsPersonId != 0)
        {
            index = twoByteEncode(msmtBuf, index, msmtGroup->header->personId);
        }
        #if (USES_HEADER_AVAS == 1)
        if (msmtGroup->header->flagsAvas != 0)
        {
            int i;
            msmtBuf[index++] = (unsigned char)(unsigned char)msmtGroup->header->currentAvaCount;
            for (i = 0; i < msmtGroup->header->currentAvaCount; i++)
            {
                memcpy(&msmtBuf[index], msmtGroup->header->avas[i], (size_t)(8L + msmtGroup->header->avas[i]->length));
                index = index + 8 + msmtGroup->header->avas[i]->length; // Using a 4-byte id field in AVA
            }
        }
        #endif
    }
    // group id
    msmtBuf[index++] = msmtGroup->header->groupId;

    // number of measurements
    if (!optimized)
    {
        msmtGroupData->no_of_msmts_index = index;
        msmtBuf[index++] = (unsigned char)(msmtGroup->header->currentMsmtCount);
    }

    // ================================ Loop: over # of met msmts
    for (j = 0; j < msmtGroupData->currentMetMsmtCount; j++)
    {
        s_MetMsmtIndex *sMetMsmtIndex = (s_MetMsmtIndex *)calloc(1, sizeof(s_MetMsmtIndex));
        if (sMetMsmtIndex == NULL)
        {
            NRF_LOG_ERROR("Could not allocate memory for group measurement index %d\r\n", j);
            free(msmtBuf);
            cleanUpMsmtGroupData(&msmtGroupData);
            return false;
        }
        unsigned short metLength = optimized ? 2 : MSMT_HEADER_LENGTH; // type, length, flags, and id
        s_MetMsmt* met = msmtGroup->metMsmt[j];
        // Need to get it now as we don't know the length yet.
        int lengthIndex = index + (optimized ? 0 : 4);
        if (optimized)
        {
            sMetMsmtIndex->id_index = index;
            index = twoByteEncode(msmtBuf, index, met->id);
        }
        else
        {
            index = encodeAlwaysMetBase(index, met, msmtBuf, &sMetMsmtIndex);
        }
        
        //================================= Simple Numeric
        if (met->simpleNumeric != NULL)
        {
            #if (USES_NUMERIC == 1)
            sMetMsmtIndex->metricType = MSMT_FLAGS_SIMPLE_NUMERIC;
            if (!optimized)
            {
                metLength = metLength + 2;
                index = twoByteEncode(msmtBuf, index, met->simpleNumeric->units);
            }
            // value 
            sMetMsmtIndex->value_index = index;
            sMetMsmtIndex->numberOfValues = 1;
            sMetMsmtIndex->isSfloat = (met->flagsSfloat != 0);
            if (met->flagsSfloat == 0)
            {
                index = index + 4; // Reserve spot
                metLength = metLength + 4;
            }
            else
            {
                index = index + 2;
                metLength = metLength + 2;
            }
            #endif
        }
        //================================= Compound Numeric
        else if (met->compoundNumeric != NULL)
        {
            #if (USES_COMPOUND == 1)
            bool isComplex = (met->flagsMsmtType == MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC);
            sMetMsmtIndex->metricType = isComplex ?  MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC : MSMT_FLAGS_COMPOUND_NUMERIC;
            metLength = metLength + (optimized ? 0 : 3);  // units + # of components
            if (!optimized)
            {
                if (!isComplex)
                {
                    // units
                    index = twoByteEncode(msmtBuf, index, met->compoundNumeric->units);
                }
                // number of components 
                msmtBuf[index++] = (unsigned char)((met->compoundNumeric->numberOfComponents) & 0xFF);
            }
            sMetMsmtIndex->value_index = optimized ? index : index + 4; // The first value in the compound starts after the 4-byte nomenclature code
            sMetMsmtIndex->numberOfValues = met->compoundNumeric->numberOfComponents;
            int k;
            sMetMsmtIndex->isSfloat = (met->flagsSfloat != 0);
            for (k = 0; k < met->compoundNumeric->numberOfComponents; k++)
            {
                if (!optimized)
                {
                    // Encode sub type
                    index = fourByteEncode(msmtBuf, index, met->compoundNumeric->value[k]->subType);
                    metLength = metLength + 4;
                }
                // now sub value
                if (met->flagsSfloat == 0)  // FLOAT case
                {
                    index = index + 4; // Reserve spot
                    metLength = metLength + 4;
                }
                else    // SFLOAT case
                {
                    index = index + 2;
                    metLength = metLength + 2;
                }
                if (isComplex)
                {
                    // units
                    index = twoByteEncode(msmtBuf, index, met->compoundNumeric->value[k]->subUnits);
                    metLength = metLength + 2;
                }
            }
            #endif
        }

        //================================= Coded Enum
        else if (met->codedEnum != NULL)
        {
            #if (USES_CODED == 1)
            sMetMsmtIndex->metricType = MSMT_FLAGS_CODED_ENUM;
            metLength = metLength + 4;
            // value
            sMetMsmtIndex->value_index = index;
            sMetMsmtIndex->numberOfValues = 1;
            
            index = index + 4; // reserve four bytes for value
            #endif
        }

        //================================= Bits Enum
        else if (met->bitsEnum != NULL)
        {
            #if (USES_BITS == 1)
            sMetMsmtIndex->metricType = MSMT_FLAGS_BITS_ENUM;
            metLength = metLength + 1 + 3 * met->bitsEnum->byteCount;

            // value index (# of bytes, bits, state or event, support)
            sMetMsmtIndex->value_index = index;
            sMetMsmtIndex->numberOfValues = 1;
            msmtBuf[index++] = met->bitsEnum->byteCount; // Number of bytes in BITs value
            // bits
            index = index + met->bitsEnum->byteCount; // leave space for the BITs value
            if (!optimized)
            {
                unsigned long stateEvent = met->bitsEnum->stateEvent;
                unsigned long supportEvent = met->bitsEnum->supportEvent;
                unsigned short count;
                for (count = 0; count < met->bitsEnum->byteCount; count++)
                {
                    msmtBuf[index + count] = (stateEvent & 0xFF);
                    msmtBuf[index + count  + met->bitsEnum->byteCount] = (supportEvent & 0xFF);
                    stateEvent = (stateEvent >> 8);
                    supportEvent = (supportEvent >> 8);
                }
                index = index + 2 * met->bitsEnum->byteCount;
            }
            #endif
        }

        //================================= RTSA
        else if (met->rtsa != NULL)
        {
            #if (USES_RTSA == 1)
            sMetMsmtIndex->metricType = MSMT_FLAGS_RTSA;
            unsigned short rtsaSampleLength = met->rtsa->sampleSize * met->rtsa->numberOfSamples;
            // units, period, scaleFactor, offset, sampleSize, numberOfSamples, data
            if (!optimized)
            {
                // units
                index = twoByteEncode(msmtBuf, index, met->rtsa->units);
                // period
                unsigned long mder;
                createIeeeFloatFromMderFloat(&met->rtsa->period, &mder);
                index = fourByteEncode(msmtBuf, index, mder);
                // scaleFactor
                createIeeeFloatFromMderFloat(&met->rtsa->scaleFactor, &mder);
                index = fourByteEncode(msmtBuf, index, mder);
                // offset
                createIeeeFloatFromMderFloat(&met->rtsa->offset, &mder);
                index = fourByteEncode(msmtBuf, index, mder);
                // size
                msmtBuf[index++] = (unsigned char)met->rtsa->sampleSize;
            }
            // numberOfSamples
            index = twoByteEncode(msmtBuf, index, met->rtsa->numberOfSamples);
            // samples
            sMetMsmtIndex->value_index = index;
            sMetMsmtIndex->numberOfValues = 1;
            index = index + rtsaSampleLength;
            metLength = metLength + 17 + rtsaSampleLength;
            #endif
        }
        sMetMsmtIndex->msmt_length = metLength;
        msmtGroupData->sMetMsmtIndex[j] = sMetMsmtIndex;
        if (!optimized)
        {
            index = encodeOptionals(index, met, msmtBuf, &metLength, &sMetMsmtIndex);
            twoByteEncode(msmtBuf, lengthIndex, (metLength - GROUP_HEADER_LENGTH));  // Don't need the updated lengthIndex in the return
        }
    }
    msmtGroupData->data = msmtBuf;
    *msmtGroupDataPtr = msmtGroupData;
    return true;
}
