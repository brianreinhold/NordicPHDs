/* configEncoder.h */
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


#ifndef CONFIG_GHS_ENCODER_H__
#define CONFIG_GHS_ENCODER_H__
#include "GhsControlStructs.h"


/*
 * How to use this library
 * A PHD consists of three basic parts
 *   1. Time and clock information - needed if the PHD stores data or reports time stamps in the measurements
 *   2. Information about itself, so-called system info; All PHDs have this
 *   3. The measurements
 * This library allows you to configure the above parts for your device and generate a byte array that is to be
 * sent when needed. The measurement byte array is then updated as you receive measurement data from the sensor
 * with the new measurement values and timestamp (if you are including time stamps). In this standard measurements
 * are grouped by time stamp. For example, a blood pressure cuff sending both blood pressure and pulse rate usually
 * sends the blood pressure and pulse rate with the same time stamp, and if you want to do that, you add both the
 * blood pressure and pulse rate to the same group. So on the wire the measurement would look as follows:
 *               | HEADER WITH TIME STAMP | BLOOD PRESSURE | PULSE RATE |
 * The library creates the byte array above and then provides methods to update that byte array with new values from
 * your sensor and appropriate time stamp. The work flow would be as follows:
 *    On powerup you would first create the time properties and measurement group:
        s_MsmtGroup *msmtGroup = NULL;              // Initialize the pointer to the measurement group to NULL. Setting to NULL is important!
        unsigned char group_id = 0;
        result = createMsmtGroup(&msmtGroup,        // The library will populate this structure for you. It will allocate memory for it
                                                    // which needs to be cleaned up. The cleanup method will set the pointer back to NULL.
                                                    // If the parameter is not NULL, this method will attempt to free it which may cause
                                                    // a crash if it has not yet been allocated! NOTE THAT IT IS A POINTER to a pointer that
                                                    // gets passed.
                                 true,              // If time stamps are being used set to true
                                 2,                 // The number of measurements in the group, in this case 2, blood pressure and pulse rate
                                 group_id++);       // The group id. This value is only used for optimization should you choose to use optimization
                                                    // but it needs to be valued and unique for the group in a connection. There is a maximum of 256 groups.

 *    Now we need to add measurements to that group. The blood pressure is more complicated because it is a compound or a vector. The systolic,
 *    diastolic, and mean components are treated as parts of a single measurement like the x, y, and z components of an acceleration. Thus before
 *    we can create this measurement we need to specify what our compound measurement contains. That is done by populating an array of s_Compound
 *    structs as follows:
        s_Compound compounds[3];            // We want three; one each for the systolic, diastolic, and mean components:
        compounds[0].subType = MDC_PRESS_BLD_NONINV_SYS;    // This is a 32-bit unsigned integer which gives the MDC code for systolic blood pressure
        compounds[1].subType = MDC_PRESS_BLD_NONINV_DIA;
        compounds[2].subType = MDC_PRESS_BLD_NONINV_MEAN;
 *    Yes it is understood that you have to know what codes are and that is not always a trivial task. Included with this library is a file nomenclature.h
 *    which contains a bunch of MDC codes that will cover 99% of the possible PHDs one can create. There is also an on line tool called the NIST Rosetta
 *    which can also help though at this time it is still a work in progress and the nomenclature.h file is your better bet.
 *    The other element of this structure is the value. That we will set when we get data from the sensor. The subTypes can be set once as they will
 *    never change. So once we decide what is in our compound measurement, we create the compound measurement and then add it to our group:
        s_GhsMsmt *bp = NULL;               // Initialize this pointer to NULL!
        result = createComplexCompoundNumericMsmt(&bp,              // The library will populate this structure. It will allocate memory for it
                                                                    // NOTE THAT IT IS A POINTER to a pointer that gets passed.
                                           MDC_PRESS_BLD_NONINV,    // This is the 32-bit MDC code that expresses what the overall compound measurement
                                                                    // is. In this case it is 'non-invasive blood pressure'.
                                           true,                    // true If the values are encoded as two-byte Mder Floats versus 4-byte MderFloats
                                           MDC_DIM_MMHG,            // The units of the measurement. Complex compound are compounds where the units are
                                                                    // different for each of the sub types. The s_Compound structure has sub units in it
                                                                    // but they are igored in this case since BP is not a complex compound. There is a
                                                                    // different method to call should you want complex compounds.
                                           3,                       // The number of elements in the compound. In this case, there are three.
                                           compounds);              // A pointer to the array of s_Compound structs.
 *    For this implementation we want to tell the world that this blood pressure cuff is an upper arm cuff. Not many BP cuffs do that. In the GHS standard we
 *    can do this by including an extra MDC code in a field called 'supplemental types'. The Supplemental types allow one to say a lot of different
 *    things about the measurement, for example that it is an average, a maximum, that the sensor is placed on a certain location of the body, that a glucose
 *    measurement was taken by a tester and on the finger and it was bedtime, etc.
 *    To add a supplemental type to our measurement we call a method to set the number of supplemental types we intend to include. There are two possibilities;
 *    the supplemental types can be common to all measurements (part of the header) or just a single measurement. To add the supplemental types values we call 
 *    update methods on the data array once created. This allows us to add the supplemental types statically (just once and use always) or dynamically as data
 *    is received. In any case, we are going to add just one supplemental type to the BP measurement, not the header. We do that by calling
 *      setGhsMsmtSupplementalTypes(&bp, 1).
      This setting will eventually create space in the final data array to put one supplemental types value.
 *    Now we add this populated s_GhsMsmt struct to the group:
        bp_index = addGhsMsmtToGroup(bp,            // A pointer to the s_GhsMsmt struct we want to add - in this case the bp
                                     &msmtGroup);   // A pointer to the measurement group we want to add the measurement to.
 *    Note that the above method returns a very critical bp_index. You have to keep this value as it tells where in the measurement group this particular
 *    measurement is. We will discuss that later.
 *    Now lets create and add the pulse rate. The pulse rate is a simple numeric and is simpler to construct. The vast majority of PHD measurements are simple
 *    numerics.
        s_GhsMsmt *pr = NULL;               // Again, Initialize this pointer to NULL!
        result = createNumericMsmt(&pr,                     // A pointer to our s_GhsMst pointer that will be populated by the library.
                                   MDC_PULS_RATE_NON_INV,   // MDC code for what this measurement is, in this case a non-invasive pulse rate. It tells 
                                                            // downstream readers that this value is not only a pulse rate but it was generated from
                                                            // a blood pressure cuff device.
                                   true,                    // true If the values are encoded as two-byte Mder Floats versus 4-byte MderFloats
                                   MDC_DIM_BEAT_PER_MIN);   // The units of the pulse rate which is beats per minute
        pr_index = addGhsMsmtToGroup(pr, &msmtGroup);       // Add the measurement to the group
 *    Note again the very important pr_index value. You need to save this!

 *    Now we have populated our structures for the blood pressure/pulse rate device. But structures are not sent on the wire. So the next step is to create
 *    the byte array that will be sent over the airwaves. Once we have created this byte array, we do not need the structs we have populated. All we need
 *    is the byte array struct and the indices of the measurements in the group. So now lets create the byte array struct. This struct cannot go out of scope!
 *    It must exist for the duration of the connection! The byte array of this struct will be populated with sensor data and delivered to the PHG.
        s_MsmtGroupData *msmtGroupBpData                = NULL; // Defined as a global variable or in some manner that it will not go out of scope

        result = createMsmtGroupDataArray(&msmtGroupBpData,     // Pointer to the s_MsmtGroupData pointer initialized to NULL. The library will populate this
                                                                // structure which contains the byte array. It will allocate memory for it
                                                                // and attempt to free it if it is not NULL which may cause a crash if it has
                                                                // not been previously allocated. NOTE THAT IT IS A POINTER to a pointer that gets passed.
                                          msmtGroup,            // The measurement group to encode into a byte array
                                          sGhsTime,             // A pointer to the s_GhsTime struct containing the time properties of the PHD
                                          PACKET_TYPE_NORMAL);  // For now, use this. There is also an optimization option which we will not use.
                                                                // The optimization only makes sense if you are sending lots of data such as streams
 *    Now that we have populated this struct, we dont need the s_MsmtGroup struct anymore. We call the cleanup method to free the allocated resources. The
 *    cleanup method also frees the individual measurement structs like bp and pr as well.
         cleanUpMsmtGroup(&msmtGroup); // cleans up any allocated data -  we only need the data array now
 *    Note that all of this can happen at power up, so there is no performance penalty while connected. At this point, the only resources left that will need
 *    to be freed later is the data array struct 'msmtGroupBpData' (also the time related structs).

 *    How to use it?
 *    Assume we get data from the BP sensor of: 102, 69, 81, and PR: 64. We can use the epoch counter for the time stamp. To enter these values into our data
 *    array we call a set of update methods. There is an update method for each measurement type. In this case there is one for the compound and one for the
 *    simple. So we call these methods and pass in the measurement values. Now many Blood pressure sensors provide data as simple integers. But other sensors
 *    report data
 *    that represent values that have fractional parts. The GHS standard requires that all numerical values be formatted in a manner that gives the precision
 *    of the data. Thus a value of 2, 2.0, 2.00, and 2.000 are all 'different'. They are all numerically equivalent but it is clear that one sensor reports
 *    the data at much greater precision than another. To do this, the data is formatted in what is called IEEE 20601 Mder SFLOATs and FLOATs. In this case
 *    we are using two-byte SFLOATs. So we need to encode our three BP values into three s_MderFloat structs
 *
 *    Assume your sensor triggers the following method:
        void dataFromSensor(short int systolic, short int diastolic, short int mean, short int pulseRate)  // Data delivered from sensor
        {
            s_MderFloat mder[3];                    // One MderFloat struct for each BP component
            mder[0].mderFloatType = MDER_SFLOAT;    // We are using 2-byte SFLOATs verus 4-byte MDER_FLOAT type.
                                                    // An SFLOAT value can only have significant figures from -2048 to 2047 (12 bits).  a FLOAT has 24 bits.
            mder[0].specialValue = MDER_NUMBER;     // Just a regular number, not a NAN (Not a number) or a PINF (positive infinity) or NINF (negative infinity)
            mder[0].exponent = 0;                   // Exponent is 0. That means there is no decimal fraction. Precision is integer level 0.
                                                    // If we wanted to send a number like 123.5 mmHg, then we would need exponent -1 and mantissa 1235
            mder[0].mantissa = systolic;            // The manitissa is the integer, for example the above 102 -  if one set exponent to -1 and mantissa to 1020,
                                                    // the result would be 102.0  The final value = mantissa * 10 ** exponent
            mder[1].mderFloatType = MDER_SFLOAT;
            mder[1].specialValue = MDER_NUMBER;
            mder[1].exponent = 0;
            mder[1].mantissa = diastolic;

            mder[2].mderFloatType = MDER_SFLOAT;
            mder[2].specialValue = MDER_NUMBER;
            mder[2].exponent = 0;
            mder[2].mantissa = mean;
                                                    // Now to update the data array with the sensor data: First the blood pressure
            updateDataCompound(&msmtGroupBpData,    // Here is the structure containing the data array that the library generated
                               bp_index,            // This is the index to the BP measurement in the group
                               mder,                // A pointer to the array of s_MderFloat structs (it is expecting three since we configured it that way)
                               msmt_id++);          // The measurement instance number. Every GHS measurement sent during a connection must have
                                                    // a unique one of these even if we do not use it for referencing ib our case.
                                                    // Now for the pulse rate.
                                                    // Reuse the first element of the s_MderFloat structs above.
            mder[0].mderFloatType = MDER_SFLOAT;    // We are using 2-byte SFLOATs again
            mder[0].specialValue = MDER_NUMBER;     // Just a regular number, not a NAN (Not a number) or a PINF (positive infinity) or NINF (negative infinity)
            mder[0].exponent = 0;                   // Exponent is 0. That means there is no deciaml fraction. Precision is integer level 0.
            mder[0].mantissa = pulseRate;           // The heart rate value
                                                    // Now populate the pulse rate of the data array
            updateDataNumeric(&msmtGroupBpData,     // Here is the structure containing the data array
                              pr_index,             // Critical index to the pulse rate measurement in the group
                              &mder[0],             // The value to update
                              msmt_id++);           // The instance of this measurement.
                                                    // We can populate this group in any order. The actual order sent is determined by when we added the
                                                    // measurement to the group.
            // Now populate the time stamp
            updateTimeStampEpoch(&msmtGroupBpData,  // Here is the structure containing the data array
                                 epoch);            // The updated current epoch time counter
                                                    // In our example we have a fixed base epoch that we add the 'current ticks since program start' to.
                                                    // When we get the correct epoch value from the PHG we can adjust our base epoch accordingly.
            // There is one more step. We need to add the command given by the PHG that we are responding to. It can only be one of two possible values
            // Command to get stored data or command to get live data. That would be done as follows:
            msmtGroupBpData->data[0] = (current_command & 0xFF);
            msmtGroupBpData->data[1] = ((current_command >> 8) & 0xFF);
       }
 * The data array has been populated and is now ready to notify over the characteristic. The actual data array is 
       msmtGroupBpData->data

 * Now in practice most PHDs wont include all that configuration code. Instead, they can create and populate that s_MsmtGroupData struct, save the 
 * measurement indices and include only that in the code, storing it in ROM. One can go a step further and create just the data array and the indices
 * to the locations in the data array for updates and not have the update routines to contend with. It's a bit more work though.
 *
 * A similar approach is used to configure the the sGhsTime properties, the current time info and the system info. The system info is the simplesr
 * as it is static and never has to be updated.
 */

