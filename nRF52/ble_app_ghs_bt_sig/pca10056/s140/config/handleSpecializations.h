#ifndef HANDLE_SPECIALIZATIONS_H__
#define HANDLE_SPECIALIZATIONS_H__

#include "stdbool.h"
#include "MderFloat.h"
#include "GhsControlStructs.h"


#define BP_CUFF 0
#define PULSE_OX 0
#define GLUCOSE 0
#define HEART_RATE 0    // At this time USES_TIMESTAMP in GhsControlStructs.h must be set to 0 or it wont work right
#define SPIROMETER 0
#define SCALE 0
#define THERMOMETER 1

#define NUMBER_OF_STORED_MSMTS 30
#define SUPPORT_PAIRING 1  // 1: requires pairing/bonding 0: no pairing or bonding
#define USES_STORED_DATA 1
#define USES_LIVE_DATA 1

#define USE_DK 1        // Set NRF_LOG_ENABLED to 0 when DK is 0. The idea is either DK or nRF52840 dongle
                        // Board in preprocessor needs to be changed from BOARD_PCA10056 (DK) to BOARD_PCA10059 (dongle)

extern s_GhsTime *sGhsTime;
extern s_TimeInfo *sTimeInfo;
extern s_TimeInfoData *sTimeInfoData;
extern s_SystemInfoData *systemInfoData;
extern unsigned short numberOfStoredMsmtGroups;
extern unsigned short initialNumberOfStoredMsmtGroups;
extern unsigned long long latestTimeStamp;
extern unsigned long long epoch;
extern unsigned long long factor;
extern unsigned char security_char[];
extern unsigned short security_char_length;
extern unsigned char feature[];
extern unsigned short feature_length;
extern unsigned long recordNumber;
extern unsigned long msmt_id;

typedef struct
{
    bool hasTimeStamp;
    s_GhsTime sGhsTime;
    unsigned long recordNumber;            // For stored data only
    bool isStoredData;
}s_MsmtCommon;

#if (BP_CUFF == 1)  // Define little endian
    #define BP_STATUS_MOVEMENT 0x01
    #define BP_STATUS_CUFF_TOO_LOOSE 0x02
    #define BP_STATUS_IRREGULAR_PULSE 0x04
    #define BP_STATUS_PULSE_UNDER_LIMIT 0x08
    #define BP_STATUS_PULSE_OVER_LIMIT 0x10
    #define BP_STATUS_IMPROPER_POSITION 0x20
    #define BP_STATUS_STATES 0x0000
    #define BP_STATUS_MOVEMENT_SUPPORTED 0x01
    #define BP_STATUS_CUFF_TOO_LOOSE_SUPPORTED 0x02
    #define BP_STATUS_IRREGULAR_PULSE_SUPPORTED 0x04
    #define BP_STATUS_PULSE_UNDER_LIMIT_SUPPORTED 0x08
    #define BP_STATUS_PULSE_OVER_LIMIT_SUPPORTED 0x10
    #define BP_STATUS_IMPROPER_POSITION_SUPPORTED 0x20
    #define BP_STATUS_ALL_SUPPORTED 0x003F
    #define LIVE_COUNT_MAX 64
    
    // We define this structure to carry the measurements our blood pressure cuff can generate. The contents and name of the
    // structure is up to the application. If your device doesnt send status events, there is no reason to include them in your
    // structure. Some BP cuffs do not report a mean and therefore would not include that either.
    typedef struct
    {
        s_MsmtCommon common;
        unsigned short systolic;                // our fake data generates simulates a BP cuff that reports it values as whole integers. No fractional part.
        unsigned short diastolic;
        unsigned short mean;
        unsigned short has_msmt_id;
        unsigned short pulseRate;               // The pulse rate is also only to whole integer values
        unsigned short status_movement;         // The rest of the values are the status values as specified by the IEEE ii073 10407 BP specialization.
        bool hasStatus;
        unsigned short status_cuff_too_loose;   // The values are given by their MDer values, so the status_cuff_too_loose value when set is 0x4000 which
                                                // is given by BP_STATUS_CUFF_TOO_LOOSE above.
        unsigned short status_irregular_pulse;
        unsigned short status_pulse_under_limit;
        unsigned short status_pulse_over_limit;
        unsigned short status_improper_position;
    }s_MsmtData;
#endif
#if (PULSE_OX == 1)
    #define LIVE_COUNT_MAX 32
    typedef struct
    {
        s_MsmtCommon common;
        bool isContinuous;
        unsigned short spo2;
        unsigned short pulseRate;
        unsigned short pulseQuality;
    }s_MsmtData;
