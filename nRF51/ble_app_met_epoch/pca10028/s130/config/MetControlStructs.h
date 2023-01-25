#ifndef MET_CONTROL_STRUCTS_H__
#define MET_CONTROL_STRUCTS_H__

#include "stdbool.h"
#include "MderFloat.h"

#define OPCODE_LENGTH  1                        /**< Length of opcode inside PO Measurement packet. */
#define HANDLE_LENGTH  2                        /**< Length of handle inside PO Measurement packet. */
#define MAX_CHAR_LEN   (BLE_L2CAP_MTU_DEF - OPCODE_LENGTH - HANDLE_LENGTH)  /**< Maximum size of a transmitted PO Measurement. */


#define COMMAND_GET_SYS_INFO 0x000A             // This command gets the system info. Since it is static for the lifetime
                                                // of the PHD, if the PHG already has this data it need not ask for it again.
                                                // This command must come before the requesting of any measurement data.
                                                
#define COMMAND_GET_CONFIG_INFO 0x000B          // This command asks for config data. Support by the PHD is optional. The
                                                // details of the response have not been specified but it will have something
                                                // like a list of measurements supported by the PHD, such as a list of MDC
                                                // Type codes. This command must come before the requesting of any measurement data.
                                                
#define COMMAND_GET_CURRENT_TIME 0x000C         // This command gets the current time info. The PHG shall send this command every
                                                // connection. This command must come before the requesting of any measurement data.
                                                
#define COMMAND_SET_CURRENT_TIME 0x000D         // If the PHD supports setting the time the PHG shall set the time if the time has
                                                // not been set. If the time is set, the PHD shall adjust the time stamps of any 
                                                // unsent measurements accordingly to the new time line and all subsequent measurements
                                                // will be on this new time line. Only absolute and base offset times can be set.
                                                // The PHG will never have to handle date-time-adjustments. This command must
                                                // come before the requesting of any measurement data.
                                                // If the value of the time sync is TIME_SYNC_NONE the time has not be set.
                                                
#define COMMAND_GET_NUMBER_OF_STORED_RECORDS 0x000E // This command asks the PHD for the number of stored records. A record is
                                                    // a measurement group. A PHD must always respond to this command whether or
                                                    // not it stores data.
                                                    
#define COMMAND_GET_ALL_STORED_RECORDS 0x000F       // This command requests all stored records. The PHG may skip this command if
                                                    // the number of stored records is 0 OR the PHG does not want the data.
                                                    
#define COMMAND_DELETE_ALL_STORED_RECORDS 0x0010    // The PHG may request deletion of the stored data. The PHD may not support this
                                                    // command.
                                                    
#define COMMAND_SEND_LIVE_DATA 0x0011               // The PHG sends this command to request live data. Once this command is sent, the
                                                    // PHG shall not request any of the previous commands. At the moment no command has
                                                    // been defined for stopping the live data.
#define COMMAND_PROPRIETARY 0xFFFF

// System Info flags
#define SYSINFO_FLAGS_REG_STATUS 1
#define SYSINFO_FLAGS_SERIAL_NO 2
#define SYSINFO_FLAGS_FIRMWARE 4
#define SYSINFO_FLAGS_SOFTWARE 8
#define SYSINFO_FLAGS_HARDWARE 16
#define SYSINFO_FLAGS_AVAS 32
#define UDI_FLAGS_LABEL 64
#define UDI_FLAGS_DEV_ID 128
#define UDI_FLAGS_ISSUER 256
#define UDI_FLAGS_AUTH 512

// Header flags
#define HEADER_FLAGS_TIMESTAMP 1
#define HEADER_FLAGS_SUPP_TYPES 2;
#define HEADER_FLAGS_REFS 4;
#define HEADER_FLAGS_DURATION 8
#define HEADER_FLAGS_PERSONID 16
#define HEADER_FLAGS_SETTING 32
#define HEADER_FLAGS_HAS_AVAS 64
#define HEADER_FLAGS_OPTIMIZED_FIRST 128
#define HEADER_FLAGS_OPTIMIZED_FOLLOWS 256
#define HEADER_FLAGS_OPTIMIZED_RECORD_DONE 512