/**
 * Frees all resources allocated when creating/configuring the s_SystemInfo struct. Usually we call this method
 * after we have created the s_SystemInfoData array structure.
 * @param systemInfo a pointer to the s_SystemInfo pointer.
 */
void cleanUpSystemInfo(s_SystemInfo** systemInfo);

/**
 * Frees all resources allocated when creating the s_SystemInfoData array. Typically called whenthe program terminates.
 * @param systemInfoData a pointer to the s_SystemInfoData pointer.
 */
void cleanUpSystemInfoData(s_SystemInfoData** systemInfoData);

#if (USES_TIMESTAMP == 1)
    void cleanUpGhsTime(s_GhsTime** sGhsTime);
#endif
void cleanUpTimeInfo(s_TimeInfo** timeInfo);

/**
 * Call this at the end of the program. Frees up all the allocated resources use in creating the TimeInfo Data
 * @param timeInfoData pointer a pointer of the application's s_TimeInfoData struct.
 */
void cleanUpTimeInfoData(s_TimeInfoData** timeInfoData);

/**
 * This standard defines a current time and time stamp that contain a base epoch, a set of flags, an offset shift,
 * and a time sync. In binary form it is 10 bytes long. The set of flags indicate whether the resolution of the clock is
 * milliseconds or seconds. PHDs that require different resolutions cannot be supported by this standard (at the moment).

 * Most PHDs are not able to support knowledge of the Offset shift from UTC to local time as
 * this knowledge requires the PHD to link to some external time source which for cost reasons is not done. The drawback
 * is that such PHDs can only report measurements from a single time zone. However, since the epoch is UTC, a PHG can always
 * convert the epoch to a correct civil time in the time zone of the PHG. For example, if a measurement was taken two time zones
 * to the east at 14:00 in that time zone but uploaded in the current time zone the civil time would be reported at the corresponding
 * civil time in the current time zone which would be 12:00.
 * The dst/offsetShift are only changed if the PHD supports knowledge of these values. If the PHD does not have knowledge
 * of these values, special values are used to indicate the lack of support for the Offset. Such a PHD will always send its time
 * values with indicators showing that it does not know these values.
 * Since most PHDs do not support an independent means of synchronizing electronically to an external time source they either let the
 * user set the clock or the PHG. If the user sets the clock the PHD shall set the timeSync to MDC_TIME_SYNC_EBWW (eyeball and wrist
 * watch). If the PHG has previously set the time in an earlier connection, the PHD sets the time Sync to the value provided by the
 * PHG instead. If the PHD has not been synchronized, it sets the value to MDC_TIME_SYNC_NONE.
 * 
 * This method creates and populates the s_GhsTime structure with the initial properties to create and update the time fields in the
 * the current time and measurement time stamps.
 *
 * @param sGhsTime pointer to an s_GhsTime pointer. This method will allocate memory for the structure and populate it. It is important
 *        that the pointer itself is initialized to NULL the first time it is called. In other words
 *           s_GhsTime *sGhsTime = NULL;        // Set to NULL 
 *           createGhsTime(&sGhsTime, ...);     // Populate the structure
 *                some stuff
 *           createGhsTime(&sGhsTime, ...);     // Okay to reuse same variable - call will free if not NULL before repopulating
 *                some stuff
 *           cleanUpGhsTime(&sGhsTime);         // clean up allocated resources before program exit.
 *        All 'create' methods work as above.
 * @param offsetShift the current Offset to local time from UTC in units of 15 minutes IF the PHD is able to obtain this information.
 *        Allowable range is -12 to + 12. All else is taken as unsupported.
 *        If the PHD does not have the ability to obtain this information, set the value to GHS_TIME_OFFSET_UNSUPPORTED. If set to 
 *        GHS_TIME_OFFSET_UNSUPPORTED this is a fixed property of the PHD and cannot change (most PHDs will use GHS_TIME_OFFSET_UNSUPPORTED).
 *        Having the PHG or user set the offset is not considered support for the offset as they are sporadic. To support this feature the
 *        PHD must have the capability to get synchronization information on powerup.
 * @param clockType one of the following possible types given by
 *           GHS_TIME_FLAGS_RELATIVE_TIME the epoch 0 value has no meaning. The time is just a tick counter.
 *           GHS_TIME_FLAGS_EPOCH_TIME the epoch is based upon UTC. If the offset is supported this is equivalent to Base offset time.
 * @param clockResolution one of the following possible values
 *           GHS_TIME_FLAG_SUPPORTS_SECONDS       PHD supports seconds resolution
             GHS_TIME_FLAG_SUPPORTS_TENTHS        PHD supports tenths of seconds (not supported)
             GHS_TIME_FLAG_SUPPORTS_HUNDREDTHS    PHD support hundredths
             GHS_TIME_FLAG_SUPPORTS_MILLISECONDS  PHD supports milliseconds
             GHS_TIME_FLAG_SUPPORTS_TENTHS_MILLIS PHD supports tenths of milliseconds
 * @param timeSync the current state of timeSynchronization as 16-bit MDC code. This may be updated by a PHG set time.
 * @return true if all went well. false indicates bad input parameters or unable to allocate memory
 */
