/* configEncoder.c */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "btle_utils.h"  // Contains "GhsControlStructs.h" which is the only include method needed
#include "nomenclature.h"


#ifdef _WIN32
    #define NRF_LOG_DEBUG printf
    #include <windows.h>
#else
    #include "nrf_log.h"
#endif

#if (USES_AVAS == 1)
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
            NRF_LOG_DEBUG("Unable to allocate memory for %d AVA pointers.", numberOfAvas);
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
            NRF_LOG_DEBUG("Could not allocate memory for the Ava.");
            return false;
        }
        avas->value = (unsigned char*)calloc(1, avaIn->length * sizeof(unsigned char));
        if (avas->value == NULL)
        {
            free(avas);
            *avasPtr = NULL;
            NRF_LOG_DEBUG("Could not allocate memory for the Ava value field of %d bytes.", avaIn->length);
            return false;
        }
        avas->attrId = avaIn->attrId;
        avas->length = avaIn->length;
        memcpy(avas->value, avaIn->value, avaIn->length);
        *avasPtr = avas; // put the allocated value back into the passed pointer to pointer.
        return true;
    }
#endif

static bool checkGhsMsmt(s_GhsMsmt *ghsMsmt)
{
    if (ghsMsmt == NULL)
    {
        NRF_LOG_DEBUG("s_GhsMsmt has not been initialized. Call one of the create measurement methods");
        return false;
    }
    return true;
}

static bool checkMsmtGroup(s_MsmtGroup *msmtGroup)
{
    if (msmtGroup == NULL)
    {
        NRF_LOG_DEBUG("s_MsmtGroup has not been initialized. Call initializeMsmtGroup()");
        return false;
    }
    return true;
}

static bool checkMsmtGroupData(s_MsmtGroupData* msmtGroupData)
{
    if (msmtGroupData == NULL)
    {
        NRF_LOG_DEBUG("s_MsmtGroupData has not been generated. Call createMsmtGroupDataArray()");
        return false;
    }
    return true;
}

//======================= Time Info
#if (USES_TIMESTAMP == 1)
    void cleanUpGhsTime(s_GhsTime **sGhsTimePtr)
    {
        s_GhsTime* sGhsTime = *sGhsTimePtr;
        if (sGhsTime != NULL)
        {
            free(sGhsTime);
           *sGhsTimePtr = NULL;
        }
    }

    void cleanUpTimeInfo(s_TimeInfo** timeInfoPtr)
    {
        s_TimeInfo* timeInfo = *timeInfoPtr;
        if (timeInfo != NULL)
        {
            //if (timeInfo->ghsTime != NULL)
            //{
            //    free(timeInfo->ghsTime);
            //    timeInfo->ghsTime = NULL;
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

    bool createGhsTime(s_GhsTime **sGhsTimePtr, short offsetShift, unsigned short clockType, unsigned short clockResolution, unsigned short timeSync)
    {
        if (sGhsTimePtr == NULL)
        {
            NRF_LOG_DEBUG("Input sGhsTimePtr was NULL");
            return false;
        }
        s_GhsTime* sGhsTime = *sGhsTimePtr;
        if (sGhsTime != NULL)
        {
            cleanUpGhsTime(sGhsTimePtr);
        }
        sGhsTime = (s_GhsTime *)calloc(1, sizeof(s_GhsTime));
        if (sGhsTime == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for the s_GhsTime struct");
            return false;
        }
        sGhsTime->clockType = (unsigned char)clockType;
        sGhsTime->clockResolution = (unsigned char)clockResolution;
        sGhsTime->offsetShift = offsetShift;
        sGhsTime->flagKnownTimeline = GHS_TIME_FLAG_ON_CURRENT_TIMELINE;
        if (offsetShift != GHS_TIME_OFFSET_UNSUPPORTED)
        {
            sGhsTime->flagSupportsOffset = GHS_TIME_FLAG_SUPPORTS_TIMEZONE;
        }
        sGhsTime->timeSync = (unsigned char)timeSync;
        *sGhsTimePtr = sGhsTime;
        return true;
    }
    
    bool setOnCurrentTimeLineState(s_GhsTime **sGhsTimePtr, bool onCurrentTimeline)
    {
        if (sGhsTimePtr == NULL || *sGhsTimePtr == NULL)
        {
            NRF_LOG_DEBUG("Input sGhsTimePtr was NULL");
            return false;
        }
        s_GhsTime* sGhsTime = *sGhsTimePtr;
        sGhsTime->clockResolution = onCurrentTimeline ? GHS_TIME_FLAG_ON_CURRENT_TIMELINE : 0;
        *sGhsTimePtr = sGhsTime;
        return true;
    }


    bool createTimeInfo(s_TimeInfo **sTimeInfoPtr, s_GhsTime *sGhsTime, bool wantSetTime)
    {
        s_TimeInfo* sTimeInfo = *sTimeInfoPtr;
        if (sTimeInfo != NULL)
        {
            cleanUpTimeInfo(sTimeInfoPtr);
        }
        sTimeInfo = (s_TimeInfo *)calloc(1, sizeof(s_TimeInfo));
        if (sTimeInfo == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for the s_TimeInfo struct");
            return false;
        }
        if (sGhsTime != NULL)
        {
            sTimeInfo->timeFlagsWantSetTime = wantSetTime ? TIME_FLAGS_WANT_SET_TIME : 0;
            sTimeInfo->ghsTime = sGhsTime;
        }
        *sTimeInfoPtr = sTimeInfo;
        return true;
    }
    
    bool createCurrentTimeDataBuffer(s_TimeInfoData** timeInfoDataPtr, s_TimeInfo *sTimeInfo)
    {
        // |ghs current time|status flags|capability flags| = |9|1|1
        if (sTimeInfo == NULL)
        {
            NRF_LOG_DEBUG("TimeInfo struct has not been initialized");
            return false;
        }
        unsigned short index = 0;
        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
        if (timeInfoData != NULL)
        {
            cleanUpTimeInfoData(timeInfoDataPtr);
        }
        timeInfoData = (s_TimeInfoData *)calloc(1, sizeof(s_TimeInfoData));
        if (timeInfoData == NULL)
        {
            NRF_LOG_DEBUG("Could not allocate memory for time info Data struct");
            return false;
        }
        if (sTimeInfo == NULL || sTimeInfo->ghsTime == NULL)
        {
            *timeInfoDataPtr = timeInfoData;
            return true;
        }
        // |ghs current time|status flags|capability flags| = |9|1|1
        unsigned short timeInfoLength = TIME_STAMP_LENGTH + 2;
        timeInfoData->timeInfoBuf = (unsigned char *)calloc(1, timeInfoLength * sizeof(unsigned char));
        if (timeInfoData->timeInfoBuf == NULL)
        {
            NRF_LOG_DEBUG("Could not allocate memory for time info buffer in the Data struct");
            return false;
        }
        timeInfoData->currentTime_index = -1;
        timeInfoData->dataLength = (unsigned short)timeInfoLength;
        
        timeInfoData->currentTime_index = index;
        // First in the buffer is the GHsTime current time = |flags|epoch|offset|sync| = |1|6|1|1|
        timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_FLAGS] = (sTimeInfo->ghsTime->flagSupportsOffset 
                                                | sTimeInfo->ghsTime->flagKnownTimeline
                                                | sTimeInfo->ghsTime->clockResolution 
                                                | sTimeInfo->ghsTime->clockType);

        timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_TIME_SYNC] = sTimeInfo->ghsTime->timeSync;
        timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_OFFSET] = (unsigned char)(sTimeInfo->ghsTime->offsetShift & 0xFF);
        timeInfoData->flags_index = index + TIME_STAMP_LENGTH;
        timeInfoData->timeInfoBuf[index + TIME_STAMP_LENGTH] = (unsigned char)sTimeInfo->timeFlagsWantSetTime;
        timeInfoData->timeInfoBuf[index + TIME_STAMP_LENGTH + 1] = 0;       // No capabilties yet

        *timeInfoDataPtr = timeInfoData;
        return true;
    }

    bool updateCurrentTimeFromSetTime(s_TimeInfoData** timeInfoDataPtr, unsigned char *update)
    {
        if (timeInfoDataPtr == NULL || *timeInfoDataPtr == NULL || update == NULL)
        {
            NRF_LOG_DEBUG("TimeInfo struct has not been initialized or timeInfoDataPtr itself was NULL");
            return false;
        }

        s_TimeInfoData* timeInfoData = *timeInfoDataPtr;

        int index = timeInfoData->currentTime_index;
        memcpy(&timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_EPOCH], &update[GHS_TIME_INDEX_EPOCH], 6);                   // PHG's Epoch
        // flags field left alone
        if ((timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_CAPS] & GHS_TIME_FLAG_SUPPORTS_TIMEZONE) == GHS_TIME_FLAG_SUPPORTS_TIMEZONE)    // Set to PHG's offset if PHD supports Offset
        {
            timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_OFFSET] = update[GHS_TIME_INDEX_OFFSET];
        }
        timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_TIME_SYNC] = update[GHS_TIME_INDEX_TIME_SYNC];
        timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_STATUS] = 0;  // Clear want to set time bit
        *timeInfoDataPtr = timeInfoData;
        return true;
    }
