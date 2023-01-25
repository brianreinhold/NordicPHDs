#ifndef GHS_CONTROL_STRUCTS_H__
#define GHS_CONTROL_STRUCTS_H__

#include "stdbool.h"
#include "MderFloat.h"

#define OPCODE_LENGTH  1                        /**< Length of opcode inside PO Measurement packet. */
#define HANDLE_LENGTH  2                        /**< Length of handle inside PO Measurement packet. */
#define MAX_CHAR_LEN   (BLE_L2CAP_MTU_DEF - OPCODE_LENGTH - HANDLE_LENGTH)  /**< Maximum size of a transmitted PO Measurement. */

// THese are the RACP Op-Codes
#define RACP_GET_NUM_OF_RECORDS 0x04
#define RACP_GET_RECORDS 0x01
#define RACP_GET_COMBINED 0x07
#define RACP_DELETE_RECORDS 0x02
#define RACP_ABORT 0x03

// These are the RACP operators
#define RACP_ALL 0x01
#define RACP_LTE 0x02
#define RACP_GTE 0x03
#define RACP_RANGE 0x04
#define RACP_FIRST 0x05
#define RACP_LAST 0x06

#define RACP_RECORD_NUM 0x01
#define RACP_TIMESTAMP 0x02

// The following are RACP command sequences we support made from the above
#define RACP_CMD_GET_NUM_ALL 0x0104
#define RACP_CMD_GET_NUM_GTE_TIMESTAMP 0x020304
#define RACP_CMD_GET_NUM_GTE_RECORD_NUM 0x010304
#define RACP_CMD_GET_NUM_FIRST 0x0504
#define RACP_CMD_GET_NUM_LAST 0x0604

#define RACP_CMD_GET_ALL 0x0101
#define RACP_CMD_GET_GTE_TIMESTAMP 0x020301
#define RACP_CMD_GET_GTE_RECORD_NUM 0x010301
#define RACP_CMD_GET_FIRST 0x0501
#define RACP_CMD_GET_LAST 0x0601

#define RACP_CMD_CMB_GET_ALL 0x0107
#define RACP_CMD_CMB_GET_GTE_TIMESTAMP 0x020307
#define RACP_CMD_CMB_GET_GTE_RECORD_NUM 0x010307
#define RACP_CMD_CMB_GET_FIRST 0x0507
#define RACP_CMD_CMB_GET_LAST 0x0607

#define RACP_CMD_DELETE_ALL 0x0102


#define GHSCP_SET_LIVE_DATA_MODE 0x01                 // The PHG sends this command to request live data on the brand new GHS CP
#define GHSCP_CLEAR_LIVE_DATA_MODE 0x02                 // The PHG sends this command to request stop sending of live data on the brand new GHS CP

#define TRAP_NONE 0
#define TRAP_READ 1
#define TRAP_WRITE 2
#define TRAP_BOTH 3


// System Info flags
#define SYSINFO_FLAGS_REG_STATUS 1
#define SYSINFO_FLAGS_SERIAL_NO 2
#define SYSINFO_FLAGS_FIRMWARE 4
#define SYSINFO_FLAGS_SOFTWARE 8
#define SYSINFO_FLAGS_HARDWARE 16
#define SYSINFO_FLAGS_UDI 32
#define UDI_FLAGS_LABEL 1
#define UDI_FLAGS_DEV_ID 2
#define UDI_FLAGS_ISSUER 4
#define UDI_FLAGS_AUTH 8


// flags
#define FLAGS_HAS_TYPE 0x1
#define FLAGS_HAS_TIMESTAMP 0x2
#define FLAGS_HAS_DURATION 0x4
#define FLAGS_HAS_MSMT_STATUS 0x8
#define FLAGS_HAS_OBJECT_ID 0x10
#define FLAGS_HAS_PATIENT 0x20
#define FLAGS_HAS_SUPPLEMENTAL_TYPES 0x40
#define FLAGS_HAS_REFERENCES 0x80
//#define FLAGS_HAS_MEMBER 0x100
#define FLAGS_HAS_TLV 0x200
#define FLAGS_IS_SETTING 0x400
#define FLAGS_USES_SFLOAT 0x800