#if (USES_TIMESTAMP == 1)
    bool createGhsTime(s_GhsTime **sGhsTime, short offsetShift, unsigned short clockType, unsigned short clockResolution, unsigned short timeSync);
    bool setCurrentTimeLine(s_GhsTime **sGhsTimePtr, bool onCurrentTimeline);
#endif
/**
 * Sets up the time info for the device. Method allocates memory for the s_TimeInfo.
 * Only one time clock type can exist. It is also possible to support no time clock at all such as for continuous live measurements.
 * This method is called even when the PHD does not support time stamps or a time clock. The PHG will be told that when it gets
 * this info from the PHD.

 * @param timeInfo pointer to an s_TimeInfo *struct pointer. This method will populate it. The pointer must remain in scope
 * @param sGhsTime a pointer to s_GhsTime struct created by the createGhsTime method. Set to NULL if there are no time stamps in any measurements
 * @param wantSetTime true if PHD wants time to be set. Ignored otherwise
 * @returns true if there are no problems.
 */
bool createTimeInfo(s_TimeInfo **sTimeInfo, s_GhsTime *sGhsTime, bool wantSetTime);

/**
 * This method generates the time info byte array to be sent to the client when the client asks for it. The only thing that will
 * be missing is the actual current time which will need to be populated by calling one of the updateCurrentTime* methods.
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will allocate memory for it and populate it.
 *        The pointer must remain in scope.
 * @param timeInfo pointer to the sTimeInfo struct populated by the createTimeInfo method If the s_GhsTime struct of the
 *        s_TimeInfo struct is NULL, the s-TimeInfoData.timeInfoBuf data buffer will be NULL.
 * @returns true if there are no problems.
 */