// Individual neasurement flags
#define MSMT_FLAGS_MSMT_TYPE_MASK 15        // Use this value to mask off the lower nibble which will then give the measurement value type
#define MSMT_FLAGS_SIMPLE_NUMERIC 0         // The measurement value is a simple numeric
#define MSMT_FLAGS_COMPOUND_NUMERIC 1       // The measurement value is a compound numeric with a common unit code
#define MSMT_FLAGS_CODED_ENUM 2             // The measurement value is an MDC code
#define MSMT_FLAGS_BITS_ENUM 3              // The measurement value is a N-bit integer where each bit indicates a state or event. N = 8, 16, 24, 32, etc.

#define MSMT_FLAGS_RTSA 5                   // The measurement value is a periodic sequence of numerics
#define MSMT_FLAGS_STRING_ENUM 6            // The measurement value is a human readable string
#define MSMT_FLAGS_EXT_OBJ 7                // The entire measurement is a sequence of AVAs
#define MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC 8   // The measurement value is a complex compound numeric where the units may be different for each component.
                                            // The measurement values are mutually exclusive and are not really flags.

#define MSMT_FLAGS_SUPP_TYPES 16            // When this flag is set the measurement contains one or more supplemental types
#define MSMT_FLAGS_REFS 32                  // When this flag is set the measurement contains one or more references to other measurements
#define MSMT_FLAGS_DURATION 64              // When this flag is set the measurement contains a duration which is in seconds as an Mder FLOAT
#define MSMT_FLAGS_AVAS 128                 // When this flag is set the measurement contains one of more AVA structs
#define MSMT_FLAGS_SFLOAT_VAL 256           // When this flag is set the numeric or compound numeric values are 16-bit SFLOATs versus 32-bit FLOATs.

// For the s_TimeInfo struct

#define TIME_FLAGS_SUPPORTS_SET_TIME 1  // PHD supports the set time operation
#define TIME_FLAGS_HAS_AVAS 2           // TimeInfo array has AVAs at the end

// MET Time flags for s_MetTime struct
#define MET_TIME_FLAGS_LOCAL_TIME 0         // 0 = local time which is treated like a relative time
#define MET_TIME_FLAGS_RELATIVE_TIME 1      // PHD supports an 'arbitrary' or relative time. Just a tick counter where the epoch 0 value has no semantic meaning
#define MET_TIME_FLAGS_EPOCH_TIME 2         // PHD supports just the UTC epoch but does not understand DST or Offsets. This is equivalent to ABS time
                                            // PHD supports full base offset time (UTC epoch, DST, and Offset when offset is not 0x80).
                                            // The clock types are mutually exclusive and should not really be flags.
#define MET_TIME_FLAGS_RESOLUTION_MASK 0x1C // 00011100
#define MET_TIME_FLAG_SUPPORTS_SECONDS 0
#define MET_TIME_FLAG_SUPPORTS_TENTHS 4
#define MET_TIME_FLAG_SUPPORTS_HUNDREDTHS 8
#define MET_TIME_FLAG_SUPPORTS_MILLISECONDS 0x0C
#define MET_TIME_FLAG_SUPPORTS_TENTHS_MILLIS 0x10

// The factor values convert our 32768 ticks per second RTC to one of the supported resolutions
//#define FACTOR 10000L   // For milliseconds/10
//#define FACTOR 1000L    // For milliseconds
//#define FACTOR 100L     // For hundredths
//#define FACTOR 10L      // For tenths
//#define FACTOR 1L       // For seconds

#define MET_TIME_FLAG_UNKNOWN_TIMELINE 0x40 // 01000000

#define MET_TIME_OFFSET_UNSUPPORTED 0x80

#define MET_TIME_EPOCH_DEFAULT 631152000L     // Time to 2020 from year 2000
                                              //662774400L     // Time to 2021 01 01 00 00 00.000 from 2000 in seconds

#define  MET_TIME_LENGTH 10
#define  MET_TIME_INDEX_EPOCH 0
#define  MET_TIME_INDEX_FLAGS 6
#define  MET_TIME_INDEX_OFFSET 7
#define  MET_TIME_INDEX_TIME_SYNC 8

#define PACKET_TYPE_NORMAL 0
#define PACKET_TYPE_OPTIMIZED_FIRST 1
#define PACKET_TYPE_OPTIMIZED_FOLLOWS 2

#define METCP_COMMAND_DONE 0
#define METCP_COMMAND_RECORD_DONE 1
#define METCP_COMMAND_UNSUPPORTED 2
#define METCP_COMMAND_UNKNOWN 3
#define METCP_COMMAND_ERROR_BUSY 4
#define METCP_COMMAND_ERROR 5

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