// feature Flags
#define FEATURE_HAS_DEVICE_SPECIALIZATIONS 1


// Individual neasurement value types
#define MSMT_VALUE_NUMERIC 0
#define MSMT_VALUE_CODED 1
#define MSMT_VALUE_STRING 2
#define MSMT_VALUE_RTSA 3
#define MSMT_VALUE_COMPOUND_COMPLEX 4
#define MSMT_VALUE_COMPOUND_CODED 5
#define MSMT_VALUE_BITS 6
#define MSMT_VALUE_MIXED_METRIC 7
#define MSMT_VALUE_TLV 8
#define MSMT_VALUE_COMPOUND 9
#define MSMT_VALUE_GROUP 0xFF

// For the s_TimeInfo struct

#define TIME_FLAGS_SUPPORTS_SET_TIME 1      // PHD supports the set time operation

// GHS Time flags for s_GhsTime struct
#define GHS_TIME_FLAGS_RELATIVE_TIME 1      // PHD supports an 'arbitrary' or relative time. Just a tick counter where the epoch 0 value has no semantic meaning
#define GHS_TIME_FLAGS_EPOCH_TIME 2         // PHD supports just the UTC epoch but does not understand DST or Offsets. This is equivalent to ABS time
                                            // PHD supports full base offset time (UTC epoch, DST, and Offset when offset is not 0x80).
                                            // The clock types are mutually exclusive and should not really be flags.
#define GHS_TIME_FLAGS_RESOLUTION_MASK 0x0C // 00001100
#define GHS_TIME_FLAG_SUPPORTS_SECONDS 0
#define GHS_TIME_FLAG_SUPPORTS_HUNDREDTHS 0x0C
#define GHS_TIME_FLAG_SUPPORTS_MILLISECONDS 4   // 0010 1110
#define GHS_TIME_FLAG_SUPPORTS_TENTHS_MILLIS 8
#define GHS_TIME_FLAG_SUPPORTS_TIMEZONE 0x10
#define GHS_TIME_FLAG_ON_CURRENT_TIMELINE 0x20 // 0010 0000
#define GHS_TIME_OFFSET_UNSUPPORTED 0x80

// The factor values convert our 32768 ticks per second RTC to one of the supported resolutions
//#define FACTOR 10000L   // For milliseconds/10
//#define FACTOR 1000L    // For milliseconds
//#define FACTOR 100L     // For hundredths
//#define FACTOR 10L      // For tenths
//#define FACTOR 1L       // For seconds

#define GHS_TIME_EPOCH_DEFAULT 631152000L  //662774400L     // Time to 2021 01 01 00 00 00.000 from 2000 in seconds 631152000

#define BLUETOOTH_SPECIALIZATION_NOT_FOUND -1
#define BLUETOOTH_MEASUREMENTS_SENT 0
#define BLUETOOTH_INITIALIZATION_FAILED 1
#define BLUETOOTH_ADDRESS_SETTING_FAILED 2
#define BLUETOOTH_DEVICE_NAME_SETTING_FAILED 3
#define BLUETOOTH_DIS_CREATION_FAILED 4
#define BLUETOOTH_CTS_CREATION_FAILED 5
#define BLUETOOTH_BATTERY_SERVICE_CREATION_FAILED 6
#define BLUETOOTH_ADVERTISMENT_CREATION_FAILED 7
#define BLUETOOTH_SPECIALIZATION_SERVICE_FAILED 8
#define BLUETOOTH_CHARACTERISTIC_CREATION_FAILED 9
#define BLUETOOTH_DATE_TIME_CHAR_FAILED 10
#define BLUETOOTH_FEATURE_CHAR_FAILED 11
#define BLUETOOTH_ADVERTISING_START_FAILED 12
#define BLUETOOTH_SPECIALIZATION_DISCONNECTED 13
#define BLUETOOTH_SPECIALIZATION_ABORTED 14
#define BLUETOOTH_NOT_CONFIGURED 20
#define BLUETOOTH_OK 100