bool createCurrentTimeDataBuffer(s_TimeInfoData** timeInfoDataPtr, s_TimeInfo *timeInfo);

/**
 * Updates the current time info data array from a PHG set time operation.
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will uodate it. The pointer must remain in scope
 * @param update will be a 10-byte array from the set time operation.
 */
 #if (USES_TIMESTAMP == 1)
    bool updateCurrentTimeFromSetTime(s_TimeInfoData** timeInfoDataPtr, unsigned char *update);
#endif
/**
 * Following methods update the current time info data array from the PHD (for example in response to a PHG get current time info command).
 * There are various methods depending upon which fields need to be updated.

 * This method updates the offset in the current time info data array. This method should only be used by PHDs that
 * independently are able to ascertain their offsets without relying on the user or PHG. Such PHDs are able to communicate
 * with an external server on power up to check their current time and time zone. This method provides a way to correct
 * that offset should it, for some reason, be wrong.
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will uodate it. The pointer must remain in scope
 * @param offsetShift the current timezone offset shift from UTC in 15-minute units. If not supported set to 0x80.
 */
bool updateCurrentTimeFromPhdOffset(s_TimeInfoData** timeInfoDataPtr, short offsetShift);

/**
 * This method updates the time sync. Usually this will happen from a set time but that is handled by calling
 * updateCurrentTimeFromSetTime
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will uodate it. The pointer must remain in scope
 * @param timeSync the time synchronization MDC term code from partition 8.
 */
