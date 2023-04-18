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
#include "MetControlStructs.h"


/*
 * All this library does is create templates (byte arrays) that one sends to the peer. 
 * There are three differnt templates
 *    Current Time Info template
 *    System Info template
 *    Measurements template
 * The templates are defined in the Metric Packet Model Implementation Guide. The Current Time Info and System Info templates are
 * easy. The System Info template is easiest because it is static. You create it once and its good for the lifetime of the device.
 * The idea of the templates is that the device is likely to only do a certain set of things and that set of things is fixed. A thermometer is
 * not going to be taking BP measurements. The capabilities are also likely to remain the dame. So the static information is populated
 * ahead of time and the dynamic information is populated when data from the sensor arrives. 
 * So the library creates the templates based upon what the application supports. That's what all the config methods are for. When the
 * library generates the templates it keeps track of where in the template the values and other dynamic fields go. The application can call
 * the update methods to populate these values. Once all the values from the sensor data are filled in, the template can be notified/indicated
 * over the characteristic.
 *
 * There might be some cases where the application needs to create its own update methods. For example, the measurement type is
 * generally static. So that is populated once and there is no update method for it. But say you had a thermometer with two buttons,
 * one is pressed when taking an ear temperature and another is pressed when taking a forehead temperature. In the first case the
 * measurement type would be 'ear drum temperature' and the second might be the generic 'body temperature'. One would then need an
 * update method for the measurement type. That would require keeping track of the index of the measurement type when generating the
 * template and then making an update method for the type. The method would take the 4-byte MDC type code and use the type-index to
 * update the type field. That is all the other update methods do.
 
 * Below is a summary of what the packets contain.
        Current time info packet:
    command | flags | length    |         Current-Time             | AVAs
                                [epoch | flags | offset | Time-sync]

        System Info Packet:
    command | flags | length | System-id | [count | Specialization-code-n] | [length | Manufacturer-Name] |
        [length | Model-number] | Regulation-status | [length | Serial-number] | [length  firmware] | 
        [length | software] | [length | hardware] | length | UDI label | length | UDI Device Identifier | length | UDI issuer | length | UDI auth | AVAs]

        Measurements Packet:
    command | header | Measurement-1 | Measurement-2 | … | Measurement-n
    
        Header contents:
        flags | Length-of-PDU  |            Time-stamp            | Supplemental-types | references | duration | Person-id | AVAs | Group-id | Number-of-measurements |
                               [epoch | flags | offset | Time-sync]
        
        Measurement n contents:
        type | length | Measurement-flags | id | Measurement-Value | Supplemental-Types | reference | duration | AVAs

        The 7 Measurement-Value types:
        1. Numeric:        units | float

        2. Compound:       units | count | Sub-type-n | Sub-value-n

        3. Coded:          code

        4. BITs N*8:       N | BITsN*8-value | BITsN*8-type-mask | BITsN*8-support-mask     N = number of bytes for the BITs measurement. N*8 gives the number of bits.

        6. RTSA:           units | period | Scale-factor | offset | Sample-size | Number-of-samples | samples
        
        7. Cmplx Cmpd:     count | Sub-type-n | Sub-value-n | Sub-unit-n

 *
 * How to use this 'library'
 * A PHD consists of three basic parts
 *   1. Time and clock information - needed if the PHD stores data or reports time stamps in the measurements
 *   2. Information about the PHD itself, so-called system info; All PHDs have this
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
                                                    // a crash if it has not yet been allocated! NOTE THAT IT IS A POINTER to a POINTER that
                                                    // gets passed.
                                 true,              // If time stamps are being used set to true
                                 2,                 // The number of measurements in the group, in this case 2, blood pressure and pulse rate
                                 group_id++);       // The group id. This value is only used for optimization should you choose to use optimization
                                                    // but it needs to be valued and unique for the group in a connection. There is a maximum of 256 groups
                                                    // if you are using optimization. Otherwise the value is not used.

 *    Now we need to add measurements to that group. The blood pressure is more complicated because it is a compound or a vector. The systolic,
 *    diastolic, and mean components are treated as parts of a single measurement like the x, y, and z components of an acceleration. The reason
 *    for treating them as a vector was probably for efficiency and it now has become the norm and part of the BP standards. One could choose to
 *    send them as individal simple numerics (and some HL7 FHIR implementations treat them that way) and link them together. But now we follow the
 *    standard. To create this measurement we need to specify what our compound measurement contains. That is done by populating an array of s_Compound
 *    structs as follows:
        s_Compound compounds[3];            // We want three; one each for the systolic, diastolic, and mean components:
        compounds[0].subType = MDC_PRESS_BLD_NONINV_SYS;    // This is a 32-bit unsigned integer which gives the MDC code for systolic blood pressure
        compounds[1].subType = MDC_PRESS_BLD_NONINV_DIA;
        compounds[2].subType = MDC_PRESS_BLD_NONINV_MEAN;
 *    This structure also has elements for the value but those are not set yet because values will be populated when data is received from the sensor.
 *    Yes it is understood that you have to know what MDC codes are and that is not always a trivial task. Included with this library is a file nomenclature.h
 *    which contains a bunch of MDC codes that will cover 99% of the possible PHDs one can create. There is also an on line tool called the NIST Rosetta
 *    which can help though at this time it is still a work in progress and the nomenclature.h file is your better bet.
 *    The other element of this structure is the value. That we will set when we get data from the sensor. The subTypes can be set once as they will
 *    never change. So once we decide what is in our compound measurement, we create the compound measurement and then add it to our group:
        s_MetMsmt *bp = NULL;               // Initialize this pointer to NULL!
        result = createCompoundNumericMsmt(&bp,                     // The library will populate this structure. It will allocate memory for it
                                                                    // NOTE THAT IT IS A POINTER to a pointer that gets passed.
                                           MDC_PRESS_BLD_NONINV,    // This is the 32-bit MDC code that expresses what the overall compound measurement
                                                                    // is. In this case it is 'non-invasive blood pressure'.
                                           true,                    // 'true' If the values are encoded as two-byte Mder Floats versus 4-byte MderFloats
                                           MDC_DIM_MMHG,            // The units of the measurement. Complex compounds are compounds where the units are
                                                                    // different for each of the sub types. The s_Compound structure has sub units in it
                                                                    // but they are ignored in this case since BP is not a complex compound. There is a
                                                                    // different method to call should you want complex compounds.
                                           3,                       // The number of elements in the compound. In this case, there are three.
                                           compounds);              // A pointer to the array of s_Compound structs.
 *    For this implementation we want to tell the world that this blood pressure cuff is an upper arm cuff. Not many BP cuffs do that. In the Metric Model
 *    standard we can do this by including an extra MDC code in a field called 'supplemental types'. The Supplemental types allow one to say a lot of different
 *    things about the measurement, for example that it is an average, a maximum, that the sensor is placed on a certain location of the body, that a glucose
 *    measurement was taken by a tester and on the finger and it was bedtime, etc.
 *    To add a supplemental type to our measurement we call a method to set the number of supplemental types we intend to include. There are two possibilities;
 *    the supplemental types can be common to all measurements (part of the header) or just a single measurement. To add the supplemental types values we call 
 *    update methods on the data array once created. This allows us to add the supplemental types statically (just once and use always) or dynamically as data
 *    is received. In any case, we are going to add just one supplemental type to the BP measurement, not the header. We do that by calling
 *      setMetMsmtSupplementalTypes(&bp, 1). The '1' tells the library how many supplemental types we are going to have.
      This setting will eventually create space in the final data array to put one supplemental types value.
 *    Now we add this populated s_MetMsmt struct to the group:
        bp_index = addMetMsmtToGroup(bp,            // A pointer to the s_MetMsmt struct we want to add - in this case the bp
                                     &msmtGroup);   // A pointer to the measurement group we want to add the measurement to.
 *    Note that the above method returns a very critical bp_index. You have to keep this value as it tells where in the measurement group this particular
 *    measurement is. That parameter is needed for the update methods. We will discuss that later.
 *    Now lets create and add the pulse rate. The pulse rate is a simple numeric and is simpler to construct. The vast majority of PHD measurements are simple
 *    numerics.
        s_MetMsmt *pr = NULL;                               // Again, Initialize this pointer to NULL!
        result = createNumericMsmt(&pr,                     // A pointer to our s_MetMsmt pointer that will be populated by the library.
                                   MDC_PULS_RATE_NON_INV,   // MDC code for what this measurement is, in this case a non-invasive pulse rate. It tells 
                                                            // downstream readers that this value is not only a pulse rate but it was generated from
                                                            // a blood pressure cuff device.
                                   true,                    // 'true' If the values are encoded as two-byte Mder Floats versus 4-byte MderFloats
                                   MDC_DIM_BEAT_PER_MIN);   // The units of the pulse rate which is beats per minute
        pr_index = addMetMsmtToGroup(pr, &msmtGroup);       // Add the measurement to the group
 *    Note again the very important pr_index value. You need to save this!

 *    Now we have populated our structures for the blood pressure/pulse rate device. But structures are not sent on the wire. So the next step is to create
 *    the byte array that will be sent over the airwaves. Once we have created this byte array, we do not need the structs we have populated. All we need
 *    is the byte array struct and the indices of the measurements in the group. So now lets create the byte array struct. This struct cannot go out of scope!
 *    It must exist for the duration of the connection! The byte array element of this struct will be populated with sensor data and delivered to the PHG.
        s_MsmtGroupData *msmtGroupBpData                = NULL; // Defined as a global variable or in some manner that it will not go out of scope

        result = createMsmtGroupDataArray(&msmtGroupBpData,     // Pointer to the s_MsmtGroupData pointer initialized to NULL. The library will populate this
                                                                // structure which contains the byte array. It will allocate memory for it
                                                                // and attempt to free it if it is not NULL which may cause a crash if it has
                                                                // not been previously allocated. NOTE THAT IT IS A POINTER to a pointer that gets passed.
                                          msmtGroup,            // The measurement group to encode into a byte array
                                          sMetTime,             // A pointer to the s_MetTime struct containing the time properties of the PHD
                                                                // If a time stamp is not used, this parameter will be NULL.
                                          PACKET_TYPE_NORMAL);  // For now, use this. There is also an optimization option which we will not use.
                                                                // The optimization only makes sense if you are sending lots of data such as streams
 *    Now that we have populated this struct, we dont need the s_MsmtGroup struct anymore. We call the cleanup method to free the allocated resources. The
 *    cleanup method also frees the individual measurement structs like bp and pr as well.
         cleanUpMsmtGroup(&msmtGroup); // cleans up any allocated data -  we only need the data array now
      Now let's add the supplemental type value MDC_UPEXT_ARM_UPPER. This value is not going to change from measurement to measurement, so we can add it once
      and be done with it. We set this value by calling the update method for supplemental types
        updateDataMetMsmtSupplementalTypes(&msmtGroupBpData,     // The data array to be sent over the airwaves.
                                           bp_index,             // The blood pressure measurement index, returned when making the BP measurement
                                           MDC_UPEXT_ARM_UPPER,  // The supplemental type value which is upper arm
                                           0);                   // The supplemental types index. We reserved only one so the index is to the first which is 0.
 *    Note that all of this can happen at power up before any Bluetooth activity starts, so there is no performance penalty while connected. At this point, 
 *    the only resources left that will need to be freed later is the data array struct 'msmtGroupBpData' (also the time related structs).

 *    Here is the template array of bytes that the method createMsmtGroupDataArray() generated in the above case:

      0x00, 0x00, 0x01, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x1F, 0x01, 0x02,  // Header
      0x04, 0x4A, 0x02, 0x00, 0x1E, 0x00, 0x11, 0x01, 0x00, 0x00,                                                  // BP msmt
                      0x20, 0x0F, 0x03, 0x05, 0x4A, 0x02, 0x00, 0x00, 0x00,    // units, 3 entries, systolic
                                        0x06, 0x4A, 0x02, 0x00, 0x00, 0x00,    // diastolic
                                        0x07, 0x4A, 0x02, 0x00, 0x00, 0x00,    // mean
                      0x01, 0xF4, 0x06, 0x07, 0x00,                            // 1 supplemental type
      0x2A, 0x48, 0x02, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0xA0, 0x0A, 0x00, 0x00                           //  Pulse rate

    Breaking it down to details, the packet contains the following:
    0x00, 0x00,     // Here we place the command we are responding to (either getting stored data or sending live data)
    0x01, 0x00,     // Header flags. (Indicates that there is a time stamp)
    0x3E, 0x00,     // The length of the remaining packet data. This will not change unless we pop/push measurements onto the packet.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // the epoch count of the time stamp (will need to be populated)
    0x01,                   // the flags (supports milliseconds)
    0x80,                   // time zone (unsupported)
    0x00, 0x1F,             // time sync none (may need to be updated when there is a set time) 
    0x01,           // Group id (not used in our case so the value is not important)
    0x02,           // Number of measurements in group
    // BP measurement:
    0x04, 0x4A, 0x02, 0x00, // Measurement type: non-invasive blood pressure
    0x1E, 0x00,             // Measurement length
    0x11, 0x01,             // Measurement flags (compound, has supplemental types, uses SFLOATs)
    0x00, 0x00,             // measurement id (will need to be populated)
    0x20, 0x0F,             // units, mmHg
    0x03,                   // number of components in compound
    0x05, 0x4A, 0x02, 0x00,     // sub-type systolic
    0x00, 0x00,                 // systolic sub-value as Mder SFLOAT (will need to be populated)
    0x06, 0x4A, 0x02, 0x00,     // sub-type diastolic
    0x00, 0x00,                 // diastolic sub-value as Mder SFLOAT (will need to be populated)
    0x07, 0x4A, 0x02, 0x00,     // sub-type MEAN
    0x00, 0x00,                 // mean sub-value as Mder SFLOAT (will need to be populated)
    0x01,                       // number of supplemental types
    0xF4, 0x06, 0x07, 0x00,     // Supplemental type is upper arm location for cuff (static in this case) 
    // PR measurement:
    0x2A, 0x48, 0x02, 0x00, // Measurement type: non invasive pulse rate
    0x08, 0x00,             // Measurement length
    0x00, 0x01,             // Measurement flags (numeric, uses Mder SFLOATs)
    0x00, 0x00,             // measurement id (will need to be populated)
    0xA0, 0x0A,             // Units beats per minute
    0x00, 0x00,             // Pulse rate as Mder SFLOAT (will need to be populated)

    Here we see the DISADVANTAGE of a generic model. Those static type fields must be present even though they wont change for a given device
    type. However, they WILL change for a different device. So if the only device you make is a BP cuff, it is more efficient to use the
    BT_SIG BP profile and even more efficient to use a proprietary profile customized for your device. However, special code has to be written for
    just those devices and the code written is not good for anything else.

    Using the generic model means that clients need only be written once and they will work with this device and all devices following this standard.
    In addition, the codes used here are recognized by HL7 and IHE and puts your data immediately onto the international market.

    Furthermore. if you support more than one device type, this one code base will work for all your devices. One esentially gets all the other devices
    for free, at least with respect to code management and maintenance. This demo has setups for BP, Glucose, Pulse Ox, Thermometer, Spirometer, and Scale. 
    If one is really tight
    for resources these templates that are created in situ can be created externally, copied into this implementation and the part of the library
    source used for setup and configuring the templates can be deleted. There is a Visual Studio Project that does that. The ble_app_met_epoch_bp
    is a project where this was done for the blood pressure. One can compare the memory footprint of that implementation versus this implementation
    configured for the Blood Pressure. It turns out that the size savings is not very signficant. It was originally thought there would be a significant
    savings in size but that was not the case.


 * GETTING SENSOR DATA INTO THE TEMPLATE (byte array)
 * We assume that data comes from the sensor through some type of interrupt or event via a UART or SPI. There is no standard interface to a sensor.
 * Instead, the application defines a structure which the application populates with the sensor data. When the data is received, the application
 * passes this data into the provided msmt_queue. The void* along with the structure size can take any structure. The app casts back when dequeing the data.
 * The data is then retrieved from the queue and the application needs to take the data in its structure and call the appropriate update*() methods.
 * The work will be converting the data in the application-defined structure to the parameters required by the update methods. Likely the most difficult
 * part will be populating the MderFloat parameters since most people are not familiar with MderFloats. There are some BT SIG profiles that use MderFloats.

 * A similar approach is used to configure the the sMetTime properties, the current time info and the system info. The system info is the simplest
 * as it is required to be static in this standard and never has to be updated.
 */