#define BTLE_GHS_BT_SIG_SERVICE 0x7F44
#define BTLE_RACP_CHAR 0x2A52  // RACP
#define BTLE_GHS_BT_SIG_STORED_DATA_NOT_CHAR 0x7F42
#define BTLE_GHS_BT_SIG_LIVE_DATA_NOT_CHAR 0x7F43
#define BTLE_GHS_BT_SIG_CP_CHAR 0x7F40  // GHS CP
#define BTLE_GHS_BT_SIG_FEATURE_CHAR 0x7F41
#define BTLE_GATT_BT_SIG_SECURITY_CHAR 0x2BF5

/*
Services

Generic Health Sensor service       0x7F44  (Temporary Assigned Number)

Simple Time service                 0x7F3E  (Temporary Assigned Number)

Attributes

Observations characteristic         0x7F43  (Temporary Assigned Number)

Stored Observations characteristic  0x7F42  (Temporary Assigned Number)

GHS Features characteristic         0x7F41  (Temporary Assigned Number)

Simple Time characteristic          0x7F3D  (Temporary Assigned Number)

Unique Device Identifier (UDI) characteristic   0x7F3A (Temporary Assigned Number)
*/

#define TIME_STAMP_LENGTH 9
#define GHS_TIME_INDEX_FLAGS 0
#define GHS_TIME_INDEX_EPOCH 1
#define GHS_TIME_INDEX_TIME_SYNC 7
#define GHS_TIME_INDEX_OFFSET 8

#define INFRA_MDC_TIME_SYNC_OTHER 0
#define INFRA_MDC_TIME_SYNC_SNTPV4 1
#define INFRA_MDC_TIME_SYNC_GPS 2
#define INFRA_MDC_TIME_SYNC_RADIO 3
#define INFRA_MDC_TIME_SYNC_EBWW 4
#define INFRA_MDC_TIME_SYNC_ATOMIC 5
#define INFRA_MDC_TIME_SYNC_OTHER_MOBILE 6
#define INFRA_MDC_TIME_SYNC_NONE 7

#define BTLE_CLOCK_INFO_SERVICE 0x7F3E
#define BTLE_CLOCK_INFO_CHAR 0x7F3D

#define BTLE_DEVICE_INFORMATION_SERVICE 0x180A
#define BTLE_DIS_MANUFACTURER_DEVICE_NAME_CHAR 0x2A29
#define BTLE_DIS_MODEL_NUMBER_CHAR 0x2A24
#define BTLE_DIS_SERIAL_NUMBER_CHAR 0x2A25
#define BTLE_DIS_FIRMWARE_REVISION_CHAR 0x2A26
#define BTLE_DIS_HARDWARE_REVISION_CHAR 0x2A27
#define BTLE_DIS_SOFTWARE_REVISION_CHAR 0x2A28
#define BTLE_DIS_REG_CERT_DATA_LIST_CHAR 0x2A2A
#define BTLE_DIS_SYSTEM_ID_CHAR 0x2A23
#define BTLE_DIS_UDI_CHAR 0x7F3A

#define BTLE_BATTERY_SERVICE 0x180F
#define BTLE_BATTERY_LEVEL_CHAR 0x2A19