bool updateCurrentTimeFromPhdTimeSync(s_TimeInfoData** timeInfoDataPtr, unsigned short timeSync);

/**
 * The option allows the application to update just the epcoh portion of the current time.
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will uodate it. The pointer must remain in scope
 * @param epoch the epoch value in units of milliseconds. If only seconds resolution is supported, the lowest three digits
 * are all zeros.
 */
bool updateCurrentTimeEpoch(s_TimeInfoData** timeInfoDataPtr, unsigned long long epoch);

unsigned short getTimeSync(s_TimeInfoData* timeInfoData);

/** Helper to load a string into bytes within an array
 * @param str input string
 * @param systemInfoBuf the byte array where you want to place the string
 * @param the index within the byte array to place the string
 * @return the index of the next place within the byte array to place further data.
 */
int loadString(char *str, unsigned char *systemInfoBuf, int index);

/**
 * This method configures the s_SystemInfo struct which is used to generate the systemInfo data array. The systemInfo pointer must
 * remain in scope until the data array is generated at which time it can be cleaned up. This structure is one of the more straight
 * forward as the information in it is static for the livetime of the device. However, it can have a fairly large set of possible
 * options, mostly just strings like firmware version, serial number, etc. One item that must be present is the device specializations
 * Usually just one like a blood pressure cuff. But some vital signs monitors might have several. All we set in the create routine
 * is the number of specializations which one adds in subsequent calls.
 *
 * It is not necessary to use these APIs to configure the s_SystemInfo structure. The application can populate the elements directly.
 *
 * @param systemInfo pointer to an s_SystemInfo *struct. This method will allocate memory for the struct and populate it with what is
 *        known currently.
 * @param numberOfSpecializations how many specializations this device supports - usually just one
 */