#endif

static bool checkValid(s_TimeInfoData** timeInfoDataPtr)
{
    if (timeInfoDataPtr == NULL || *timeInfoDataPtr == NULL || (*timeInfoDataPtr)->dataLength < 11)
    {
        NRF_LOG_DEBUG("TimeInfo struct has not been initialized or timeInfoDataPtr itself was NULL or time not supported");
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
    timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_OFFSET] = (unsigned char)offsetShift;
    if (offsetShift != GHS_TIME_OFFSET_UNSUPPORTED)
    {
        timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_FLAGS] = (timeInfoData->timeInfoBuf[index + GHS_TIME_INDEX_FLAGS] | GHS_TIME_FLAG_SUPPORTS_TIMEZONE);
    }
    *timeInfoDataPtr = timeInfoData;
    return true;
}

bool updateCurrentTimeFromPhdTimeSync(s_TimeInfoData** timeInfoDataPtr, unsigned char timeSync)
{
    if (!checkValid(timeInfoDataPtr))
    {
        return false;
    }

    s_TimeInfoData* timeInfoData = *timeInfoDataPtr;
    int index = timeInfoData->currentTime_index + GHS_TIME_INDEX_TIME_SYNC;
    timeInfoData->timeInfoBuf[index] = timeSync;
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
    int index = timeInfoData->currentTime_index + GHS_TIME_INDEX_EPOCH;
    int i;
    for (i = 0; i < 6; i++)
    {
        timeInfoData->timeInfoBuf[index + i] = (epoch & 0xFF);
        epoch = (epoch >> 8);
    }
    *timeInfoDataPtr = timeInfoData;
    return true;
}

unsigned char getTimeSync(s_TimeInfoData* sTimeInfoData)
{
    if (sTimeInfoData != NULL && sTimeInfoData->dataLength > 6)
    {
        return sTimeInfoData->timeInfoBuf[GHS_TIME_INDEX_TIME_SYNC];
    }
    return INFRA_MDC_TIME_SYNC_NONE;
}

// ====================== System Info

static bool checkSystemInfo(s_SystemInfo *systemInfo)
{
    if (systemInfo == NULL)
    {
        NRF_LOG_DEBUG("SystemInfo not initialized. Need to call initializeSystemInfo");
        return false;
    }
    return true;
}

void cleanUpSystemInfo(s_SystemInfo** systemInfoPtr)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (systemInfo == NULL) return;
    if (systemInfo->specializations != NULL)
    {
        free(systemInfo->specializations);
    }
    if (systemInfo->sUdi != NULL)
    {
        free(systemInfo->sUdi);
    }
    free(systemInfo);
    *systemInfoPtr = NULL;
}