typedef struct 
{
    unsigned short chunks_outstanding;  // chunks sent and not 'acked' (mainly for notifications)
    unsigned short offset;              // Needed to handle fragmentation. 
    unsigned char* data;                // The data buffer being indicated/notified
    unsigned short data_length;         // the total length of the data buffer being indicated 
    unsigned short handle;              // the handle of the characteristic to make the indications/notifications on
    unsigned short current_command;     // the current command being handled
    unsigned short chunk_size;          // maximum length of each indication/notification
    unsigned short number_of_groups;    // how many records to send
    unsigned long  recordNumber;        // for stored data
} s_global_send;

typedef struct
{
    unsigned long attrId;
    unsigned short length;
    unsigned char* value;
} s_Avas;

typedef struct
{
    unsigned short specialization;      // term code from partition 8
    unsigned short version;             // specialization version
} s_Specialization;

typedef struct
{
    unsigned char flagsUdiLabel;
    unsigned char flagsUdiDevId;
    unsigned char flagsUdiIssuer;
    unsigned char flagsUdiAuthority;

    char* udi_label;                // String value matching the UDI in human readable form as assigned
                                    // to the product by a recognized UDI Issuer. Zero-terminated.
    char* udi_device_identifier;    // A fixed portion of a UDI that identifies the labeler and the 
                                    // specific version or model of a device. Zero-terminated
    char* udi_issuer_oid;           // OID representing the UDI Issuing Organization, such as GS1. Zero-terminated
    char* udi_authority_oid;        // OID representing the regional UDI Authority, such as the US FDA. Zero-terminated
                                    // FDA OID: 2.16.840.1.113883.3.24
} s_Udi;

typedef struct
{
    unsigned short flagsRegStatus;
    unsigned short flagsSerialNo;
    unsigned short flagsFirmware;
    unsigned short flagsSoftware;
    unsigned short flagsHardware;
    unsigned short flagsUdi;
    unsigned short length;
    unsigned char systemId[8];
    unsigned short currentSpecializationCount;
    unsigned short numberOfSpecializations;
    s_Specialization* specializations;
    char* manufacturer;
    char* modelNumber;
    unsigned short regulationStatus;
    char* serialNo;
    char* firmware;
    char* software;
    char* hardware;
    unsigned char trapRead;
    unsigned char *regCertDataList;
    unsigned short regCertDataListLength;
    s_Udi* sUdi;

} s_SystemInfo;

typedef struct
{
    unsigned long long epoch;
    unsigned char      clockType;       // GHS_TIME_FLAGS_RELATIVE_TIME         PHD supports an 'arbitrary' or relative time. Just a tick counter where the 0 value has no semantic meaning
                                        // GHS_TIME_FLAGS_EPOCH_TIME            PHD supports just the UTC epoch but does not understand DST or Offsets. The 0 value is 2000/01/01 00:00:00.000 UTC
    unsigned char      clockResolution; // GHS_TIME_FLAG_SUPPORTS_SECONDS       PHD supports seconds resolution
                                        // GHS_TIME_FLAG_SUPPORTS_TENTHS        PHD supports tenths of seconds (not supported)
                                        // GHS_TIME_FLAG_SUPPORTS_HUNDREDTHS    PHD support hundredths
                                        // GHS_TIME_FLAG_SUPPORTS_MILLISECONDS  PHD supports milliseconds
                                        // GHS_TIME_FLAG_SUPPORTS_TENTHS_MILLIS PHD supports tenths of milliseconds
    unsigned char      flagKnownTimeline;   // GHS_TIME_FLAG_ON_CURRENT_TIMELINE        set when on current timeline
    unsigned char      flagSupportsOffset;  // GHS_TIME_FLAG_SUPPORTS_TIMEZONE
    short              offsetShift;     // Value 0x80 (-128) in byte on wire indicates not supported
    unsigned char      timeSync;
} s_GhsTime;

typedef struct
{
    unsigned short timeFlagsSetTime;    // Whether the device supports the set time operation.
    s_GhsTime *ghsTime;                 // Pointer to an s_GhsTime struct that needs to remain in scope
} s_TimeInfo;