//=========================================== METHODS ==========================================================
/**
 * Frees all resources allocated when creating/configuring the s_SystemInfo struct. Usually we call this method
 * after we have created the s_SystemInfoData array structure.
 * @param systemInfo a pointer to the s_SystemInfo pointer.
 */
void cleanUpSystemInfo(s_SystemInfo** systemInfo);

/**
 * Frees all resources allocated when creating the s_SystemInfoData array. Typically called when the program terminates.
 * @param systemInfoData a pointer to the s_SystemInfoData pointer.
 */
void cleanUpSystemInfoData(s_SystemInfoData** systemInfoData);

#if (USES_TIMESTAMP == 1)
    void cleanUpMetTime(s_MetTime** sMetTime);

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
 * This method creates and populates the s_MetTime structure with the initial properties to create and update the time fields in the
 * the current time and measurement time stamps.
 *
 * @param sMetTime pointer to an s_MetTime pointer. This method will allocate memory for the structure and populate it. It is important
 *        that the pointer itself is initialized to NULL the first time it is called. In other words
 *           s_MetTime *sMetTime = NULL;        // Set to NULL 
 *           createMetTime(&sMetTime, ...);     // Populate the structure
 *                some stuff
 *           createMetTime(&sMetTime, ...);     // Okay to reuse same variable - call will free if not NULL before repopulating
 *                some stuff
 *           cleanUpMetTime(&sMetTime);         // clean up allocated resources before program exit.
 *        All 'create' methods work as above.
 * @param offsetShift the current Offset to local time from UTC in units of 15 minutes IF the PHD is able to obtain this information.
 *        Allowable range is -12 to + 12. All else is taken as unsupported.
 *        If the PHD does not have the ability to obtain this information, set the value to MET_TIME_OFFSET_UNSUPPORTED. If set to 
 *        MET_TIME_OFFSET_UNSUPPORTED this is a fixed property of the PHD and cannot change (most PHDs will use MET_TIME_OFFSET_UNSUPPORTED).
 *        Having the PHG or user set the offset is not considered support for the offset as they are sporadic. To support this feature the
 *        PHD must have the capability to get synchronization information on powerup.
 * @param clockType one of the following possible types given by
 *           MET_TIME_FLAGS_RELATIVE_TIME the epoch 0 value has no meaning. The time is just a tick counter.
 *           MET_TIME_FLAGS_EPOCH_TIME the epoch is based upon UTC. If the offset is supported this is equivalent to Base offset time.
 * @param clockResolution one of the following possible values
 *           MET_TIME_FLAG_SUPPORTS_SECONDS       PHD supports seconds resolution
             MET_TIME_FLAG_SUPPORTS_TENTHS        PHD supports tenths of seconds (not supported)
             MET_TIME_FLAG_SUPPORTS_HUNDREDTHS    PHD support hundredths
             MET_TIME_FLAG_SUPPORTS_MILLISECONDS  PHD supports milliseconds
             MET_TIME_FLAG_SUPPORTS_TENTHS_MILLIS PHD supports tenths of milliseconds
 * @param timeSync the current state of timeSynchronization as 16-bit MDC code. This may be updated by a PHG set time.
 * @return true if all went well. false indicates bad input parameters or unable to allocate memory
 */
    bool createMetTime(s_MetTime **sMetTime, short offsetShift, unsigned short clockType, unsigned short clockResolution, unsigned short timeSync);