bool createSystemInfo(s_SystemInfo **systemInfoPtr, unsigned short numberOfSpecializations)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    systemInfo = (s_SystemInfo *)calloc(1, sizeof(s_SystemInfo));
    if (systemInfo == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for the s_SystemInfo struct");
        return false;
    }
    systemInfo->currentSpecializationCount = 0;
    systemInfo->numberOfSpecializations = numberOfSpecializations;
    systemInfo->specializations = (s_Specialization *)calloc(1, numberOfSpecializations * sizeof(s_Specialization));
    if (systemInfo->specializations == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for the number of specializations %d", numberOfSpecializations);
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
        NRF_LOG_DEBUG("Number of specializations exceeded. Cannot add more. Skipping.");
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
    systemInfo->sUdi = (s_Udi *)calloc(1, sizeof(s_Udi));
    if (systemInfo->sUdi == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for the s_SystemInfo UDI");
        return false;
    }
    if (udi_label != NULL && strlen(udi_label) > 0)
    {
        systemInfo->sUdi->udi_label = udi_label;
        systemInfo->sUdi->flagsUdiLabel = UDI_FLAGS_LABEL;
    }
    if (udi_dev_id != NULL && strlen(udi_dev_id) > 0)
    {
        systemInfo->sUdi->udi_device_identifier = udi_dev_id;
        systemInfo->sUdi->flagsUdiDevId = UDI_FLAGS_DEV_ID;
    }
    if (udi_issuer_oid != NULL && strlen(udi_issuer_oid) > 0)
    {
        systemInfo->sUdi->udi_issuer_oid = udi_issuer_oid;
        systemInfo->sUdi->flagsUdiIssuer = UDI_FLAGS_ISSUER;
    }
    if (udi_auth_oid != NULL && strlen(udi_auth_oid) > 0)
    {
        systemInfo->sUdi->udi_authority_oid = udi_auth_oid;
        systemInfo->sUdi->flagsUdiAuthority = UDI_FLAGS_AUTH;
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
 * 16-digit HEX string. This method will put the system id in little endian order as bytes.
 */
bool setSystemIdentifier(s_SystemInfo **systemInfoPtr, char *systemId)
{
    s_SystemInfo* systemInfo = *systemInfoPtr;
    if (!checkSystemInfo(systemInfo)) return false;
    if (!hexToLittleEndianByte(systemId, systemInfo->systemId))
    {
        NRF_LOG_DEBUG("System Id was not properly encoded. Value will be all 0's.");
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

//====================================== Measurements

void cleanUpGhsMsmt(s_GhsMsmt** ghsMsmtPtr)
{
    int i;
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    #if (USES_AVAS == 1)
        cleanUpAvas(ghsMsmt->avas, ghsMsmt->numberOfAvas);
    #endif
    ghsMsmt->avas = NULL;
    if (ghsMsmt->compoundNumeric != NULL)
    {
        for (i = 0; i < ghsMsmt->compoundNumeric->numberOfComponents; i++)
        {
            free(ghsMsmt->compoundNumeric->value[i]);
        }
        free(ghsMsmt->compoundNumeric->value);
        free(ghsMsmt->compoundNumeric);
    }
    if (ghsMsmt->rtsa != NULL)
    {
        free(ghsMsmt->rtsa);
    }
    if (ghsMsmt->simpleNumeric != NULL)
    {
        free(ghsMsmt->simpleNumeric);
    }
    if (ghsMsmt->codedEnum != NULL)
    {
        free(ghsMsmt->codedEnum);
    }
    if (ghsMsmt->bitsEnum != NULL)
    {
        free(ghsMsmt->bitsEnum);
    }
    free(ghsMsmt);
    ghsMsmt = NULL;
    *ghsMsmtPtr = NULL;
}

static void cleanUpHeader(s_GhsMsmt **headerPtr)
{
    s_GhsMsmt* header = *headerPtr;
    if (header != NULL)
    {
        #if (USES_AVAS == 1)
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
        for (i = 0; i < msmtGroup->numberOfMsmts; i++)
        {
            if (msmtGroup->ghsMsmts[i] != NULL)
            {
                cleanUpGhsMsmt(&msmtGroup->ghsMsmts[i]);
                msmtGroup->ghsMsmts[i] = NULL;
            }
        }
        if (msmtGroup->ghsMsmts != NULL)
        {
            free(msmtGroup->ghsMsmts);
            msmtGroup->ghsMsmts = NULL;
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
        if (msmtGroupData->sGhsMsmtIndex != NULL)
        {
            int i;
            for (i = 0; i < msmtGroupData->currentGhsMsmtCount; i++)
            {
                if (msmtGroupData->sGhsMsmtIndex[i] != NULL)
                {
                    free(msmtGroupData->sGhsMsmtIndex[i]);
                }
            }
            free(msmtGroupData->sGhsMsmtIndex);
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
#define ID_SIZE 4
#define PERSON_ID_SIZE 2
#define LENGTH_INDEX 1
bool createMsmtGroup(s_MsmtGroup **msmtGroupPtr, bool hasTimeStamp, unsigned char numberOfGhsMsmts)
{
    if (numberOfGhsMsmts == 0)
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
        NRF_LOG_DEBUG("Could not allocate memory for the s_MsmtGroup.");
        return false;
    }
    msmtGroup->ghsMsmts = (s_GhsMsmt **)calloc(1, numberOfGhsMsmts * sizeof(s_GhsMsmt *));
    if (msmtGroup->ghsMsmts == NULL)
    {
        NRF_LOG_DEBUG("Could not allocate memory for the %d Msmt group s_GhsMsmt pointers.", numberOfGhsMsmts);
        free (msmtGroup);
        return false;
    }
    msmtGroup->header = (s_GhsMsmt *)calloc(1, sizeof(s_GhsMsmt));
    if (msmtGroup->header == NULL)
    {
        NRF_LOG_DEBUG("Could not allocate memory for the s_MsmtHeader struct.");
        free (msmtGroup->ghsMsmts);
        free (msmtGroup);
        return false;
    }
    msmtGroup->numberOfMsmts = numberOfGhsMsmts;
    msmtGroup->header->flagTimeStamp = hasTimeStamp ? FLAGS_HAS_TIMESTAMP : 0;
    *msmtGroupPtr = msmtGroup;
    return true;
}

bool setHeaderOptions(s_MsmtGroup** msmtGroupPtr, bool areSettings, bool hasPersonId, unsigned short personId)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    msmtGroup->header->flagPersonId = hasPersonId ? FLAGS_HAS_PATIENT : 0;
    if (hasPersonId)
    {
        msmtGroup->header->personId = personId;
    }
    msmtGroup->header->flagSetting = areSettings ? FLAGS_IS_SETTING : 0;
    return true;
}

bool setHeaderSupplementalTypes(s_MsmtGroup **msmtGroupPtr, unsigned short numberOfSupplementalTypes )
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;

    if (numberOfSupplementalTypes > 0)
    {
        msmtGroup->header->numberOfSuppTypes = numberOfSupplementalTypes;
        msmtGroup->header->flagSuppTypes = FLAGS_HAS_SUPPLEMENTAL_TYPES;
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
        msmtGroup->header->flagRefs = FLAGS_HAS_REFERENCES;
    }
    return true;
}

bool setHeaderDuration(s_MsmtGroup **msmtGroupPtr)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return false;
    msmtGroup->header->flagDuration = FLAGS_HAS_DURATION;
    return true;
}

#if (USES_AVAS == 1)
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
        NRF_LOG_DEBUG("Header AVAs not initialized or Ava initialization failed.");
        return false;
    }
    if (msmtGroup->header->currentAvaCount >= msmtGroup->header->numberOfAvas)
    {
        NRF_LOG_DEBUG("Number of Header Avas have been exceeded. Cannot add any more.");
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

// |msmt value types|length|flags|type|timeStamp|duration|msmt Status|msmt-id|patient-id|supp types|derived-from|hasMember|TLV|value
#define FLAGS_INDEX 3
#define TIMESTAMP_INDEX 5 // Header has no type
bool updateTimeStampEpoch(s_MsmtGroupData **msmtGroupDataPtr, unsigned long long epoch)
{
    NRF_LOG_DEBUG("Update time stamp epoch value 0x%X", epoch);
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[FLAGS_INDEX] & 0xFF + ((msmtGroupData->data[FLAGS_INDEX + 1] << 8) & 0xFF00);
    if ((flags & FLAGS_HAS_TIMESTAMP) == FLAGS_HAS_TIMESTAMP)
    {
        unsigned short i;
        for (i = 0; i < 6; i++)
        {
            msmtGroupData->data[i + TIMESTAMP_INDEX + GHS_TIME_INDEX_EPOCH] = (epoch & 0xFF);
            epoch = (epoch >> 8);
        }
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.");
        return false;
    }
    *msmtGroupDataPtr = msmtGroupData;
    return true;
}

bool updateTimeStampTimeSync(s_MsmtGroupData **msmtGroupDataPtr, unsigned short timeSync)
{
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[FLAGS_INDEX] & 0xFF + ((msmtGroupData->data[FLAGS_INDEX + 1] << 8) & 0xFF00);
    if ((flags & FLAGS_HAS_TIMESTAMP) == FLAGS_HAS_TIMESTAMP)
    {
        msmtGroupData->data[TIMESTAMP_INDEX + GHS_TIME_INDEX_TIME_SYNC] = timeSync; // Time stamp: 1 flag 6 time, 1 sync, 1 offset
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.");
        return false;
    }
    return true;
}


bool updateTimeStampOffset(s_MsmtGroupData **msmtGroupDataPtr, short offsetShift)
{
    if (msmtGroupDataPtr == NULL || *msmtGroupDataPtr == NULL)
    {
        NRF_LOG_DEBUG("Input parameters were NULL.");
        return false;
    }
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    unsigned short flags =  msmtGroupData->data[FLAGS_INDEX] & 0xFF + ((msmtGroupData->data[FLAGS_INDEX + 1] << 8) & 0xFF00);
    if ((flags & FLAGS_HAS_TIMESTAMP) == FLAGS_HAS_TIMESTAMP)
    {
        msmtGroupData->data[TIMESTAMP_INDEX + 7] = (offsetShift & 0xFF);
    }
    else
    {
        NRF_LOG_DEBUG("Time stamps not supported or time stamp was NULL.");
        return false;
    }
    return true;
}

short addGhsMsmtToGroup(s_GhsMsmt *ghsMsmt, s_MsmtGroup **msmtGroupPtr)
{
    s_MsmtGroup* msmtGroup = *msmtGroupPtr;
    if (!checkMsmtGroup(msmtGroup)) return -1;
    if (!checkGhsMsmt(ghsMsmt)) return -1;
    if (msmtGroup->currentMsmtCount >= msmtGroup->numberOfMsmts)
    {
        NRF_LOG_DEBUG("Number of GhsMsmts %d exceeds numberOfGhsMsmts %d.", (msmtGroup->currentMsmtCount + 1), msmtGroup->numberOfMsmts);
        return -1;
    }
    ghsMsmt->flagType = FLAGS_HAS_TYPE; // All measurements have a type, but the header does not
    msmtGroup->ghsMsmts[msmtGroup->currentMsmtCount++] = ghsMsmt;
    *msmtGroupPtr = msmtGroup;
    return (msmtGroup->currentMsmtCount - 1);
}

void addId(s_MsmtGroupData** msmtGroupData, unsigned short id_index, unsigned long msmt_id)
{
    if (id_index != 0)
    {
        if (ID_SIZE >= 2)
        {
            (*msmtGroupData)->data[id_index] = (msmt_id & 0xFF);
            (*msmtGroupData)->data[id_index + 1] = ((msmt_id >> 8) & 0xFF);

            if (ID_SIZE == 4)
            {
                (*msmtGroupData)->data[id_index + 2] = ((msmt_id >> 16) & 0xFF);
                (*msmtGroupData)->data[id_index + 3] = ((msmt_id >> 24) & 0xFF);
            }
        }
    }
}

#if (USES_NUMERIC == 1)
    bool createNumericMsmt(s_GhsMsmt **ghsMsmtPtr, unsigned long type, bool isSfloat, unsigned short units, bool hasMsmtId)
    {
        s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
        if (ghsMsmt != NULL)
        {
            cleanUpGhsMsmt(ghsMsmtPtr);
        }
        ghsMsmt = (s_GhsMsmt *)calloc(1, sizeof(s_GhsMsmt));
        if (ghsMsmt == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Numeric Ghs Measurement.");
            return false;
        }
        ghsMsmt->simpleNumeric = (s_SimpNumeric *) calloc(1, sizeof(s_SimpNumeric));
        if (ghsMsmt->simpleNumeric == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Simple Numeric entry.");
            free (ghsMsmt);
            return false;
        }
        ghsMsmt->type = type;
        ghsMsmt->msmtValueType = MSMT_VALUE_NUMERIC;
        ghsMsmt->flagSfloat = isSfloat ? FLAGS_USES_SFLOAT : 0;
        ghsMsmt->flagType = FLAGS_HAS_TYPE;
        ghsMsmt->flagId = (hasMsmtId ? FLAGS_HAS_OBJECT_ID : 0);
        ghsMsmt->simpleNumeric->units = units;
        *ghsMsmtPtr = ghsMsmt;
        return true;
    }

    bool updateDataNumeric(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat* value, unsigned short msmt_id)
    {
        s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
        if (!checkMsmtGroupData(msmtGroupData)) return false;

        if (value == NULL)
        {
            NRF_LOG_DEBUG("Input value is NULL. Skipping");
            return false;
        }
        if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
        {
            NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
            return false;
        }
        s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
        if (sGhsMsmtIndex->msmtValueType != MSMT_VALUE_NUMERIC)
        {
            NRF_LOG_DEBUG("Measurement is not a numeric. Skipping");
            return false;
        }
        addId(msmtGroupDataPtr, sGhsMsmtIndex->id_index, msmt_id);
        if (sGhsMsmtIndex->isSfloat)
        {
            unsigned short ieeeFloat;
            createIeeeSFloatFromMderFloat(value, &ieeeFloat);
            msmtGroupData->data[sGhsMsmtIndex->value_index] = (ieeeFloat & 0xFF);
            msmtGroupData->data[sGhsMsmtIndex->value_index + 1] = ((ieeeFloat >> 8) & 0xFF);
        }
        else
        {
            unsigned long ieeeFloat;
            createIeeeFloatFromMderFloat(value, &ieeeFloat);
            fourByteEncode(msmtGroupData->data, sGhsMsmtIndex->value_index, ieeeFloat);
        }
        return true;
    }
#endif
#if (USES_COMPOUND == 1)
    static bool createCompoundMsmt(s_GhsMsmt** ghsMsmtPtr, unsigned long type, bool isSfloat, unsigned short units, unsigned short numberOfComponents, s_Compound *compounds,
                                       bool isComplex, bool hasMsmtId)
    {
        int i;
        if (compounds == NULL || numberOfComponents == 0)
        {
            NRF_LOG_DEBUG("Compounds entry was NULL or numberOfComponents 0.");
            return false;
        }
        s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
        if (ghsMsmt != NULL)
        {
            cleanUpGhsMsmt(ghsMsmtPtr);
        }
        ghsMsmt = (s_GhsMsmt *)calloc(1, sizeof(s_GhsMsmt));
        if (ghsMsmt == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Numeric Ghs Measurement.");
            return false;
        }
        ghsMsmt->compoundNumeric = (s_CompoundNumeric *) calloc(1, sizeof(s_CompoundNumeric));
        if (ghsMsmt->compoundNumeric == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Compound Numeric Entry.");
            free (ghsMsmt);
            return false;
        }
        ghsMsmt->type = type;
        ghsMsmt->msmtValueType = MSMT_VALUE_COMPOUND_COMPLEX;
        ghsMsmt->flagSfloat = isSfloat ? FLAGS_USES_SFLOAT : 0;
        ghsMsmt->flagId = (hasMsmtId ? FLAGS_HAS_OBJECT_ID : 0);
        ghsMsmt->compoundNumeric->numberOfComponents = numberOfComponents;
        ghsMsmt->compoundNumeric->value = (s_Compound **)calloc(1, numberOfComponents * sizeof(s_Compound *));
        if (ghsMsmt->compoundNumeric->value == NULL)
        {
            NRF_LOG_DEBUG("Unable to allocate memory for Compound Numeric value pointers.");
            free (ghsMsmt->compoundNumeric);
            free (ghsMsmt);
            return false;
        }
        for (i = 0; i < numberOfComponents; i++)
        {
            ghsMsmt->compoundNumeric->value[i] = (s_Compound *)calloc(1, sizeof(s_Compound));
            if (ghsMsmt->compoundNumeric->value[i] == NULL)
            {
                NRF_LOG_DEBUG("Unable to allocate memory for Compound Numeric value %d.", i);
                while (i > 0)
                {
                    free (ghsMsmt->compoundNumeric->value[i - 1]);
                    i--;
                }
                free (ghsMsmt->compoundNumeric);
                free (ghsMsmt);
                return false;
            }
            ghsMsmt->compoundNumeric->value[i]->subType = compounds[i].subType;
            if (isComplex)
            {
                ghsMsmt->compoundNumeric->value[i]->subUnits = compounds[i].subUnits;
            }
        }
        if (!isComplex)
        {
            ghsMsmt->compoundNumeric->units = units;
        }
        *ghsMsmtPtr = ghsMsmt;
        return true;
    }

    bool updateDataCompound(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat* value, unsigned short msmt_id)
    {
        s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
        if (!checkMsmtGroupData(msmtGroupData)) return false;

        if (value == NULL)
        {
            NRF_LOG_DEBUG("Input value is NULL. Skipping");
            return false;
        }
        if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
        {
            NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
            return false;
        }


        s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
        //bool isComplex = (sGhsMsmtIndex->msmtValueType == MSMT_VALUE_COMPOUND_COMPLEX);
        if (sGhsMsmtIndex->msmtValueType != MSMT_VALUE_COMPOUND_COMPLEX)
        {
            NRF_LOG_DEBUG("Measurement is not a complex compound. Skipping");
            return false;
        }
        unsigned short i;
        addId(msmtGroupDataPtr, sGhsMsmtIndex->id_index, msmt_id);
        unsigned short index = sGhsMsmtIndex->value_index;
        unsigned short increment = 7;
        for (i = 0; i < sGhsMsmtIndex->numberOfCmpds; i++)
        {
            if (sGhsMsmtIndex->isSfloat)
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
            index = index + increment;  // increment takes into account the complex part
        }
        return true;
    }

    static bool createCompoundNumericMsmt(s_GhsMsmt** ghsMsmtPtr, unsigned long type, bool isSfloat, unsigned short units, 
        unsigned short numberOfComponents, s_Compound *compounds, bool hasMsmtId)
    {
        return createCompoundMsmt(ghsMsmtPtr, type, isSfloat, units, numberOfComponents, compounds, false, hasMsmtId);
    }

    bool createComplexCompoundNumericMsmt(s_GhsMsmt** ghsMsmtPtr, unsigned long type, bool isSfloat, 
        unsigned short numberOfComponents, s_Compound *compounds, bool hasMsmtId)
    {
        return createCompoundMsmt(ghsMsmtPtr, type, isSfloat, 0, numberOfComponents, compounds, true, hasMsmtId);
    }
#endif
#if (USES_CODED == 1)
bool createCodedMsmt(s_GhsMsmt** ghsMsmtPtr, unsigned long type, bool hasMsmtId)
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (ghsMsmt != NULL)
    {
        cleanUpGhsMsmt(ghsMsmtPtr);
    }
    ghsMsmt = (s_GhsMsmt *)calloc(1, sizeof(s_GhsMsmt));
    if (ghsMsmt == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for Coded Ghs Measurement.");
        return false;
    }
    ghsMsmt->codedEnum = (s_CodedEnum *) calloc(1, sizeof(s_CodedEnum));
    if (ghsMsmt->codedEnum == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for Coded Enum Entry.");
        free (ghsMsmt);
        return false;
    }
    ghsMsmt->type = type;
    ghsMsmt->flagId = (hasMsmtId ? FLAGS_HAS_OBJECT_ID : 0);
    ghsMsmt->msmtValueType = MSMT_VALUE_CODED;
    *ghsMsmtPtr = ghsMsmt;
    return true;
}

bool updateDataCoded(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long code, unsigned short msmt_id)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
        return false;
    }

    s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
    if (sGhsMsmtIndex->msmtValueType != MSMT_VALUE_CODED)
    {
        NRF_LOG_DEBUG("Measurement is not a coded enum. Skipping");
        return false;
    }
    addId(msmtGroupDataPtr, sGhsMsmtIndex->id_index, msmt_id);
    fourByteEncode(msmtGroupData->data, sGhsMsmtIndex->value_index, code);
    return true;
}
#endif

#if (USES_BITS == 1)
bool createBitsEnumMsmt(s_GhsMsmt** ghsMsmtPtr, unsigned long type, unsigned short state, unsigned short support, 
    unsigned char numberOfBytes, bool hasMsmtId)
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (ghsMsmt != NULL)
    {
        cleanUpGhsMsmt(ghsMsmtPtr);
    }
    ghsMsmt = (s_GhsMsmt*)calloc(1, sizeof(s_GhsMsmt));
    if (ghsMsmt == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for BITS 16 Ghs Measurement.");
        return false;
    }
    ghsMsmt->type = type;
    ghsMsmt->bitsEnum = (s_Bits*)calloc(1, sizeof(s_Bits));
    if (ghsMsmt->bitsEnum == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for BITs Enum Entry.");
        free(ghsMsmt);
        return false;
    }
    ghsMsmt->bitsEnum->stateEvent = state;
    ghsMsmt->bitsEnum->supportEvent = support;
    ghsMsmt->bitsEnum->numberOfBytes = numberOfBytes;
    ghsMsmt->flagId = (hasMsmtId ? FLAGS_HAS_OBJECT_ID : 0);
    ghsMsmt->msmtValueType = MSMT_VALUE_BITS;
    *ghsMsmtPtr = ghsMsmt;
    return true;
}