typedef struct
{
    s_MderFloat value;
    unsigned short units;
} s_SimpNumeric;

typedef struct
{
    unsigned long subType;              // the subtype 32 nomenclature code
    s_MderFloat subValue;               // the sub value
    unsigned short subUnits;            // when complex

} s_Compound;

typedef struct
{
    unsigned short numberOfComponents;  // the number of sub components in the compound
    s_Compound** value;
    unsigned short units;               // when not complex
} s_CompoundNumeric;

typedef struct
{
    unsigned long code;                 // the measurement as a 32 bit nomenclature code - we just need a placeholder
} s_CodedEnum;

typedef struct
{
    unsigned short length;
    unsigned short numberOfBytes;
    unsigned long bits;                 // the bits measurement
    unsigned long stateEvent;           // whether the bit is a state (set) or event (cleared)
    unsigned long supportEvent;         // whether the bit is supported (set) or not (cleared)
} s_Bits;

typedef struct
{
    s_MderFloat scaleFactor;            // part of the y = mx + b. This is the 'm' value. y= original quantity, x = scaled value (FLOAT)
    s_MderFloat offset;                 // part of the y = mx + b. This is the 'b' value (FLOAT)
    s_MderFloat period;                 // the length in seconds (FLOAT)
    long scaledMin;
    long scaledMax;
    unsigned char sampleSize;           // Size of each sample in bits
    unsigned short numberOfSamples;     // The number of samples
            // the samples are handled in updateData methods
    unsigned short units;               // the units
} s_Rtsa;

typedef struct
{
    unsigned long type;                     // The 32-bit nomenclature code giving the measurement type
    
    unsigned short flagTimeStamp;
    unsigned short flagPersonId;
    unsigned short flagType;
    unsigned short flagSuppTypes;           // Set to FLAGS_HAS_SUPP_TYPES if measurement has supplemental types
    unsigned short flagRefs;                // Set to FLAGS_HAS_REFS if measurement has references to other measurements
    unsigned short flagDuration;            // Set to FLAGS_HAS_DURATION if measurement has a duration
    unsigned short flagAvas;                // Set to FLAGS_HAS_AVAs if measurement has AVA structs
    unsigned short flagSfloat;              // Set to FLAGS_USES_SFLOAT if numeric measurement value uses SFLOATs vs FLOATs
    unsigned short flagSetting;             // Set to FLAGS_IS_SETTING if msmts are settings
    unsigned short flagId;
    unsigned long id;                      // The identifier of this measurement instance
    unsigned short personId;                // only valid id flagsPersonId = HEADER_FLAGS_PERSONID
    unsigned short msmtValueType;           // Set to one of the following below:
                                            // Set to MSMT_VALUE_NUMERIC if a simple numeric
                                            // Set to MSMT_VALUE_COMPOUND_NUMERIC if a compound numeric
                                            // Set to MSMT_VALUE_CODED_ENUM if a coded measurement
                                            // Set to MSMT_VALUE_BITS_ENUM if a BITs measurement
                                            // Set to MSMT_VALUE_RTSA if an RTSA
                                            // Set to MSMT_VALUE_STRING_ENUM if a string measurement
                                            // Set to MSMT_VALUE_EXT_OBJ if an extended object (nothing but AVAs - will this ever happen?)
                                            // Set to MSMT_VALUE_COMPLEX_COMPOUND_NUMERIC if a complex compound (each component may have different units)
                                            // Set to MSMT_VALUE_GROUP if a group

    s_GhsTime* sGhsTime;
    s_SimpNumeric* simpleNumeric;           // Simple Numeric entry
    s_CompoundNumeric* compoundNumeric;     // Compound Numeric entry
    s_CodedEnum* codedEnum;                 // Coded Enumeration entry
    s_Bits* bitsEnum;                       // Bits 16 Enumeration entry
    s_Rtsa* rtsa;                           // RTSA entry
    unsigned short numberOfSuppTypes;
    unsigned short numberOfRefs;            // Only valid if the flagsRefs = MSMT_FLAGS_REFS
    unsigned short currentAvaCount;         // Used only for creating the template)
    unsigned short numberOfAvas;
    s_Avas** avas;                          // The additional attributes as AVA structs relevant for this measurement
                                            //     Only valid if the flagsAvas = MSMT_FLAGS_AVAS
} s_GhsMsmt;