#endif
#if (GLUCOSE == 1)
    typedef struct
    {
        s_MsmtCommon common;
        unsigned long meal_context;
        unsigned long tester;
        unsigned long body_site;
        unsigned long health;
        unsigned long medication_type;
        unsigned long carbs_type;
        unsigned short conc;    // mg/dL * 10
        unsigned short carbs;   // grams
        unsigned short meds;    // IU * 10
        unsigned short exer;    // percent
        unsigned short duration;   // seconds
    }s_MsmtData;
#endif
#if (HEART_RATE == 1)
    #define LIVE_COUNT_MAX 2
    typedef struct
    {
        s_MsmtCommon common;
        unsigned char heartRate;
    }s_MsmtData;
#endif
#if (SPIROMETER == 1)
    #define LIVE_COUNT_MAX 8

    typedef struct
    {
        s_MsmtCommon common;
    }s_MsmtData;

    typedef struct
    {
        s_MsmtCommon common;
        unsigned short fev05;           // milliliters
        unsigned short fev075;          // milliliters
        unsigned short fev1;            // milliliters  fev1/fvc and fev1/fev6 is in %
        unsigned short fev3;            // milliliters
        unsigned short fev6;            // milliliters
        unsigned short fvc;             // milliliters
        unsigned short pef;             // milliliters/second
        unsigned short fef25;           // milliliters/second
        unsigned short fef50;           // milliliters/second
        unsigned short fef75;           // milliliters/second
        unsigned short fef25_75;        // milliliters/second
        unsigned short fet;             // milliseconds
        unsigned short tpef;            // milliseconds
        unsigned short extrap;          // milliliters
        unsigned short temp;            // deg * 10
        unsigned short humid;           // percent
        unsigned short airPress;        // HECTO_PASCAL (millibars)
        unsigned short fev1Z;           // dimless
        unsigned short fev1PP;          // percent
        unsigned short fev1LLN;         // milliliters
    }s_MsmtSpiroManeuv;

    typedef struct
    {
        s_MsmtCommon common;
        unsigned char *flow;
        unsigned char *volume;
    }s_SpiroStream;

    typedef struct
    {
        s_MsmtCommon common;
        unsigned short age;             // years
        unsigned short weight;          // kgs * 100
        unsigned short height;          // cm
        unsigned long sex;              // MDC code
        unsigned long ethnicity;        // MDC code
    }s_SpiroSettings;

    typedef struct
    {
        s_MsmtCommon common;
        unsigned long fev1AtsGrade;     // MDC code
        unsigned long fvcAtsGrade;      // MDC code
    }s_SpiroSummary;

    typedef struct  // This is the only structure used
    {
        s_MsmtCommon common;
        unsigned long sessionType;      // MDC code
    }s_SpiroSession;

    
    typedef struct
    {
        s_MsmtCommon common;
        unsigned long sub_sessionType;      // MDC code
    }s_SpiroSubSession;

#endif
#if (SCALE == 1)
    #define LIVE_COUNT_MAX 8
    typedef struct
    {
        s_MsmtCommon common;
        unsigned short mass;        // Weight * 100
    }s_MsmtData;
#endif
#if (THERMOMETER == 1)
    #define LIVE_COUNT_MAX 8
    typedef struct
    {
        s_MsmtCommon common;
        unsigned short temp;  // Body Temperature * 100
        unsigned short ambient;  // Room Temperature * 100
    }s_MsmtData;
#endif

unsigned char *getBtAddress(void);
void configureSpecializations(void);
bool generateAndAddStoredMsmt(unsigned long long timeStampMsmt, unsigned long timeStamp, unsigned short numberOfStoredMsmtGroups);
void handleSpecializationsOnSetTime(unsigned short numberOfStoredMsmtGroups, long long diff, unsigned short timeSync);
void sendStoredSpecializationMsmts(unsigned short stored_count);
void deleteStoredSpecializationMsmts(void);
unsigned short getNumberOfStoredRecords(unsigned char* cmd, unsigned short len);
long getStartIndexInStoredRecords(unsigned char* cmd, unsigned short len);
bool encodeSpecializationMsmts(s_MsmtData *msmt);
void generateLiveDataForSpecializations(unsigned long live_data_count, unsigned long long timeStampMsmt, unsigned long timeStamp);
void setNotOnCurrentTimeline(unsigned long long newCount);
void cleanUpSpecializations(void);

#endif