/**
 * Sets up the time info for the device. Method allocates memory for the s_TimeInfo.
 * Only one time clock type can exist. It is also possible to support no time clock at all such as for continuous live measurements.

 * @param timeInfo pointer to an s_TimeInfo *struct pointer. This method will populate it. The pointer must remain in scope
 * @param sMetTime a pointer to s_MetTime struct created by the createMetTime method. Set to NULL if there are no time stamps in any measurements
 * @param allowSetTime true if PHD clock can be set. Applies only to absolute and base offset times. Ignored otherwise
 * @returns true if there are no problems.
 */
    bool createTimeInfo(s_TimeInfo **sTimeInfo, s_MetTime *sMetTime, bool allowSetTime);

/**
 * This method generates the time info byte array to be sent to the client when the client asks for it. The only thing that will
 * be missing is the actual current time which will need to be populated by calling one of the updateCurrentTime* methods.
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will allocate memory for it and populate it.
 *        The pointer must remain in scope.
 * @param timeInfo pointer to the sTimeInfo struct populated by the createTimeInfo method.
 * @returns true if there are no problems.
 */
    bool createCurrentTimeDataBuffer(s_TimeInfoData** timeInfoDataPtr, s_TimeInfo *timeInfo);

/**
 * Updates the current time info data array from a PHG set time operation.
 * @param timeInfoDataPtr pointer to an s_TimeInfoData *struct. This method will uodate it. The pointer must remain in scope
 * @param update will be a 10-byte array from the set time operation.
 */

    bool updateCurrentTimeFromSetTime(s_TimeInfoData** timeInfoDataPtr, unsigned char *update);

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
#endif
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
 * forward as the information in it is static for the lifetime of the device. However, it can have a fairly large set of possible
 * options, mostly just strings like firmware version, serial number, etc. One item that must be present is the device specializations
 * Usually just one like a blood pressure cuff. But some vital signs monitors might have several. All we set in the create routine
 * is the number of specializations. The specializations themselves are set using an 'add' method after creating the SystemInfo packet.
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