typedef struct
{
    unsigned char currentMsmtCount;
    unsigned char numberOfMsmts;
    s_GhsMsmt* header;
    s_GhsMsmt** ghsMsmts;
}s_MsmtGroup;

typedef struct
{
    unsigned short dataLength;  // Length of the byte array timeInfoBuf
    unsigned short flags_index; // the index of the flags field in timeInfoBuf
    short currentTime_index;    // the index to the current time in timeInfoBuf (flags field is first in the current time)
    unsigned char *timeInfoBuf; // the byte array to be sent to the client when asked for
}s_TimeInfoData;

typedef struct
{
    unsigned short dataLength;      // the length of the byte array systemInfoBuf
    unsigned char *systemInfoBuf;   // the byte array to be sent to the client when asked for
}s_SystemInfoData;

/**
 * The idea here is that any real agent will create a few templates for the measurements the device sends. When measurement
 * values are updated from the sensor, the agent will insert these values in the template. For example the following is the
 * byte sequence sent over the airwaves for a pulse ox with SpO2, PR, and Pulse Quality using SPOT modality with relative
 * with millisecond resolution time:
     0F 00 01 00 3E 00 [A8 56 E7 00 00 00 80 00 00 00] 02 03        Header
     B8 4B 02 00 0D 00 81 08 [01 00] 20 02 [60 00] 01 3C 4C 02 00   SpO2
     1A 48 02 00 0D 00 81 08 [02 00] A0 0A [2E 00] 01 3C 4C 02 00   Pulse Rate
     30 4B 02 00 08 00 01 08 [03 00] 20 02 [44 E2]                  Pulsatile Quality
 * Only the bytes in square brackets will change when one gets an update from the sensor; all the other values remain the same.
 * So the agent creates this template, stores it in ROM, and then when active, copies it into RAM and updates the terms in square
 * brackets as it gets new data and sends it. This is easy to do for a singular use case, but difficult to do generically.
 * Generically one would populate the structures, update the fields in the structures, and recreate the data packet each time
 * the sensor updates. Here we are trying to generically create the data template. One would only do it generically if the device
 * type changed or the measurements for a given device for some reason changed. THe need for a genric scenario is likely VERY rare.
 *
 * The s_MsmtGroupData struct will have the template for the measurement group created with empty fields that need to be populated
 * with sensor info. But to do that one needs the indicies to the location of the data field to be populated. That's what the
 * s_GhsMsmtIndex struct has. There is one of these for each measurement in the group. The entries in the s_MsmtGroupData.sGhsMsmtIndex[]
 * have the s_GhsMsmtIndex struct for each measurement in the order they were added to the group.
 * s_GhsMsmtIndex.id_index gives the start index to the id for that measurement
 * s_GhsMsmtIndex.value_index gives the index to the start of the value for that measurement. This works easily except for compounds where
 * one has multiple values. The number of values is given by s_GhsMsmtIndex.numberOfValues which is 1 for everything but compounds.
 * From the value_index one has to enter the first, skip the 4 bytes of the sub-type code, and enter the next, etc. For the numeric
 * and compound entries one also needs to know whether it is a 2-byte SFLOAT or 4-byte FLOAT.
 *
 * The best thing is all of this can be done by libraries since it only needs raw C code supported on any platform. (We provide such
 * a library in this implementation.) Then all the implementer needs to do is call update methods which will use these structures
 * internally.
 */