#define BTLE_MET_SERVICE 0xF990
#define BTLE_MET_CP_CHAR 0xF991
#define BTLE_MET_RESPONSE_CHAR 0xF992

typedef struct 
{
    unsigned short chunks_outstanding;  // chunks sent and not 'acked' (mainly for notifications)
    unsigned short offset;              // Needed to handle fragmentation. 
    unsigned char* data;                // The data buffer being indicated/notified
    unsigned short data_length;         // the total length of the data buffer being indicated/notified 
    unsigned short handle;              // the handle of the characteristic to make the indications/notifications on
    unsigned short current_command;     // the current command being handled
    unsigned short chunk_size;          // maximum length of each indication/notification
    unsigned short number_of_groups;    // how many records to send
    unsigned short continuous_stage;    // when true, this is part of a continuous sequence
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
    unsigned short command;
    unsigned short flagsRegStatus;
    unsigned short flagsSerialNo;
    unsigned short flagsFirmware;
    unsigned short flagsSoftware;
    unsigned short flagsHardware;
    unsigned short flagsUdiLabel;
    unsigned short flagsUdiDevId;
    unsigned short flagsUdiIssuer;
    unsigned short flagsUdiAuthority;
    unsigned short flagsAvas;
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
    char* udi_label;                // String value matching the UDI in human readable form as assigned
                                    // to the product by a recognized UDI Issuer. Zero-terminated.
    char* udi_device_identifier;    // A fixed portion of a UDI that identifies the labeler and the 
                                    // specific version or model of a device. Zero-terminated
    char* udi_issuer_oid;           // OID representing the UDI Issuing Organization, such as GS1. Zero-terminated
    char* udi_authority_oid;        // OID representing the regional UDI Authority, such as the US FDA. Zero-terminated
                                    // FDA OID: 2.16.840.1.113883.3.24
    unsigned short currentAvaCount;
    unsigned short numberOfAvas;
    s_Avas** avas;

} s_SystemInfo;

typedef struct
{
    unsigned long long epoch;
    unsigned char      clockType; //   MET_TIME_FLAGS_RELATIVE_TIME   PHD supports an 'arbitrary' or relative time. Just a tick counter where the 0 value has no semantic meaning
                                  //   MET_TIME_FLAGS_EPOCH_TIME      PHD supports just the UTC epoch but does not understand DST or Offsets. The 0 value is 2000/01/01 00:00:00.000 UTC
    unsigned char      clockResolution; // MET_TIME_FLAG_SUPPORTS_SECONDS       PHD supports seconds resolution
                                        // MET_TIME_FLAG_SUPPORTS_TENTHS        PHD supports tenths of seconds (not supported)
                                        // MET_TIME_FLAG_SUPPORTS_HUNDREDTHS    PHD support hundredths
                                        // MET_TIME_FLAG_SUPPORTS_MILLISECONDS  PHD supports milliseconds
                                        // MET_TIME_FLAG_SUPPORTS_TENTHS_MILLIS PHD supports tenths of milliseconds
    unsigned char      flagUnknownTimeline;
    short              offsetShift;     // Value 0x80 (-128) in byte on wire indicates not supported
    unsigned short     timeSync;
} s_MetTime;

typedef struct
{
    unsigned short timeFlagsSetTime;    // If equal to TIME_FLAGS_TIME_SET PHD has had its time set
    unsigned short timeFlagsHasAvas;    // If equal to TIME_FLAGS_HAS_AVAS there are extra AVA structs at the end of the standard timeInfo byte array
    s_MetTime *metTime;                 // Pointer to an s_MetTime struct that needs to remain in scope
} s_TimeInfo;

typedef struct
{
            // the s_mderFloat measurement is handled in updateData methods
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
    unsigned short units;               // Ignored when complex
} s_CompoundNumeric;

typedef struct
{
    unsigned long code;                 // the measurement as a 32 bit nomenclature code - we just need a placeholder
} s_CodedEnum;

typedef struct
{
    unsigned short length;
    unsigned short byteCount;           // Number of bytes making up the bits (8, 16, 24, and 32). The unsigned long limits this to 32 bits.
                                        // the bits measurement is handled in updateData methods
    unsigned long stateEvent;           // whether the bit is a state (set) or event (cleared)
    unsigned long supportEvent;         // whether the bit is supported (set) or not (cleared)
} s_BitsEnum;