#if (USES_SYSTEM_AVAS == 1)
    bool initializeSystemInfoAvas(s_SystemInfo** systemInfo, unsigned short numberOfAvas);

    bool addSystemInfoAvas(s_SystemInfo** systemInfo, s_Avas *ava);
#endif

/**
 * This method creates the systemInfo data array to be sent to the client from the info configured into the s_SystemInfo
 * struct. It also populates some support information (in this case only the total length of the data array as this
 * array is static and there is no need to update or change any values in the data array.
 * Once the data array struct is created, one can free the systemInfo struct. It is no longer needed.
 */
bool createSystemInfoData(s_SystemInfoData** systemInfoData, s_SystemInfo *systemInfo);


void cleanUpMetMsmt(s_MetMsmt** metMsmt);

void cleanUpMsmtGroup(s_MsmtGroup** msmtGroup);

void cleanUpMsmtGroupData(s_MsmtGroupData** msmtGroupData);

bool createMsmtGroup(s_MsmtGroup **msmtGroup, bool hasTimeStamp, unsigned char numberOfMetMsmts, unsigned char groupId);

bool setHeaderOptions(s_MsmtGroup **msmtGroup, bool areSettings, bool hasPersonId, unsigned short personId);

bool setHeaderSupplementalTypes(s_MsmtGroup **msmtGroup, unsigned short numberOfSupplementalTypes );