bool createSystemInfo(s_SystemInfo **systemInfo, unsigned short numberOfSpecializations);

/**
 * Method sets the system identifier as a byte array. It consists of an IEEE assigned 40-bit OUI and a 40-bit manufacture
 * assigned value. It can be generated from the public Bluetooth Address.
 * @param systemInfo pointer to an s_SystemInfo *struct. This method will allocate memory for the struct and populate it with what is
 *        known currently.
 * @param systemId the EUI-64 system Identifier as a little endian byte array - least significant byte first.
 */
bool setSystemIdentifierByte(s_SystemInfo **systemInfo, unsigned char *systemId);

/**
 * Adds a specialization supported by the device. One can only add the 'numberOfSpecializations' set in the create method.
 * @param systemInfo pointer to an s_SystemInfo *struct. This method will allocate memory for the struct and populate it with what is
 *        known currently.
 * @param specializationTermCode the specialization MDC term code from partition 8.
 * @param version the specialization version. Should probably make this a string. Just happens to be the 20601 version is always an
 *        integer.
 */
bool addSpecialization(s_SystemInfo **systemInfo, unsigned short specializationTermCode, unsigned short version);

/**
 * Adds the required fields of the system info. If there is nothing entered they will be sent as empty strings. That option
 * is frowned upon. Shall not be NULL.
 * @param systemInfo pointer to an s_SystemInfo *struct. This method will allocate memory for the struct and populate it with what is
 *        known currently.
 * @param manufacturerName the PHD's manufacturer name
 * @param modelNumber the PHD model number
 */
bool setRequiredSystemInfoStrings(s_SystemInfo **systemInfo, char *manufacturerName, char *modelNumber);