typedef struct
{
    unsigned short msmtValueType;   // What type of measurement value type this is. One of:
                                    // Set to MSMT_VALUE_NUMERIC if a simple numeric
                                    // Set to MSMT_VALUE_COMPOUND_NUMERIC if a compound numeric
                                    // Set to MSMT_VALUE_CODED_ENUM if a coded measurement
                                    // Set to MSMT_VALUE_BITS_ENUM if a BITs measurement
                                    // Set to MSMT_VALUE_RTSA if an RTSA
                                    // Set to MSMT_VALUE_STRING_ENUM if a string measurement
                                    // Set to MSMT_VALUE_EXT_OBJ if an extended object (nothing but AVAs - will this ever happen?)
                                    // Set to MSMT_VALUE_COMPLEX_COMPOUND_NUMERIC if a complex compound (each component may have different units)
                                    // Set to MSMT_VALUE_GROUP if a group
    unsigned short msmt_length;     // The number of bytes occupied by this measurement
    unsigned short id_index;        // The index in the MsmtGroup data[] buffer for the measurement id, if 0 there is no msmt id
    unsigned char numberOfCmpds ;   // The number of compound elements
    unsigned char numberOfBytes ;   // The number of bytes in a BITS msmt.
    unsigned short value_index;     // The index in the MsmtGroup data[] buffer for the first measurement value
                                    // The size of the measurement value depends upon the metric type
                                    //  MSMT_FLAGS_SIMPLE_NUMERIC 2 bytes or 4 bytes for SFLOAT or FLOAT
                                    //  MSMT_FLAGS_COMPOUND_NUMERIC 2 bytes or 4 bytes each entry for SFLOAT or FLOAT
                                    //      Only the first index is given. After the first is done one needs to skip 
                                    //      four bytes for the 4-byte sub type MDC code and then insert the next value
                                    //      and so forth until all values are filled.
                                    //  MSMT_FLAGS_CODED_ENUM 4 bytes
                                    //  MSMT_FLAGS_16BITS_ENUM 2 bytes
                                    //  MSMT_FLAGS_32BITS_ENUM 4 bytes
                                    //  MSMT_FLAGS_RTSA variable given by the number of samples
                                    //  MSMT_FLAGS_STRING_ENUM do a memcpy of the string
    bool isSfloat;                  //  If a simple numeric or compound numeric, indicates use of SFLOAT or FLOAT
    unsigned short numberOfSuppTypes; // Maximum number of supplemental types allocated in the data array.
    unsigned short suppTypes_index;   // The index of the supplemental types data_array
    unsigned short numberRefs;        // Maximum number of references allocated in the data array.
    unsigned short ref_index;         // The index of the references
    unsigned short duration_index;    // The index of the duration
}s_GhsMsmtIndex;

typedef struct
{
    unsigned short dataLength;          // total length of the data buffer
    unsigned short timestamp_index;     // index to the time stamp
    unsigned short no_of_msmts_index;   // Index to the number of measurements
    unsigned short numberOfSuppTypes;   // How many src refs there are
    unsigned short suppTypes_index;     // Location of the supplemental Types array
    unsigned short numberRefs;          // How many references there are
    unsigned short ref_index;           // Location of the reference array
    unsigned short duration_index;      // Location of the duration
    unsigned short currentGhsMsmtCount; // The number of measurements currently in the group (this is used while creating the template and popping/pushing msmts in the group)
    s_GhsMsmtIndex **sGhsMsmtIndex;     // the array of support info for each measurement entry
    unsigned char *data;                // the byte array to be sent to the PHG
}s_MsmtGroupData;                       // Support information for using the measurement group data buffer for this measurement group

#define USES_NUMERIC 1
#define USES_COMPOUND 1
#define USES_CODED 1
#define USES_BITS 1
#define USES_RTSA 1
#define USES_AVAS 1

#define USES_TIMESTAMP 1
#endif