bool updateDataBits(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long bits, unsigned short msmt_id)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
        return false;
    }

    s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
    if (sGhsMsmtIndex->msmtValueType != MSMT_VALUE_BITS)
    {
        NRF_LOG_DEBUG("Measurement is not a BITS enum. Skipping");
        return false;
    }
    addId(msmtGroupDataPtr, sGhsMsmtIndex->id_index, msmt_id);
    if (sGhsMsmtIndex->numberOfBytes == 1)
    {
        msmtGroupData->data[sGhsMsmtIndex->value_index + 2] = (bits & 0xFF);
    }
    else if (sGhsMsmtIndex->numberOfBytes == 2)
    {
        twoByteEncode(msmtGroupData->data, sGhsMsmtIndex->value_index + 4, (unsigned short)(bits & 0xFFFF));
    }
    else if (sGhsMsmtIndex->numberOfBytes == 3)
    {
        twoByteEncode(msmtGroupData->data, sGhsMsmtIndex->value_index + 6, (unsigned short)(bits & 0xFFFF));
        msmtGroupData->data[sGhsMsmtIndex->value_index + 8] = ((bits >> 16) & 0xFF);
    }
    else if (sGhsMsmtIndex->numberOfBytes == 4)
    {
        fourByteEncode(msmtGroupData->data, sGhsMsmtIndex->value_index + 8, bits);
    }
    return true;
}
#endif