/**
 * Adds the optional string fields of the system info.
 * @param systemInfo pointer to an s_SystemInfo *struct. This method will allocate memory for the struct and populate it with what is
 *        known currently.
 * @param serialNumber the PHD's serial number - this is strongly desired
 * @param firmware the PHD firmware version set to NULL if not wanted.
 * @param hardware the PHD hardware version set to NULL if not wanted.
 * @param software the PHD software version set to NULL if not wanted.
 */
bool setOptionalSystemInfoStrings(s_SystemInfo **systemInfo, char *serialNumber, char *firmware, char *hardware, char *software);

/**
 * Adds the UDI to the system info. Any of the udi_* parameters can be NULL.
 * @param systemInfo: pointer to an s_SystemInfo *struct. This method will allocate memory for the struct and populate it with what is
 *        known currently.
 * @param udi_label: String value matching the UDI in human readable form as assigned
              to the product by a recognized UDI Issuer. Zero-terminated
 * @param udi_dev_id: A fixed portion of a UDI that identifies the labeler and the 
                      specific version or model of a device. Zero-terminated
 * @param udi_issuer_oid: OID representing the UDI Issuing Organization, such as GS1. Zero-terminated
 * @param udi_auth_oid: OID representing the regional UDI Authority, such as the US FDA. Zero-terminated.
 */
bool setUdi(s_SystemInfo **systemInfo, char* udi_label, char* udi_dev_id, char* udi_issuer_oid, char* udi_auth_oid);

/**
 *  The set value 'isRegulated' is only used if 'hasRegulationStatus' is true.
 */
bool setRegulationStatus(s_SystemInfo **systemInfoPtr, bool hasRegulationStatus, bool isRegulated);

/**
 * Sets the system identifier using a 16-digit HEX string. This method will put the system id in little endian order as bytes.
 */
bool setSystemIdentifier(s_SystemInfo **systemInfo, char *systemId);

#if (USES_AVAS == 1)
    bool initializeSystemInfoAvas(s_SystemInfo** systemInfo, unsigned short numberOfAvas);

    bool addSystemInfoAvas(s_SystemInfo** systemInfo, s_Avas *ava);
#endif


void cleanUpGhsMsmt(s_GhsMsmt** ghsMsmt);

void cleanUpMsmtGroup(s_MsmtGroup** msmtGroup);

void cleanUpMsmtGroupData(s_MsmtGroupData** msmtGroupData);

bool createMsmtGroup(s_MsmtGroup **msmtGroup, bool hasTimeStamp, unsigned char numberOfGhsMsmts);

bool setHeaderOptions(s_MsmtGroup **msmtGroup, bool areSettings, bool hasPersonId, unsigned short personId);

bool setHeaderSupplementalTypes(s_MsmtGroup **msmtGroup, unsigned short numberOfSupplementalTypes );

bool setHeaderRefs(s_MsmtGroup **msmtGroup, unsigned short numberOfRefs );

bool setHeaderDuration(s_MsmtGroup **msmtGroup);

#if (USES_AVAS == 1)
    bool initializeHeaderAvas(s_MsmtGroup **msmtGroup, unsigned short numberOfAvas);

    bool addHeaderAva(s_MsmtGroup** msmtGroup, s_Avas* avaIn);
#endif
short addGhsMsmtToGroup(s_GhsMsmt *ghsMsmt, s_MsmtGroup **msmtGroupPtr);

bool updateTimeStampEpoch(s_MsmtGroupData **msmtGroupDataPtr, unsigned long long epoch);
bool updateTimeStampTimeline(s_MsmtGroupData **msmtGroupDataPtr, unsigned char onCurrentTimelineFlag);
bool updateTimeStampTimeSync(s_MsmtGroupData **msmtGroupDataPtr, unsigned short timeSync);
bool updateTimeStampOffset(s_MsmtGroupData **msmtGroupDataPtr, short offsetShift);