typedef struct
{
    s_MderFloat scaleFactor;            // part of the y = mx + b. This is the 'm' value. y= original quantity, x = scaled value (FLOAT)
    s_MderFloat offset;                 // part of the y = mx + b. This is the 'b' value (FLOAT)
    s_MderFloat period;                 // the length in seconds (FLOAT)
    unsigned char sampleSize;           // Size of each sample in bits
    unsigned short numberOfSamples;     // The number of samples
                                        // the samples are handled in updateData methods
    unsigned short units;               // the units
} s_Rtsa;

typedef struct
{
    unsigned long type;             // The 32-bit nomenclature code giving the measurement type
    unsigned short flagsMsmtType;           // Set to one of the following below:
                                            // Set to MSMT_FLAGS_SIMPLE_NUMERIC if a simple numeric
                                            // Set to MSMT_FLAGS_COMPOUND_NUMERIC if a compound numeric
                                            // Set to MSMT_FLAGS_CODED_ENUM if a coded measurement
                                            // Set to MSMT_FLAGS_BITS_ENUM
                                            // Set to MSMT_FLAGS_RTSA if an RTSA
                                            // Set to MSMT_FLAGS_STRING_ENUM if a string measurement
                                            // Set to MSMT_FLAGS_EXT_OBJ if an extended object (nothing but AVAs - will this ever happen?)
                                            // Set to MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC if a complex compound (each component may have different units)
                                            // Note that these are not 'bit flags' but values ranging from 0 to 15 of which only values up to 8
                                            // have been defined. Values are used since measurement value types are mutually exclusive...one can
                                            // only have one measurement at a time.
    unsigned short flagsSuppTypes;          // Set to MSMT_FLAGS_SUPP_TYPES if measurement has supplemental types
    unsigned short flagsRefs;               // Set to MSMT_FLAGS_REFS if measurement has references to other measurements
    unsigned short flagsDuration;           // Set to MSMT_FLAGS_DURATION if measurement has a duration
    unsigned short flagsAvas;               // Set to MSMT_FLAGS_AVAs if measurement has AVA structs
    unsigned short flagsSfloat;             // Set to MSMT_FLAGS_SFLOAT_VAL if numeric measurement value uses SFLOATs vs FLOATs
    unsigned short id;                      // The identifier of this measurement instance
    s_SimpNumeric* simpleNumeric;           // Simple Numeric entry
    s_CompoundNumeric* compoundNumeric;     // Compound Numeric entry
    s_CodedEnum* codedEnum;                 // Coded Enumeration entry
    s_BitsEnum* bitsEnum;                   // Bits Enumeration entry
    s_Rtsa* rtsa;                           // RTSA entry
    unsigned short numberOfSuppTypes;
    unsigned short numberOfRefs;            // Only valid if the flagsRefs = MSMT_FLAGS_REFS
    unsigned short currentAvaCount;         // Used only for creating the template)
    unsigned short numberOfAvas;
    s_Avas** avas;                          // The additional attributes as AVA structs relevant for this measurement
                                            //     Only valid if the flagsAvas = MSMT_FLAGS_AVAS
} s_MetMsmt;

typedef struct
{
    unsigned short command;
    unsigned short flagsTimeStamp;          // Set to HEADER_FLAGS_TIMESTAMP if measurements have time stamps
    unsigned short flagsSuppTypes;          // Set to HEADER_FLAGS_SUPP_TYPES if all measurements have a common supplemental types
    unsigned short flagsRefs;               // Set to HEADER_FLAGS_REFS if all measurements have common reference
    unsigned short flagsDuration;           // Set to HEADER_FLAGS_DURATION if all measurements have a common duration
    unsigned short flagsPersonId;           // Set to HEADER_FLAGS_PERSONID if there is a person id
    unsigned short flagsSetting;            // Set to HEADER_FLAGS_SETTING if the measurements are settings
    unsigned short flagsAvas;               // Set to HEADER_FLAGS_HAS_AVAS if all measurements have common AVAs
    unsigned short flagsFirstCont;          // Set to HEADER_FLAGS_CONT_FIRST if the group is first in a sequence of optimized groups
    unsigned short flagsFollowsCont;        // Set to HEADER_FLAGS_CONT_FOLLOWS if the group is one of the optimized groups after the first
    unsigned short flagsRecordCont;         // Set to HEADER_FLAGS_CONT_RECORD_DONE if the group is an optinzed record complete PDU for notification
                                            //     record complete use instead of indications on the CP. Not sure if this is worth the effort of
                                            //     supporting.
    unsigned char numberOfMsmts;            // Number of measurements in the group
    unsigned char currentMsmtCount;         // For adding MetMsmts into group (used only for creating the template)

    unsigned short numberOfSuppTypes;       // The number of supplemental types
    unsigned short numberOfRefs;            // The number of references  Only valid if the flagsRefs = HEADER_FLAGS_REFS
    unsigned short personId;                // only valid id flagsPersonId = HEADER_FLAGS_PERSONID
    unsigned short currentAvaCount;         // Used only for creating the template)
    unsigned short numberOfAvas;
    s_Avas** avas;                          // The additional attributes as AVA structs relevant for this measurement
                                            //     Only valid if the flagsAvas = MSMT_FLAGS_AVAS
    unsigned char groupId;                  // Used only if one of the HEADER_FLAGS_CONT_* are set, but always present. This group id is the same for the same
                                            // group no matter how many times the group is instantiated. It is therefore different than the measurement id which
                                            // is unique for every measurement instance. The group id is more like the 20601 handle. The measurement id is more
                                            // like the FHIR resource logical id
} s_MsmtHeader;