bool setHeaderRefs(s_MsmtGroup **msmtGroup, unsigned short numberOfRefs );

bool setHeaderDuration(s_MsmtGroup **msmtGroup);

#if (USES_HEADER_AVAS == 1)
    bool initializeHeaderAvas(s_MsmtGroup **msmtGroup, unsigned short numberOfAvas);

    bool addHeaderAva(s_MsmtGroup** msmtGroup, s_Avas* avaIn);
#endif
short addMetMsmtToGroup(s_MetMsmt *metMsmt, s_MsmtGroup **msmtGroupPtr);

bool updateTimeStampEpoch(s_MsmtGroupData **msmtGroupDataPtr, unsigned long long epoch);
bool updateTimeStampTimeline(s_MsmtGroupData **msmtGroupDataPtr, unsigned char unknownTimelineFlag);
bool updateTimeStampTimeSync(s_MsmtGroupData **msmtGroupDataPtr, unsigned short timeSync);
bool updateTimeStampOffset(s_MsmtGroupData **msmtGroupDataPtr, short offsetShift);
bool updateTimeStampFlags(s_MsmtGroupData **msmtGroupDataPtr, unsigned short newflags);

bool updateDataHeaderSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, unsigned long suppType, unsigned suppType_index);
bool updateDataMetMsmtSupplementalTypes(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, unsigned long suppType, unsigned suppType_index);
bool updateDataHeaderRefs(s_MsmtGroupData** msmtGroupData, unsigned short ref, unsigned ref_index);
bool updateDataMetMsmtRefs(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned short ref, unsigned ref_index);
bool updateDataHeaderDuration(s_MsmtGroupData** msmtGroupDataPtr, s_MderFloat *duration);
bool updateDataMetMsmtDuration(s_MsmtGroupData** msmtGroupDataPtr, short msmtIndex, s_MderFloat *duration);
/**
 * This method 'removes' the last measurement in the given group. The measurement is still there but it will not get sent.
 * To restore the measurement so it will get sent, call updateDataRestoreLastMsmt(). These two methods are like popping and
 * pushing variables on the stack. So one can call this method twice to 'remove' the last and then next to last measurements.
 * Making Two calls to updateDataRestoreLastMsmt() will 'restore' the two measurements.
 * The purpose of these two methods is to be able to have an occassionally used measurement like a device sensor status msmt
 * in the group and only use it when needed. It is up to the application to order the measurements in the group appropriately
 * to take advantage of this method. It is a more efficient than making two groups, one without the extra measurement and
 * one with.
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

/**
 * Helper method for Dropping and Restoring measurements
 * This method tells the current state of the measurement stack given the above Drop and Restore methods. It returns
 * the number of measurements dropped. If 0, no measurements have been dropped. If -1, the measurement group has not
 * been initialized and that is an error condition on the use of this method. The first measurement in the group will
 * not be dropped. It is assumed the user knows how many measurements are in the group.
 * @param msmtGroupDataPtr pointer to an s_msmtGroupData pointer of the group to 'pop' the measurement off.
 * @return true if the measurement is restored. false indicates that no measurement has been popped off to be restored.
 */