#if (USES_RTSA == 1)
bool createRtsaMsmt(s_GhsMsmt** ghsMsmtPtr, unsigned long type, unsigned short units, s_MderFloat *period, s_MderFloat *scaleFactor, s_MderFloat *offset, 
    unsigned short numberOfSamples, unsigned char sampleSize, bool hasMsmtId)
{
    if (period == NULL || scaleFactor == NULL || offset == NULL)
    {
        NRF_LOG_DEBUG("One or more of the s_MderFloat Input parameters are NULL.");
        return false;
    }
    if (numberOfSamples == 0 || sampleSize == 0)
    {
        NRF_LOG_DEBUG("Either the sample size or the number of samples is 0.");
        return false;
    }
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (ghsMsmt != NULL)
    {
        cleanUpGhsMsmt(ghsMsmtPtr);
    }
    ghsMsmt = (s_GhsMsmt*)calloc(1, sizeof(s_GhsMsmt));
    if (ghsMsmt == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for RTSA Ghs Measurement.");
        return false;
    }
    ghsMsmt->type = type;
    ghsMsmt->rtsa = (s_Rtsa*)calloc(1, sizeof(s_Rtsa));
    if (ghsMsmt->rtsa == NULL)
    {
        NRF_LOG_DEBUG("Unable to allocate memory for RTSA Entry.");
        free(ghsMsmt);
        return false;
    }
    ghsMsmt->rtsa->offset = *copyMderFloat(&ghsMsmt->rtsa->offset, offset);
    ghsMsmt->rtsa->period = *copyMderFloat(&ghsMsmt->rtsa->period, period);
    ghsMsmt->rtsa->scaleFactor = *copyMderFloat(&ghsMsmt->rtsa->scaleFactor, scaleFactor);
    ghsMsmt->rtsa->units = units;
    ghsMsmt->rtsa->numberOfSamples = numberOfSamples;
    ghsMsmt->rtsa->sampleSize = sampleSize;
    ghsMsmt->msmtValueType = MSMT_VALUE_RTSA;
    ghsMsmt->flagId = (hasMsmtId ? FLAGS_HAS_OBJECT_ID : 0);
    *ghsMsmtPtr = ghsMsmt;
    return true;
}

bool updateDataRtsa(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned char* samples, unsigned short dataLength, unsigned short msmt_id)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
        return false;
    }

    s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
    if (sGhsMsmtIndex->msmtValueType != MSMT_VALUE_RTSA)
    {
        NRF_LOG_DEBUG("Measurement is not an RTSA. Skipping");
        return false;
    }
    addId(msmtGroupDataPtr, sGhsMsmtIndex->id_index, msmt_id);
    memcpy(&msmtGroupData->data[sGhsMsmtIndex->value_index], samples, dataLength);
    return true;
}
#endif
bool setGhsMsmtSupplementalTypes(s_GhsMsmt **ghsMsmtPtr, unsigned short numberOfSupplementalTypes )
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (!checkGhsMsmt(ghsMsmt)) return false;

    if (numberOfSupplementalTypes > 0)
    {
        ghsMsmt->numberOfSuppTypes = numberOfSupplementalTypes;
        ghsMsmt->flagSuppTypes = FLAGS_HAS_SUPPLEMENTAL_TYPES;
        return true;
    }
    NRF_LOG_DEBUG("Number of supplemental types was zero.");
    return false;
}

bool setGhsMsmtRefs(s_GhsMsmt **ghsMsmtPtr, unsigned short numberOfRefs )
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (!checkGhsMsmt(ghsMsmt)) return false;

    if (numberOfRefs > 0)
    {
        ghsMsmt->numberOfRefs = numberOfRefs;
        ghsMsmt->flagRefs = FLAGS_HAS_REFERENCES;
        return true;
    }
    NRF_LOG_DEBUG("Number of references was zero.");
    return false;
}

bool updateDataHeaderSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, unsigned long suppType, unsigned suppType_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (suppType_index >= msmtGroupData->numberOfSuppTypes)
    {
        NRF_LOG_DEBUG("suppTypes_index exceeds number of header supplemental types allocated.");
        return false;
    }
    fourByteEncode(msmtGroupData->data, msmtGroupData->suppTypes_index + suppType_index * 4, suppType);
    return true;
}

bool updateDataGhsMsmtSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, unsigned short msmtIndex, unsigned long suppType, unsigned suppType_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
    if (suppType_index >= sGhsMsmtIndex->numberOfSuppTypes)
    {
        NRF_LOG_DEBUG("suppType_index exceeds number of supplemental types allocated.");
        return false;
    }
    fourByteEncode(msmtGroupData->data, sGhsMsmtIndex->suppTypes_index + suppType_index * 4, suppType);
    return true;
}

bool updateDataHeaderRefs(s_MsmtGroupData** msmtGroupDataPtr, unsigned short ref, unsigned ref_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (ref_index >= msmtGroupData->numberRefs)
    {
        NRF_LOG_DEBUG("ref_index exceeds number of header src handle refs allocated.");
        return false;
    }
    if (ID_SIZE == 2)
    {
        msmtGroupData->data[msmtGroupData->ref_index + (ref_index << 1)] = (ref & 0xFF);
        msmtGroupData->data[msmtGroupData->ref_index + (ref_index << 1) + 1] = ((ref >> 8) & 0xFF);
    }
    else if (ID_SIZE == 4)
    {
        fourByteEncode(msmtGroupData->data, msmtGroupData->ref_index + (ref_index << 2), ref);
    }
    return true;
}

bool updateDataHeaderDuration(s_MsmtGroupData** msmtGroupDataPtr, s_MderFloat *duration)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (duration == NULL)
    {
        NRF_LOG_DEBUG("Duration is NULL. Skipping.");
        return false;
    }
    unsigned long mder;
    createIeeeFloatFromMderFloat(duration, &mder);
    fourByteEncode(msmtGroupData->data, msmtGroupData->duration_index, mder);
    return true;
}

bool updateDataGhsMsmtRefs(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned ref, unsigned ref_index)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
        return false;
    }

    s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
    if (ref_index >= sGhsMsmtIndex->numberRefs)
    {
        NRF_LOG_DEBUG("ref_index exceeds number of src handle refs allocated.");
        return false;
    }
    if (ID_SIZE == 2)
    {
        msmtGroupData->data[sGhsMsmtIndex->ref_index + (ref_index << 1)] = (ref & 0xFF);
        msmtGroupData->data[sGhsMsmtIndex->ref_index + (ref_index << 1) + 1] = ((ref >> 8) & 0xFF);
    }
    else if (ID_SIZE == 4)
    {
        fourByteEncode( msmtGroupData->data, sGhsMsmtIndex->ref_index + (ref_index << 2), ref );
    }
    return true;
}