typedef struct
{
    s_MsmtHeader* header;
    s_MetMsmt** metMsmt;
}s_MsmtGroup;

typedef struct
{
    unsigned short dataLength;  // Length of the byte array timeInfoBuf
    unsigned short flags_index; // the index of the flags field in timeInfoBuf
    short currentTime_index;    // the index to the current time in timeInfoBuf
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
 * s_MetMsmtIndex struct has. There is one of these for each measurement in the group. The entries in the s_MsmtGroupData.sMetMsmtIndex[]
 * have the s_MetMsmtIndex struct for each measurement in the order they were added to the group.
 * s_MetMsmtIndex.id_index gives the start index to the id for that measurement
 * s_MetMsmtIndex.value_index gives the index to the start of the value for that measurement. This works easily except for compounds where
 * one has multiple values. The number of values is given by s_MetMsmtIndex.numberOfValues which is 1 for everything but compounds.
 * From the value_index one has to enter the first, skip the 4 bytes of the sub-type code, and enter the next, etc. For the numeric
 * and compound entries one also needs to know whether it is a 2-byte SFLOAT or 4-byte FLOAT.
 *
 * The best thing is all of this can be done by libraries since it only needs raw C code supported on any platform. (We provide such
 * a library in this implementation.) Then all the implementer needs to do is call update methods which will use these structures
 * internally.
 */

typedef struct
{
    unsigned short metricType;      // What type of metric this is. One of:
                                    //  MSMT_FLAGS_SIMPLE_NUMERIC
                                    //  MSMT_FLAGS_COMPOUND_NUMERIC
                                    //  MSMT_FLAGS_CODED_ENUM
                                    //  MSMT_FLAGS_BITS_ENUM
                                    //  
                                    //  MSMT_FLAGS_RTSA
                                    //  MSMT_FLAGS_STRING_ENUM
                                    //  MSMT_FLAGS_COMPLEX_COMPOUND_NUMERIC
    unsigned short msmt_length;     // The number of bytes occupied by this measurement
    unsigned short id_index;        // The index in the MsmtGroup data[] buffer for the measurement id
    unsigned short numberOfValues;  // The number of measurement values. It is always 1 except for compounds
                                    //   where there may be more than one.
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
}s_MetMsmtIndex;

typedef struct
{
    unsigned short dataLength;          // total length of the data buffer
    unsigned short no_of_msmts_index;   // Index to the number of measurements
    unsigned short numberOfSuppTypes;   // How many src refs there are
    unsigned short suppTypes_index;     // Location of the supplemental Types array
    unsigned short numberRefs;          // How many references there are
    unsigned short ref_index;           // Location of the reference array
    unsigned short duration_index;      // Location of the duration
    unsigned short currentMetMsmtCount; // The number of measurements currently in the group (this is used while creating the template and popping/pushing msmts in the group)
    s_MetMsmtIndex **sMetMsmtIndex;     // the array of support info for each measurement entry
    unsigned char *data;                // the byte array to be sent to the PHG
}s_MsmtGroupData;                       // Support information for using the measurement group data buffer for this measurement group

#define USES_NUMERIC 1
#define USES_COMPOUND 1
#define USES_CODED 1
#define USES_BITS 1

#define USES_RTSA 1
#define USES_SYSTEM_AVAS 1
#define USES_HEADER_AVAS 1
#define USES_MSMT_AVAS 1
#define USES_TIMESTAMP 1
#endif