short getNumberOfMeasurementsDropped(s_MsmtGroupData** msmtGroupDataPtr);

#if(USES_NUMERIC == 1)
    bool createNumericMsmt(s_MetMsmt **metMsmt, unsigned long type, bool isSfloat, unsigned short units);
    bool updateDataNumeric(s_MsmtGroupData** msmtGroupData, short msmtIndex, s_MderFloat* value, unsigned short msmt_id);
#endif
#if(USES_COMPOUND == 1)
    bool createCompoundNumericMsmt(s_MetMsmt** metMsmt, unsigned long type, bool isSfloat, unsigned short units, 
            unsigned short numberOfComponents, s_Compound *compounds);  // Classic compound
    bool createComplexCompoundNumericMsmt(s_MetMsmt** metMsmt, unsigned long type, bool isSfloat, 
            unsigned short numberOfComponents, s_Compound *compounds);  // Complex compound
    bool updateDataCompound(s_MsmtGroupData** msmtGroupData, short msmtIndex, s_MderFloat* value, unsigned short msmt_id);
#endif
#if(USES_CODED == 1)
    bool createCodedMsmt(s_MetMsmt** metMsmt, unsigned long type);

    bool updateDataCoded(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned long code, unsigned short msmt_id);
#endif
#if (USES_BITS == 1)
    bool createBitEnumMsmt(s_MetMsmt** metMsmt, unsigned short byteCount, unsigned long type, unsigned long state, unsigned long support);

    bool updateDataBits(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned long bits, unsigned short msmt_id);