bool updateDataHeaderSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, unsigned long suppType, unsigned suppType_index);
bool updateDataGhsMsmtSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long suppType, unsigned suppType_index);
bool updateDataHeaderRefs(s_MsmtGroupData** msmtGroupData, unsigned ref, unsigned ref_index);
bool updateDataGhsMsmtRefs(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned ref, unsigned ref_index);
bool updateDataHeaderDuration(s_MsmtGroupData** msmtGroupDataPtr, s_MderFloat *duration);
bool updateDataGhsMsmtDuration(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat *duration);
/**
 * This method 'removes' the last measurement in the given group. The measurement is still there but it will not get sent.
 * To restore the measurement so it will get sent, call updateDataRestoreLastMsmt(). These two methods are like popping and
 * pushing variables on the stack. So one can call this method twice to 'remove' the last and then next to last measurements.
 * Two calls to updateDataRestoreLastMsmt() to restore the two measurements.
 * The purpose of these two methods is to be able to have an occassionally used measurement like a device sensor status msmt
 * in the group and only use it when needed. It is up to the application to order the measurements in the group appropriately
 * to take advantage of this method. It is a more efficient than making two groups, one without the extra measurement and
 * one without.
 * @param msmtGroupDataPtr pointer to an s_msmtGroupData pointer of the group to 'pop' the measurement off.
 * @return true if the measurement is removed. false indicates that there is only one measurement left and it will
 *         not get removed.
 */
bool updateDataDropLastMsmt(s_MsmtGroupData** msmtGroupDataPtr);
/**
 * This method restores the last measurement in the given group. This method undoes what the updateDataDropLastMsmt() does.
 * @param msmtGroupDataPtr pointer to an s_msmtGroupData pointer of the group to 'pop' the measurement off.
 * @return true if the measurement is restored. false indicates that no measurement has been popped off to be restored.
 */
bool updateDataRestoreLastMsmt(s_MsmtGroupData** msmtGroupDataPtr);

#if(USES_NUMERIC == 1)
    bool createNumericMsmt(s_GhsMsmt **ghsMsmt, unsigned long type, bool isSfloat, unsigned short units, bool hasMsmtId);
    bool updateDataNumeric(s_MsmtGroupData** msmtGroupData, short msmtIndex, s_MderFloat* value, unsigned short msmt_id);
#endif
#if(USES_COMPOUND == 1)
    //bool createCompoundNumericMsmt(s_GhsMsmt** ghsMsmt, unsigned long type, bool isSfloat, unsigned short units, 
    //        unsigned short numberOfComponents, s_Compound *compounds, bool hasMsmtId);  // Classic compound
    bool createComplexCompoundNumericMsmt(s_GhsMsmt** ghsMsmt, unsigned long type, bool isSfloat, 
            unsigned short numberOfComponents, s_Compound *compounds, bool hasMsmtId);  // Complex compound
    bool updateDataCompound(s_MsmtGroupData** msmtGroupData, short msmtIndex, s_MderFloat* value, unsigned short msmt_id);
#endif
#if(USES_CODED == 1)
    bool createCodedMsmt(s_GhsMsmt** ghsMsmt, unsigned long type, bool hasMsmtId);

    bool updateDataCoded(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned long code, unsigned short msmt_id);
#endif
#if (USES_BITS == 1)
    bool createBitsEnumMsmt(s_GhsMsmt** ghsMsmt, unsigned long type, unsigned long state, unsigned long support, 
        unsigned char numberOfBytes, bool hasMsmtId);

    bool updateDataBits(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned long bits, unsigned short msmt_id);
#endif

#if (USES_RTSA == 1)
    /**
     * @param sampleSize number of bytes per sample
     */
    bool createRtsaMsmt(s_GhsMsmt** ghsMsmt, unsigned long type, unsigned short units, s_MderFloat* period, s_MderFloat* scaleFactor, s_MderFloat* offset,
        unsigned short numberOfSamples, unsigned char sampleSize, bool hasMsmtId);

    bool updateDataRtsa(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned char* samples, unsigned short dataLength, unsigned short msmt_id);
#endif

bool setGhsMsmtSupplementalTypes(s_GhsMsmt **ghsMsmt, unsigned short numberOfSupplementalTypes );

bool setGhsMsmtRefs(s_GhsMsmt **ghsMsmt, unsigned short numberOfRefs );

bool setGhsMsmtDuration(s_GhsMsmt **ghsMsmt);

#if (USES_AVAS == 1)
    bool initializeGhsMsmtAvas(s_GhsMsmt **ghsMsmt, unsigned short numberOfAvas);

    bool addGhsMsmtAva(s_GhsMsmt **ghsMsmt, s_Avas *ava);
#endif

bool createMsmtGroupDataArray(s_MsmtGroupData** msmtGroupData, s_MsmtGroup *msmtGroup, s_GhsTime *sGhsTime);

#endif  //CONFIG_GHS_ENCODER_H__