bool updateDataGhsMsmtDuration(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat *duration)
{
    s_MsmtGroupData* msmtGroupData = *msmtGroupDataPtr;
    if (!checkMsmtGroupData(msmtGroupData)) return false;

    if (duration == NULL)
    {
        NRF_LOG_DEBUG("Duration is NULL. Skipping.");
        return false;
    }
    if (!((msmtIndex >= 0) && (msmtIndex < msmtGroupData->currentGhsMsmtCount)))
    {
        NRF_LOG_DEBUG("Msmt index %d is invalid. Skipping", msmtIndex);
        return false;
    }

    s_GhsMsmtIndex* sGhsMsmtIndex = msmtGroupData->sGhsMsmtIndex[msmtIndex];
    if (sGhsMsmtIndex->duration_index == 0)
    {
        NRF_LOG_DEBUG("Duration is not configured. Skipping.");
        return false;
    }
    unsigned long mder;
    createIeeeFloatFromMderFloat(duration, &mder);
    fourByteEncode(msmtGroupData->data, sGhsMsmtIndex->duration_index, mder);
    return true;
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
        msmtGroupData->dataLength = msmtGroupData->dataLength - msmtGroupData->sGhsMsmtIndex[no_of_msmts]->msmt_length;
        twoByteEncode(msmtGroupData->data, LENGTH_INDEX, (msmtGroupData->dataLength - 3));
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
    if (no_of_msmts < msmtGroupData->currentGhsMsmtCount)
    {
        msmtGroupData->dataLength = msmtGroupData->dataLength + msmtGroupData->sGhsMsmtIndex[no_of_msmts]->msmt_length;
        no_of_msmts = no_of_msmts + 1;
        msmtGroupData->data[msmtGroupData->no_of_msmts_index] = (unsigned char)no_of_msmts;
        twoByteEncode(msmtGroupData->data, LENGTH_INDEX, (msmtGroupData->dataLength - 3));
        *msmtGroupDataPtr = msmtGroupData;
        return true;
    }
    return false;
}

bool setGhsMsmtDuration(s_GhsMsmt **ghsMsmtPtr)
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (!checkGhsMsmt(ghsMsmt)) return false;
    ghsMsmt->flagDuration = FLAGS_HAS_DURATION;
    return true;
}

#if (USES_AVAS == 1)
bool initializeGhsMsmtAvas(s_GhsMsmt **ghsMsmtPtr, unsigned short numberOfAvas)
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (!checkGhsMsmt(ghsMsmt)) return false;

    ghsMsmt->numberOfAvas = numberOfAvas;
    if (numberOfAvas > 0)
    {
        if (!allocateAvas(&ghsMsmt->avas, numberOfAvas))
        {
            ghsMsmt->numberOfAvas = 0;
        }
    }
    return true;
}

bool addGhsMsmtAva(s_GhsMsmt **ghsMsmtPtr, s_Avas *avaIn)
{
    s_GhsMsmt* ghsMsmt = *ghsMsmtPtr;
    if (!checkGhsMsmt(ghsMsmt)) return false;

    if (ghsMsmt->numberOfAvas == 0)
    {
        NRF_LOG_DEBUG("GhsMsmt AVAs not initialized or Ava initialization failed.");
        return false;
    }
    if (ghsMsmt->currentAvaCount >= ghsMsmt->numberOfAvas)
    {
        NRF_LOG_DEBUG("Number of Ghs Msmt Avas have been exceeded. Cannot add any more.");
        return false;
    }
    if (addAva(&ghsMsmt->avas[ghsMsmt->currentAvaCount], avaIn))
    {
        ghsMsmt->currentAvaCount++;
        ghsMsmt->flagAvas = FLAGS_HAS_TLV;
        return true;
    }
    return false;
}
#endif

// ===================== Measurement Data Array
static int encodeAlwaysGhsBase(int index, s_GhsMsmt* ghsMsmt, unsigned char* msmtBuf, s_GhsMsmtIndex **sGhsMsmtIndex)
{
    // msmt value type
    msmtBuf[index++] = (unsigned char)ghsMsmt->msmtValueType;
    // length
    index = index + 2; // reserve space
    // flags
    int flags = (ghsMsmt->flagAvas
        | ghsMsmt->flagSfloat
        | ghsMsmt->flagDuration
        | ghsMsmt->flagType
        | ghsMsmt->flagId
        | ghsMsmt->flagSetting
        | ghsMsmt->flagRefs
        | ghsMsmt->flagSuppTypes);
    index = twoByteEncode(msmtBuf, index, flags);
    // type
    index = fourByteEncode(msmtBuf, index, ghsMsmt->type);
    return index;
}

static int encodeOptionals(int index, s_GhsMsmt * ghsMsmt, unsigned char *msmtBuf, unsigned short *msmtLengths, s_GhsMsmtIndex** sGhsMsmtIndex)
{
    // |msmt value types|length|flags|[type]|timeStamp|duration|msmt Status|[msmt-id]|patient-id|supp types|derived-from|hasMember|TLV|value
    int k;
    // time stamp
    if (ghsMsmt->flagTimeStamp != 0)
    {
        index = index + TIME_STAMP_LENGTH; // reserve space
    }
    // duration
    if (ghsMsmt->flagDuration != 0)
    {
        (*sGhsMsmtIndex)->duration_index = index;
        index = index + 4;      // reserve space
        *msmtLengths = *msmtLengths + 4;
    }
    if (ghsMsmt->flagId != 0)
    {
        (*sGhsMsmtIndex)->id_index = index;
        index = index + ID_SIZE;    // reserve space 
    }
    if (ghsMsmt->flagPersonId != 0)
    {
        index = index + PERSON_ID_SIZE; // Reserve space
    }
    // supplemental types
    if (ghsMsmt->numberOfSuppTypes > 0)
    {
        msmtBuf[index++] = (unsigned char)(ghsMsmt->numberOfSuppTypes & 0xFF);
        (*sGhsMsmtIndex)->suppTypes_index = index;
        (*sGhsMsmtIndex)->numberOfSuppTypes = ghsMsmt->numberOfSuppTypes;
        index = index + ghsMsmt->numberOfSuppTypes * 4; // reserve space for supplemental types via update method
        *msmtLengths = *msmtLengths + 1 + 4 * ghsMsmt->numberOfSuppTypes;
    }
    // refs
    if (ghsMsmt->flagRefs != 0)
    {
        msmtBuf[index++] = (unsigned char)(ghsMsmt->numberOfRefs & 0xFF);
        (*sGhsMsmtIndex)->ref_index = index;
        (*sGhsMsmtIndex)->numberRefs = ghsMsmt->numberOfRefs;
        index = index + ghsMsmt->numberOfRefs * ID_SIZE; // reserve space for srefs via update method
        *msmtLengths = *msmtLengths + 1 + ID_SIZE * ghsMsmt->numberOfRefs;
    }
    #if (USES_AVAS == 1)
    if (ghsMsmt->flagAvas != 0)
    {
        msmtBuf[index++] = (unsigned char)ghsMsmt->currentAvaCount;
        *msmtLengths = *msmtLengths + 1;
        NRF_LOG_DEBUG("Msmt has %u AVAs\n", ghsMsmt->currentAvaCount);
        for (k = 0; k < ghsMsmt->currentAvaCount; k++)
        {
            unsigned short len = 6 + ghsMsmt->avas[k]->length;
            index = fourByteEncode(msmtBuf, index, ghsMsmt->avas[k]->attrId);
            index = twoByteEncode(msmtBuf, index, ghsMsmt->avas[k]->length);
            memcpy(&msmtBuf[index], ghsMsmt->avas[k]->value, (size_t)(ghsMsmt->avas[k]->length));
            index = index + ghsMsmt->avas[k]->length;
            *msmtLengths = *msmtLengths + len;
        }
    }
    #endif
    return index;
}

static unsigned short sizeOfGhsBase(s_GhsMsmt* ghsMsmt)
{
    unsigned short groupLength =   ((ghsMsmt->flagDuration != 0) ? 4 : 0) +
                                   ((ghsMsmt->flagType != 0) ? 4 : 0) +
                                   ((ghsMsmt->flagTimeStamp != 0) ? 9 : 0) +
                                   ((ghsMsmt->flagId != 0) ? ID_SIZE : 0) +
                                   ((ghsMsmt->flagPersonId != 0) ? PERSON_ID_SIZE : 0) +
                                   ((ghsMsmt->flagSuppTypes != 0) ? (1 + ghsMsmt->numberOfSuppTypes * 4) : 0) +
                                   ((ghsMsmt->flagRefs != 0) ? (1 + ghsMsmt->numberOfRefs * ID_SIZE) : 0);
    #if (USES_AVAS == 1)
    if (ghsMsmt->flagAvas != 0)
    {
        unsigned short j;
        groupLength++; // for the number of AVAs entry
        for (j = 0; j < ghsMsmt->currentAvaCount; j++)
        {
            groupLength = groupLength + ghsMsmt->avas[j]->length + 6;  // 4-byte id and 2-byte length + 1 byte # of AVAs
        }
    }
    #endif
    return groupLength + 5; // msmtValueType, length, flags
}

