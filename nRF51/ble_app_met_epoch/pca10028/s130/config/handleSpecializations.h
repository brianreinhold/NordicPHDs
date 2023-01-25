#ifndef HANDLE_SPECIALIZATIONS_H__
#define HANDLE_SPECIALIZATIONS_H__

#include "stdbool.h"
#include "MderFloat.h"
#include "MetControlStructs.h"

                            // Assure USES_TIMESTAMP is set to 1 in MetControlStructs.h unless noted
#define BP_CUFF 0
#define PULSE_OX 0
#define GLUCOSE 0           // Be sure USES_STORED_DATA is set to 1 or nothing will happen
#define HEART_RATE 0        // USES_STORED_DATA can be set to 0
#define SPIROMETER 1        // USES_STORED_DATA can be set to 0
#define SCALE 0
#define THERMOMETER 0
#define NUMBER_OF_STORED_MSMTS 30
#define SUPPORT_PAIRING 1  // 1: requires pairing/bonding 0: no pairing or bonding
                           // Set to 1 to skip
#define USES_STORED_DATA 1

#define USE_DK 1        // Set NRF_LOG_ENABLED to 0 when DK is 0. The idea is either DK or nRF52840 dongle
                        // Board in preprocessor needs to be changed from BOARD_PCA10056 (DK) to BOARD_PCA10059 (dongle)

#define CONT_NONE 0
#define CONT_RECORD_SEND 1
#define CONT_RECORD_DONE 2

extern s_MetTime *sMetTime;
extern s_TimeInfo *sTimeInfo;
extern s_TimeInfoData *sTimeInfoData;
extern s_SystemInfoData *systemInfoData;
//extern unsigned short timeSync;
extern unsigned short numberOfStoredMsmtGroups;
extern unsigned short initialNumberOfStoredMsmtGroups;
extern unsigned long long latestTimeStamp;
extern unsigned long long epoch;
extern unsigned long long factor;
extern bool first_cont_sent;
extern unsigned short msmt_id;
#if (SCALE == 1)
    extern unsigned short scale_sequence;
#endif
#if (SPIROMETER == 1)
    extern unsigned short spiro_sequence;
#endif

#if (BP_CUFF == 1)
    #define SEND_OPTIMIZED 0
    #define BP_STATUS_MOVEMENT 0x8000
    #define BP_STATUS_CUFF_TOO_LOOSE 0x4000
    #define BP_STATUS_IRREGULAR_PULSE 0x2000
    #define BP_STATUS_PULSE_UNDER_LIMIT 0x1000
    #define BP_STATUS_PULSE_OVER_LIMIT 0x800
    #define BP_STATUS_IMPROPER_POSITION 0x400
    #define BP_STATUS_STATES 0x0000
    #define BP_STATUS_MOVEMENT_SUPPORTED 0x8000
    #define BP_STATUS_CUFF_TOO_LOOSE_SUPPORTED 0x4000
    #define BP_STATUS_IRREGULAR_PULSE_SUPPORTED 0x2000
    #define BP_STATUS_PULSE_UNDER_LIMIT_SUPPORTED 0x1000
    #define BP_STATUS_PULSE_OVER_LIMIT_SUPPORTED 0x800
    #define BP_STATUS_IMPROPER_POSITION_SUPPORTED 0x400
    #define BP_STATUS_ALL_SUPPORTED 0xFC00
    #define LIVE_COUNT_MAX 64
    // We define this structure to carry the measurements our blood pressure cuff can generate. The contents and name of the
    // structure is up to the application. If your device doesnt send status events, there is no reason to include them in your
    // structure. Some BP cuffs do not report a mean and therefore would not include that either.
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned short systolic;                // our fake data generates simulates a BP cuff that reports it values as whole integers. No fractional part.
        unsigned short diastolic;
        unsigned short mean;
        unsigned short pulseRate;               // The pulse rate is also only to whole integer values
        bool hasStatus;
        unsigned short status_movement;         // The rest of the values are the status values as specified by the IEEE ii073 10407 BP specialization.
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
    #define SEND_OPTIMIZED 1
    typedef struct
    {
        bool hasTimeStamp;
        bool isContinuous;
        s_MetTime sMetTime;
        unsigned short spo2;
        unsigned short pulseRate;
        unsigned short pulseQuality;
    }s_MsmtData;
#endif
#if (GLUCOSE == 1)
    #define SEND_OPTIMIZED 0
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
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
    #define SEND_OPTIMIZED 1
    #define LIVE_COUNT_MAX 2
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned char heartRate;
    }s_MsmtData;
#endif
#if (SPIROMETER == 1)
    #define SEND_OPTIMIZED 0
    #define LIVE_COUNT_MAX 8
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
    }s_MsmtData;
    
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
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
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned char *flow;
        unsigned char *volume;
    }s_SpiroStream;

    typedef struct
    {
        unsigned short age;             // years
        unsigned short weight;          // kgs * 100
        unsigned short height;          // cm
        unsigned long sex;              // MDC code
        unsigned long ethnicity;        // MDC code
    }s_SpiroSettings;

    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned long fev1AtsGrade;     // MDC code
        unsigned long fvcAtsGrade;      // MDC code
    }s_SpiroSummary;

    typedef struct  // This is the only structure used
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned long sessionType;      // MDC code
    }s_SpiroSession;

    
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned long sub_sessionType;      // MDC code
    }s_SpiroSubSession;

#endif
#if (SCALE == 1)
    #define SEND_OPTIMIZED 0
    #define LIVE_COUNT_MAX 8
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned short mass;        // Weight * 100
    }s_MsmtData;               // We calculate the BMI with a fixed height setting
#endif
#if (THERMOMETER == 1)
    #define SEND_OPTIMIZED 0
    #define LIVE_COUNT_MAX 8
    typedef struct
    {
        bool hasTimeStamp;
        s_MetTime sMetTime;
        unsigned short temp;  // Body Temperature * 100
        unsigned short ambient;  // Room Temperature * 100
    }s_MsmtData;
#endif

unsigned char *getBtAddress(void);
void configureSpecializations(void);
void generateAndAddStoredMsmt(unsigned long long timeStampMsmt, unsigned long timeStamp, unsigned short numberOfStoredMsmtGroups);
void handleSpecializationsOnSetTime(unsigned short numberOfStoredMsmtGroups, long long diff, unsigned short timeSync);
void sendStoredSpecializationMsmts(unsigned short stored_count);
void deleteStoredSpecializationMsmts(void);
bool encodeSpecializationMsmts(s_MsmtData *msmt);
void generateLiveDataForSpecializations(unsigned long live_data_count, unsigned long long timeStampMsmt, unsigned long timeStamp);
void setNotOnCurrentTimeline(unsigned long long newCount);
void cleanUpSpecializations(void);

#endif