#endif

#if (USES_RTSA == 1)
    /**
     * @param sampleSize number of bytes per sample
     */
    bool createRtsaMsmt(s_MetMsmt** metMsmt, unsigned long type, unsigned short units, s_MderFloat* period, s_MderFloat* scaleFactor, s_MderFloat* offset,
        unsigned short numberOfSamples, unsigned char sampleSize);

    bool updateDataRtsa(s_MsmtGroupData** msmtGroupData, short msmtIndex, unsigned char* samples, unsigned short dataLength, unsigned short msmt_id);
#endif

bool setMetMsmtSupplementalTypes(s_MetMsmt **metMsmt, unsigned short numberOfSupplementalTypes );

bool setMetMsmtRefs(s_MetMsmt **metMsmt, unsigned short numberOfRefs );

bool setMetMsmtDuration(s_MetMsmt **metMsmt);

#if (USES_MSMT_AVAS == 1)
    bool initializeMetMsmtAvas(s_MetMsmt **metMsmt, unsigned short numberOfAvas);

    bool addMetMsmtAva(s_MetMsmt **metMsmt, s_Avas *ava);
#endif

bool createMsmtGroupDataArray(s_MsmtGroupData** msmtGroupData, s_MsmtGroup *msmtGroup, s_MetTime *sMetTime, unsigned short packetType);

#endif  //CONFIG_GHS_ENCODER_H__