static unsigned short computeLengthOfMsmtGroup(s_MsmtGroup *msmtGroup)
{
    // |msmt value types|length|flags|[type]|timeStamp|duration|msmt Status|[msmt-id]|patient-id|supp types|derived-from|hasMember|TLV|value
    unsigned short j;
    unsigned short groupLength;
    groupLength = ((msmtGroup->header->flagTimeStamp != 0) ? 15 : 6) + // msmtValueType(1) length(2), flags(2), # of msmts(1)
        ((msmtGroup->header->flagType != 0) ? 4 : 0) +
        ((msmtGroup->header->flagId != 0) ? ID_SIZE : 0) +
        ((msmtGroup->header->flagPersonId != 0) ? PERSON_ID_SIZE : 0) +
        ((msmtGroup->header->flagDuration != 0) ? 4 : 0) +
        ((msmtGroup->header->flagSuppTypes != 0) ? (1 + msmtGroup->header->numberOfSuppTypes * 4) : 0) +
        ((msmtGroup->header->flagRefs != 0) ? (1 + msmtGroup->header->numberOfRefs * ID_SIZE) : 0);

    if (msmtGroup->header->flagAvas != 0)
    {
        groupLength++; // for the number of AVAs entry
        for (j = 0; j < msmtGroup->header->currentAvaCount; j++)
        {
            groupLength = groupLength + msmtGroup->header->avas[j]->length + 6 + 1;
        }
    }

    // Find the length of all GhsMeasurements
    for (j = 0; j < msmtGroup->currentMsmtCount; j++)
    {
        s_GhsMsmt* ghs = msmtGroup->ghsMsmts[j];
        groupLength = groupLength + sizeOfGhsBase(msmtGroup->ghsMsmts[j]);
        if (ghs->simpleNumeric != NULL)
        {
            groupLength = groupLength + 2 + // units
                (((ghs->flagSfloat & FLAGS_USES_SFLOAT) == FLAGS_USES_SFLOAT) ? 2 : 4);
        }
        else if (ghs->compoundNumeric != NULL)
        {
            groupLength = groupLength + 1 + // number of components
            ghs->compoundNumeric->numberOfComponents * ((ghs->flagSfloat) ? 9 : 11); // sub types + msmt type + sub units + sub values

        }
        else if (ghs->codedEnum != NULL)
        {
            groupLength = groupLength + 4;  // coded enum value
        }
        else if (ghs->bitsEnum != NULL)
        {
            groupLength = groupLength + 3 * ghs->bitsEnum->numberOfBytes + 1; // bits enum value set plus 1 for numberOfBytes field
        }
        else if (ghs->rtsa != NULL)
        {
            groupLength = groupLength + 26 + (ghs->rtsa->sampleSize * ghs->rtsa->numberOfSamples);
        }
    }
    return groupLength;
}

bool createMsmtGroupDataArray(s_MsmtGroupData** msmtGroupDataPtr, s_MsmtGroup *msmtGroup, s_GhsTime *sGhsTime)
{
    if (msmtGroup == NULL)
    {
        NRF_LOG_DEBUG("MsmtGroup not initialized.");
        return false;
    }
    unsigned short groupLength = computeLengthOfMsmtGroup(msmtGroup);

    unsigned char *msmtBuf = (unsigned char*)calloc(1, groupLength);
    if (msmtBuf == NULL)
    {
        NRF_LOG_DEBUG("Could not allocate memory for group measurement buffer");
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
        NRF_LOG_DEBUG("Could not allocate memory for group measurement data");
        free(msmtBuf);
        return false;
    }
    if (msmtGroup->currentMsmtCount > 0)
    {
        msmtGroupData->sGhsMsmtIndex = (s_GhsMsmtIndex**)calloc(1, msmtGroup->currentMsmtCount * sizeof(s_GhsMsmtIndex*));
        if (msmtGroupData->sGhsMsmtIndex == NULL)
        {
            NRF_LOG_DEBUG("Could not allocate memory for group measurement index pointers");
            free(msmtBuf);
            free(msmtGroupData);
            return false;
        }
    }
    msmtGroupData->dataLength = (unsigned short)groupLength;
    msmtGroupData->currentGhsMsmtCount = msmtGroup->currentMsmtCount;
    
    unsigned short index = 0;
    unsigned short j;

    // Encode the first measurement group: |msmt value types|length|flags|[type]|timeStamp|duration|msmt Status|[msmt-id]|patient-id|supp types|derived-from|hasMember|TLV|value
    // header - indicate group
    msmtBuf[index++] = MSMT_VALUE_GROUP;
    // header - length of PDU is given by the group length - 1
    index = twoByteEncode(msmtBuf, index, (groupLength - 3));
    // header - flags
    unsigned short flags = (unsigned char)(msmtGroup->header->flagTimeStamp | msmtGroup->header->flagDuration |
        msmtGroup->header->flagSuppTypes | msmtGroup->header->flagRefs | msmtGroup->header->flagId | msmtGroup->header->flagType |
        msmtGroup->header->flagPersonId | msmtGroup->header->flagSetting |
        msmtGroup->header->flagAvas);
    index = twoByteEncode(msmtBuf, index, flags);
    // type code if it exists
    if (msmtGroup->header->flagType != 0)
    {
        index = fourByteEncode(msmtBuf, index, msmtGroup->header->type);
    }
    // header- timestamp if it exists
    if (msmtGroup->header->flagTimeStamp != 0 && sGhsTime != NULL)
    {
        msmtGroupData->timestamp_index = index;
        msmtBuf[index + GHS_TIME_INDEX_FLAGS] = (sGhsTime->clockResolution | sGhsTime->clockType | sGhsTime->flagKnownTimeline);
        msmtBuf[index + GHS_TIME_INDEX_OFFSET] = (sGhsTime->offsetShift & 0xFF);
        msmtBuf[index + GHS_TIME_INDEX_TIME_SYNC] = sGhsTime->timeSync;
        index = index + TIME_STAMP_LENGTH;     // flags, Epoch, time sync, offset
    }
    // duration
    if (msmtGroup->header->flagDuration != 0)
    {
        index = index + 4;
    }
    // msmt id
    if (msmtGroup->header->flagId)
    {
        index = (ID_SIZE == 2) ? twoByteEncode(msmtBuf, index, msmtGroup->header->id) :
                                 fourByteEncode(msmtBuf, index, msmtGroup->header->id);
    }
    // person id
    if (msmtGroup->header->flagPersonId != 0)
    {
        if (PERSON_ID_SIZE == 2)
        {
            index = twoByteEncode(msmtBuf, index, msmtGroup->header->personId);
        }
        else
        {
            msmtBuf[index++] = (unsigned char)msmtGroup->header->personId;
        }
    }
    // supplemental types
    if (msmtGroup->header->flagSuppTypes != 0)
    {
        msmtGroupData->numberOfSuppTypes = msmtGroup->header->numberOfSuppTypes;
        msmtBuf[index++] = (msmtGroup->header->numberOfSuppTypes & 0xFF);
        msmtGroupData->suppTypes_index = index;
        index = index + msmtGroup->header->numberOfSuppTypes * 4;
    }
    // refs
    if (msmtGroup->header->flagRefs != 0)
    {
        msmtGroupData->numberRefs = msmtGroup->header->numberOfRefs;
        msmtBuf[index++] = (msmtGroup->header->numberOfRefs & 0xFF);
        msmtGroupData->ref_index = index;
        index = index + msmtGroup->header->numberOfRefs * ID_SIZE; // reserve space for refs via update method
    }
    #if (USES_AVAS == 1)
    if (msmtGroup->header->flagAvas != 0)
    {
        int i;
        msmtBuf[index++] = (unsigned char)(unsigned char)msmtGroup->header->currentAvaCount;
        for (i = 0; i < msmtGroup->header->currentAvaCount; i++)
        {
            memcpy(&msmtBuf[index], msmtGroup->header->avas[i], (size_t)(8 + msmtGroup->header->avas[i]->length));
            index = index + 8 + msmtGroup->header->avas[i]->length; // Using a 4-byte id field in AVA
        }
    }
    #endif

    // number of measurements

    msmtGroupData->no_of_msmts_index = index;
    msmtBuf[index++] = (unsigned char)(msmtGroup->currentMsmtCount);


    // ================================ Loop: over # of ghs msmts
    for (j = 0; j < msmtGroupData->currentGhsMsmtCount; j++)
    {
        s_GhsMsmtIndex *sGhsMsmtIndex = (s_GhsMsmtIndex *)calloc(1, sizeof(s_GhsMsmtIndex));
        if (sGhsMsmtIndex == NULL)
        {
            NRF_LOG_DEBUG("Could not allocate memory for group measurement index %d", j);
            free(msmtBuf);
            cleanUpMsmtGroupData(&msmtGroupData);
            return false;
        }
        unsigned short ghsLength = 0;
        s_GhsMsmt* ghs = msmtGroup->ghsMsmts[j];
        // Need to set index of length location now as we don't know the length yet.
        int lengthIndex = index + 1;
        index = encodeAlwaysGhsBase(index, ghs, msmtBuf, &sGhsMsmtIndex); // load |msmt value types|length|flags|[type]
        index = encodeOptionals(index, ghs, msmtBuf, &ghsLength, &sGhsMsmtIndex); // load |timeStamp|duration|msmt Status|[msmt-id]|patient-id|supp types|derived-from|hasMember|TLV|
        ghsLength = index - lengthIndex + 1;
        //================================= Simple Numeric
        if (ghs->simpleNumeric != NULL)
        {
            #if (USES_NUMERIC == 1)
            sGhsMsmtIndex->msmtValueType = MSMT_VALUE_NUMERIC;
            ghsLength = ghsLength + 2;
            index = twoByteEncode(msmtBuf, index, ghs->simpleNumeric->units);
            // value 
            sGhsMsmtIndex->value_index = index;
            sGhsMsmtIndex->numberOfCmpds = 1;
            sGhsMsmtIndex->isSfloat = (ghs->flagSfloat != 0);
            if (ghs->flagSfloat == 0)
            {
                index = index + 4; // Reserve spot
                ghsLength = ghsLength + 4;
            }
            else
            {
                index = index + 2;
                ghsLength = ghsLength + 2;
            }
            #endif
        }
        //================================= Compound Numeric
        else if (ghs->compoundNumeric != NULL)
        {
            #if (USES_COMPOUND == 1)
            bool isComplex = (ghs->msmtValueType == MSMT_VALUE_COMPOUND_COMPLEX);
            sGhsMsmtIndex->msmtValueType = ghs->msmtValueType;
            ghsLength = isComplex ? ghsLength + 1 : ghsLength + 3;  // units + # of components

            if (!isComplex)
            {
                // units
                index = twoByteEncode(msmtBuf, index, ghs->compoundNumeric->units);
            }
            // number of components 
            msmtBuf[index++] = (unsigned char)(ghs->compoundNumeric->numberOfComponents);

            sGhsMsmtIndex->value_index = isComplex ? index + 4 + 2 + 1 : index + 4; // The first value in the compound starts after the 4-byte nomenclature code
            sGhsMsmtIndex->numberOfCmpds = (unsigned char)ghs->compoundNumeric->numberOfComponents;
            int k;
            sGhsMsmtIndex->isSfloat = (ghs->flagSfloat != 0);
            for (k = 0; k < ghs->compoundNumeric->numberOfComponents; k++)
            {

                // Encode sub type
                index = fourByteEncode(msmtBuf, index, ghs->compoundNumeric->value[k]->subType);
                ghsLength = ghsLength + 4;
                
                if (isComplex)
                {
                    // msmt type flag (only doing numeric)
                    msmtBuf[index++] = MSMT_VALUE_NUMERIC;
                    // units
                    index = twoByteEncode(msmtBuf, index, ghs->compoundNumeric->value[k]->subUnits);
                    ghsLength = ghsLength + 3;
                }

                // now sub value
                if (ghs->flagSfloat == 0)  // FLOAT case
                {
                    index = index + 4; // Reserve spot
                    ghsLength = ghsLength + 4;
                }
                else    // SFLOAT case
                {
                    index = index + 2;
                    ghsLength = ghsLength + 2;
                }
            }
            #endif
        }

        //================================= Coded Enum
        else if (ghs->codedEnum != NULL)
        {
            #if (USES_CODED == 1)
                sGhsMsmtIndex->msmtValueType = MSMT_VALUE_CODED;
                ghsLength = ghsLength + 4;
                // value
                sGhsMsmtIndex->value_index = index;
                sGhsMsmtIndex->numberOfCmpds = 1;
                
                index = index + 4; //fourByteEncode(msmtBuf, index, ghs->codedEnum->code);
            #endif
        }

        //================================= Bits Enum
        else if (ghs->bitsEnum != NULL)
        {
            #if (USES_BITS == 1)
            sGhsMsmtIndex->msmtValueType = MSMT_VALUE_BITS;
            ghsLength = ghsLength + 3 * ghs->bitsEnum->numberOfBytes + 1;
            msmtBuf[index++] = ghs->bitsEnum->numberOfBytes;

            // value 
            sGhsMsmtIndex->value_index = index;
            sGhsMsmtIndex->numberOfCmpds = 1;
            sGhsMsmtIndex->numberOfBytes = ghs->bitsEnum->numberOfBytes;
            
            if (ghs->bitsEnum->numberOfBytes == 1)
            {
                msmtBuf[index++] = (unsigned char)(ghs->bitsEnum->supportEvent & 0xFF);  // support
                msmtBuf[index++] = (ghs->bitsEnum->stateEvent & 0xFF);  // state
                msmtBuf[index++] = (ghs->bitsEnum->bits & 0xFF); // bits
            }
            else if (ghs->bitsEnum->numberOfBytes == 2)
            {
                index = twoByteEncode(msmtBuf, index, (unsigned short)(ghs->bitsEnum->supportEvent & 0xFFFF));
                index = twoByteEncode(msmtBuf, index, (unsigned short)(ghs->bitsEnum->stateEvent & 0xFFFF));
                index = twoByteEncode(msmtBuf, index, (unsigned short)(ghs->bitsEnum->bits & 0xFFFF));
            }
            else if (ghs->bitsEnum->numberOfBytes == 3)
            {
                index = twoByteEncode(msmtBuf, index, (unsigned short)(ghs->bitsEnum->supportEvent & 0xFFFF));
                msmtBuf[index++] = ((ghs->bitsEnum->supportEvent >> 16) & 0xFF);
                index = twoByteEncode(msmtBuf, index, (unsigned short)(ghs->bitsEnum->stateEvent & 0xFFFF));
                msmtBuf[index++] = ((ghs->bitsEnum->stateEvent >> 16) & 0xFF);
                index = twoByteEncode(msmtBuf, index, (unsigned short)(ghs->bitsEnum->bits & 0xFFFF));
                msmtBuf[index++] = ((ghs->bitsEnum->bits >> 16) & 0xFF);
            }
            else if (ghs->bitsEnum->numberOfBytes == 4)
            {
                index = fourByteEncode(msmtBuf, index, ghs->bitsEnum->supportEvent);
                index = fourByteEncode(msmtBuf, index, ghs->bitsEnum->stateEvent);
                index = fourByteEncode(msmtBuf, index, ghs->bitsEnum->bits);
            }

            #endif
        }

        //================================= RTSA
        else if (ghs->rtsa != NULL)
        {
            #if (USES_RTSA == 1)
            unsigned long mder;
            sGhsMsmtIndex->msmtValueType = MSMT_VALUE_RTSA;
            unsigned short rtsaSampleLength = ghs->rtsa->sampleSize * ghs->rtsa->numberOfSamples;
            // Unit|scalefactor|offset|scaledmin|scaledmax|samplePeriod|dimension|#BytesPerSample|#samples|data
            // units
            index = twoByteEncode(msmtBuf, index, ghs->rtsa->units);
            // scaleFactor
            createIeeeFloatFromMderFloat(&ghs->rtsa->scaleFactor, &mder);
            index = fourByteEncode(msmtBuf, index, mder);
            // offset
            createIeeeFloatFromMderFloat(&ghs->rtsa->offset, &mder);
            index = fourByteEncode(msmtBuf, index, mder);
            // scaled min
            index = fourByteEncode(msmtBuf, index, ghs->rtsa->scaledMin);
            // scaled max
            index = fourByteEncode(msmtBuf, index, ghs->rtsa->scaledMax);
            // period
            createIeeeFloatFromMderFloat(&ghs->rtsa->period, &mder);
            index = fourByteEncode(msmtBuf, index, mder);
            // dimension
            msmtBuf[index++] = 1;
            // size (bytes per samples)
            msmtBuf[index++] = (unsigned char)ghs->rtsa->sampleSize;
            // number of Samples
            index = twoByteEncode(msmtBuf, index, ghs->rtsa->numberOfSamples);
            // samples
            sGhsMsmtIndex->value_index = index;
            sGhsMsmtIndex->numberOfCmpds = 1;
            index = index + rtsaSampleLength;
            ghsLength = ghsLength + 26 + rtsaSampleLength;
            #endif
        }
        sGhsMsmtIndex->msmt_length = ghsLength;
        msmtGroupData->sGhsMsmtIndex[j] = sGhsMsmtIndex;
        twoByteEncode(msmtBuf, lengthIndex, (ghsLength - 3));  // Don't need the updated lengthIndex in the return
    }
    msmtGroupData->data = msmtBuf;
    *msmtGroupDataPtr = msmtGroupData;
    return true;
}
