# Nordic-MPM-Health-Devices
Support for the Metric Packet Model (MPM) Health Devices on Nordic platforms

(At this time users are referred to the upcoming BT-SIG GHSS, GHSP and ETS standards for references on the GHS)

This respository contains code that runs on the Nordic nRF52840 and nRF51 DKs. The code that runs on the nRF52840 DK should also run without issue on the nRF52 DK though it has not been tested.

Unlike the existing Bluetooth SIG health device profiles, the Metric Packet Model is generic and all health devices are supported by a single specification. The MPM is based upon the IEEE 11073 20601 Domain Information Model Objects though in a considerably simplified form. It is not necessary to understand the 20601 standard to work with the MPM. However, use of the MPM requires the understanding of the Medical Device Code (MDC) system and Mder Floats. The use of codes is totally foreign to the BT-SIG profiles but some of the profiles do use Mder Floats. Mder Floats are used to preserve precision.

The Bluetooth implementation is about as simple as it can get. GATT is used simply as a tunnel. The client sends commands to the server in one characteristic and the server notifies responses through a second characteristic. The exchange is completely synchronous. The endpoints only need to support descriptor and characteristic writes and indications and notifications. There is no need for the server to persist data in its service data tables but the Nordic SoftDevice will do that anyways. Only SoftDevice is used for the Bluetooth.

The bulk of the code work is encoding and decoding the packets which requires no platform-specific APIs and can be done with standard C on any platform. The platform independence of the packet work means this code would be quite simple to port to different BLE MCUs. In fact, this MPM prototype could be supported by any reliable transport, for example, USB.

## Repository Contents
The Nordic SDKs for nRF52 and nRF51 can be freely downloaded from https://www.nordicsemi.com/Products/Development-software/nRF5-SDK/Download#infotabs. This repository only contains code that is meant to be inserted into the nRF5_SDK_17+\examples\ble_peripheral or nrf_SDK_12.3.0\examples\ble_peripheral directory. Projects have been made for Segger Embedded Studio (which is free for development on Nordic platforms) and Keil. For the nRF51 projects only Keil projects are provided. However, the nRF51 project builds are small enough that one can use the size-limited free version for most of the specializations (you might have to set a more limited log level).

The following describes the Metric Packet Model prototype:

# Metric Packet Model Implementation Guide

This guide specifies how one implements personal health devices and gateways that use the Metric Packet Model (MPM) prototype to transfer data. The MPM defines a set of packets that are requested from the gateway in an ordered, synchronous manner. Any reliable transport can be used to transfer these packets, but this guide focuses on the use of the popular Bluetooth Low Energy GATT protocol. Two GATT characteristics are used, one for the gateway to send commands to the Personal Health Device (PHD), and a second for the PHD to send the requested MPM packets to gateway. The amount of Bluetooth knowledge needed to implement this tunnel is minimal.

This code already provides a pre-configured Blood Pressure, Glucose Monitor, Thermometer, Pulse Oximeter, Weight Scale, and Spirometer following the IEEE 104** specialization Device Information Models. One can specify which one of the devices is simulated by setting define constants in handleSpecializations.h. All the data is generated. An actual device would need to feed sensor data to this application via SPI or UART. The generation of fake data would need to be disabled.

## **Packet Model**

The packets are based upon the IEEE 11073 20601 Domain Information Model (DIM) Medical Device System (MDS) and Metric Objects and their attributes. It is not necessary to understand the DIM or 20601 to implement the MPM. However, those familiar with the 20601 standard will recognize the borrowed concepts.

The MPM defines four packet templates that contain the information sent from the PHD to the gateway. Only three have been defined.

- Current time and clock information packet (current time and clock properties)
- System Information packet (static information about the device like the serial and model number)
- *Configuration Information packet (optional and currently not defined)*
- Metric Data packet (measurement data)

The Metric Data packet defines the following templates for the supported measurement value types

- Simple numeric quantity (pulse rate, temperature, miles run, weight, glucose concentration, etc.)
- Compound numeric quantity (systolic, diastolic, and mean blood pressure, x, y, z acceleration components)
- Code (limited set of enumerated values; run, walk, bike, etc.)
- BITs (a set of potentially simultaneous events or states)
- Periodic waveforms (ecg trace, pleth wave, spirometric flow data)

Each packet is a &#39;template&#39;. IEEE 11073 10101 nomenclature codes are used to interpret the semantic meaning of the items within the template. Nomenclature codes are 32-bit unsigned numbers. Since 32-bit numbers are hard to remember, reference identifiers are used to indicate what those numbers mean for human convenience. Reference identifiers are not used on the wire in this packet model. An example of a reference identifier and the 32-bit code it corresponds to is MDC\_TEMP\_ORAL (188424).

The template allows an implementer to write code once for each template and it will handle all PHD types now and in the future that use that template. Recall the Reference identifiers are just 32-bit numbers. The unit code reference identifiers are 16-bit numbers.

### Example

Let&#39;s illustrate how this packet model would send simple numeric quantities

Generic simple numeric template:

| type | length | Measurement flags | id | units | value |
| --- | --- | --- | --- | --- | --- |

Glucose concentration in milligrams/deciliter:

| MDC\_CONC\_GLU\_CAPPILARY\_PLASMA | length | Measurement flags | 1 | MDC\_DIM\_MILLI\_G\_PER\_DL | 101.4 |
| --- | --- | --- | --- | --- | --- |

Oral temperature in degrees Celcius

| MDC\_TEMP\_ORAL | length | Measurement flags | 2 | MDC\_DIM\_DEG\_C | 36.7 |
| --- | --- | --- | --- | --- | --- |

Body weight in pounds

| MDC\_MASS\_BODY\_ACTUAL | length | Measurement flags | 3 | MDC\_DIM\_LB | 160.2 |
| --- | --- | --- | --- | --- | --- |

Heart rate in beats per minute

| MDC\_ECG\_HEART\_RATE | length | Measurement flags | 6 | MDC\_DIM\_BEAT\_PER\_MINUTE | 46 |
| --- | --- | --- | --- | --- | --- |

It should be noted that the above packets are not the complete simple numeric packet, but they illustrate how the template is the same for each simple numeric measurement type, and thus a decoder can handle all simple numeric quantities with the same code.

## **PHD-Gateway Exchange Sequence**

The exchange between the gateway client and PHD is steered from the client and is _synchronous and ordered._ The client sends commands to get the MPM packets in a specified order. The client cannot send another command until the server indicates it is done with the previous command.

- Request current time and clock information
  - PHD sends current time and clock information packet – may indicate no time support
  - PHD indicates it is done with the task
- Request to set the PHD time if needed and supported by the PHD
  - PHD updates its time clock and updates the time stamp of all unsent measurements to the new timeline (there is no date-time-adjustment in the MPM)
  - PHD indicates it is done with the task
- Request System Information (the static MDS data - if already known the gateway may skip the request)
  - PHD sends system information
  - PHD indicates it is done with the task
- Request Configuration Information (optional and not yet designed)
- Request number of Stored Records if desired
  - PHD sends number of stored records\*
  - PHD indicates it is done with the task (\*the number is sent with the done indication)
- Request transfer of all Stored Records if desired (other options are in the works)
  - For each stored record
    - PHD sends stored record
    - PHD indicates it is done with the record
  - PHD indicates it is done with the task (all records have been sent)
- Request deletion of all Stored Records if desired
  - PHD deletes stored records
  - PHD indicates it is done with the task
- Send Live Data (this is the last request the gateway can make)
  - For each live measurement taken
    - PHD send record
    - PHD indicates it is done with the record
  - PHD disconnects when it has nothing more to send

The time information and any set time actions shall be requested before any request to transfer measurement data, stored or otherwise. A PHD that only sends live data does not need to support a time clock and does not need to include time stamps in the measurement data. The time of reception by the gateway is taken as the time stamp. An example of data that is often sent live without timestamps is streamed data from a pulse oximeter.

### Bluetooth Specific Addition

One of the complex tasks for the user of Bluetooth devices is pairing. To simplify operations for the user and to keep the behavior synchronous, before the gateway sends the first command, pairing is handled. The Bluetooth PHD includes in its advertisement whether it requires pairing. If the PHD requires pairing, it shall also secure the MPM Control Point and MPM Response Characteristic Descriptors. The PHD shall NOT send a security request.

On platforms that give the application control over pairing (for example, Android) if pairing is required, the application can invoke pairing before enabling the descriptors of the MPM Control Point and MPM Response Characteristic.

On platforms that do not give the application control over pairing (for example iOS) if the PHD requires pairing, the gateway will know that pairing will be initiated when it attempts to enable the MPM Control Point or MPM Response descriptors.

In both cases, the gateway knows when pairing will occur. The sensor also knows it will not get a pairing request while it is handling some other exchange.

## **MDC Nomenclature Codes**

The use of MDC nomenclature codes is ubiquitous in the MPM and it is important to understand them. Every nomenclature code is 32 bits, but the upper 16-bits is referred to as the _partition_. The lower 16-bits is the _term code_. The sets of codes are divided into partitions where each partition groups codes with a similar purpose. For example, partition 4 contains all the codes related to units. Thus, if the context is such that you know the code is referring to a unit, one can use only the term code. The MPM will take advantage of such context knowledge when it can to reduce packet size.

The reference identifier always refers to the 32-bit value of the code. One will not have the same reference identifier in two different partitions. Recall that reference identifiers are primarily for human convenience, as it is much easier to know what MDC\_TEMP\_ORAL means versus 188424. However, on the wire, the code takes up only 4 bytes and is fixed in length, whereas the corresponding reference identifier would take far more bytes and reference identifiers are not fixed in length.

Though there are thousands of term codes, at this time there are only a few partitions defined. They are shown below:

| **Reference Identifier** | **Value** | **Description** |
| --- | --- | --- |
| MDC\_PART\_OBJ | 1 | Object partition contains 20601 object and attribute definition codes |
| MDC\_PART\_SCADA | 2 | &#39;Supervisory control and data acquisition&#39; partition. Contains many measurement type codes |
| MDC\_PART\_EVT | 3 | Event partition. Not used in MPM PHDs (yet) |
| MDC\_PART\_DIM | 4 | Dimension partition. Contains the codes for units. |
| MDC\_PART\_SITES | 7 | Body site codes. |
| MDC\_PART\_INFRA | 8 | Infrastructure: Contains codes for specializations |
| MDC\_PART\_PHD\_DM | 128 | Disease management codes. Here we have a lot of diabetes, sleep apnea, etc. related codes. |
| MDC\_PART\_PHD\_HF | 129 | Health and Fitness related codes |
| MDC\_PART\_PHD\_AI | 130 | Assisted and Independent Living related codes. |

The use of codes is what makes the MPM generic. The only device protocol that uses codes at the current time is the IEEE 11073 20601 and 10201 standards. Codes are totally absent in the BT-SIG profiles, however, codes are ubquitous in HL7 and IHE. The MDC codes are used versus LOINC or SNOMED since MDC codes are designed for medical devices.

# **Handling Measurement Time Stamps**

Handling time is likely the most complicated aspect of PHDs. Cost requirements are going to make it impractical for most PHDs to be externally synchronized to an NTP time source that provides both UTC time and local offset. Most PHDs will either not provide a timestamp with the measurement or assume a fixed time zone approach. Synchronization (setting) of such clocks will rely either on the user through a UI or the gateway via a set time operation or both. PHDs that support some type of continuously monitored connection to an external network that has access to time synchronization and thus has knowledge of its global location and time zone will be the exception.

For that reason, this MPM standard will require that the PHD provide additional information with its time stamps (should it support time stamps) so the gateway will be able to handle these time stamps and align them to UTC plus offset. When we refer to &#39;time stamp&#39; or &#39;current time&#39; in this document, it will contain not only the time, but the meta data associated with that time. These features will be described as their implementation is discussed.

This type of time stamp is the only place where the MPM differs from the 20601 DIM. There is no analogous time attribute in 20601 for the &#39;time stamp&#39; used here.

# **Bluetooth Requirements**

## Advertisements

The advertisement and Scan Response shall contain at least the following three fields:

- Service UUIDs (all or partial) – must contain the MPM service UUID.
- Service Data
  - contains the MDC term code(s) for the specialization(s) supported – usually just one
  - also contains a trailing byte indicating pairing requirements for the device. The value 1 indicates pairing/bonding is required, 0 means do not pair.
- Friendly name (shortened or full)

We are hoping for a 16-bit UUID which, for now, would is 0xF990. The service data contains the UUID of the service followed by the data. In this case the data is a list of 16-bit MDC terms codes from partition 8 giving the specializations supported, for example 0x1004 would be pulse oximeter.

The PHD shall support responding to a scan request.

## Metric Packet Model Service

The Metric Packet Model Service is given by:

- UUID: 0000F990-0000-1000-8000-00805f9B34FB

The service contains two characteristics:

- The MPM Control Point is writable by the gateway and supports PHD indications.
  - UUID: 0000F991-0000-1000-8000-00805f9B34FB
- The MPM Response Characteristic supports PHD notifications.
  - UUID: 0000F992-0000-1000-8000-00805f9B34FB

A MPM gateway and PHD do not need to support the read operation. Therefore, a PHD does not need to update the MPM characteristic values in its service table database.

The MPM makes no use of any other existing GATT services or profiles. All the &#39;intelligence&#39; and information are contained within the data packets exchanged via the control point and the response characteristics and is thus handled at the application layer and not the transport layer. There is no semantic information obtained from the UUIDs of the characteristics.

The minimal use of the GATT structures and transactions reduces the number of platform-dependent calls, via a platform-dependent SDK or otherwise, implementations must make. This reduction in platform dependent code elements makes an MPM implementation easier to port not only between platforms but between different versions of a given platform. It also requires far less expertise in the Bluetooth GATT Specification to implement a MPM device versus a BT SIG profile/service-based device.

# **Message Packet Formats**

The MPM defines the following packet types:

- Current Time Info
- System Info
- *Config Info* (not defined yet)
- Measurement Records

## **Motivation Behind the Packet Formats**

The modeling of these packets is based upon the 20601 DIM MDS and Metric Objects and some of their attributes although in a simplified form.

In 20601, attributes are binary. They are defined by self-describing, type-length-value, ASN.1 Attribute Value Assertion (AVA) structs. An AVA contains an id entry whose value is an MDC code defining what the attribute is, followed by a length field giving the length of the value. The value could be a simple scalar or itself a struct. Decoding an AVA requires knowledge of the ASN.1 struct for that attribute. In this manner a decoder can look at the id field, and if it does not understand that attribute, it can use the length field to skip to the next AVA; so-called parse-and-ignore.

_At this point with AVAs and ASN.1, the MPM is beginning to sound daunting_. However, it turns out that only a small subset of all the attributes defined in 20601 are needed for the most common use cases. For example, the MPM measurement templates used in market PHDs are described for the most part with the following attributes (shown in square brackets[]):

- **Numeric** : [Type] [Time-Stamp][Unit-Code][Simple-Nu-Observed-Value]
- **Compound Numeric** : [Type] [Time-Stamp][Unit-Code][Metric-Id-List][Compound-Simple-Nu-Observed-Value]
- **Coded** : [Type] [Time-Stamp][Enum-Observed-Value-Simple-OID]
- **BITs** : [Type] [Time-Stamp][Enum-Observed-Value-Simple-Bit-Str][Capability-Mask-Simple][State-Flag-Simple]
- **RTSA** : [Type] [Time-Stamp][Sample-Period][Scale-And-Range-Specification][Sa-Specification][Unit-Code][Simple-Sa-Observed-Value]
- **String** : [Type] [Time-Stamp][Enum-Observed-Value-Simple-Str]

Three additional attributes [Supplemental-Types], [Observation-Reference-List], and [Measure-Active-Period] are occasionally used in all measurement templates. That makes a total of 18. This set of attributes accounts for everything that exists in the market today as well as specializations defined but not yet available in the market.

The MPM PHD could send a numeric measurement by sending the bytes representing the sequence of attributes [Type] [Time-Stamp][Unit-Code][Simple-Nu-Observed-Value] to the PHG. Since the attributes are self-describing, the four attributes could be sent in any order and the PHG could still decode the measurement.

So, one possible solution would be to represent all MPM packets using AVA structs, much like the variable format observation scans in 20601. However, understanding AVAs and ASN.1 is not common knowledge to most programmers. In addition, the overhead from the type and length fields of the AVA can double the size of some message packets. Since most measurements consist of just a few attributes, and those few attributes are used repeatedly, the common AVAs can be simplified by using a &#39;fixed-format&#39; representation. We illustrate:

We begin with the sequence of four AVA structs:

| Type | Time-Stamp | Unit-Code | Simple-Nu-Observed-Value |
| --- | --- | --- | --- |

Which fully expanded is as follows:

| Type attr-id | Length | nomenclature code | Time-Stamp attr-id | length | 10-byte time stamp field\* | Unit-Code attr-id | length | nomenclature code | Simple-Nu-Observed-ValueAttr-id | length | FLOAT |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

_\*In 20601 the absolute time is 8 bytes and is of civil time format._

where the attribute id is the four-byte nomenclature code for that attribute, and the length field is two bytes. To make the expansion above, we had to know what the ASN.1 struct for each of the attributes is. The fixed-format form of the above would contain only the value fields and would appear as follows:

| Type nomenclature code | 10-byte Time-Stamp field | Units nomenclature code | FLOAT |
| --- | --- | --- | --- |

The attribute ID and length fields have been removed which shortens the packet by 24 bytes. In addition, the ASN.1 decoding has been implicitly done.

This simplification comes at a cost. The order of the packet is specified and fixed and must be known by the implementer. The first four bytes is always the 32-bit nomenclature code for the measurement type, the next 10 bytes is always the time stamp, the next four bytes is always the 32-bit nomenclature code for the units, and the next four bytes is always the four-byte Mder FLOAT giving the value. An additional cost is that we have lost flexibility. Every measurement following the above must have all four fields.

### Fixed-Format, Flags, and AVA Extensions

To retain both flexibility, extensibility, and still reap the benefits of the reduced packet size and dealing with ASN.1 structs by using fixed format, the following is done:

- A bit in a flags field is used to define which of the fixed format fields is present. The flag allows one to drop a field if it is not needed. Some fields, like the type, are required in every packet.
- AVA structs may be appended to the end of the packet when needed. A bit in the flags field is used to indicate the presence of appended AVA structs. This need is so rare, that at the current time there are no market PHDs that would need an appended AVA struct.
- A length field is added for consistency checks (standard for most binary protocols).
- The first field in every packet is the command given by the gateway to obtain this packet.

This approach is used for all the MPM packet formats, not just the measurement records. The result is that implementers will rarely need to deal with AVAs or ASN.1, but they will have to know the fixed-format orders for each of the MPM packets.

We have also defined an MPM-specific time stamp which is 10-bytes. There is no 20601 attribute for this type of time stamp. Experience has shown that the 10-byte MPM time stamp is a better and more flexible approach and solves many of the time issues experienced in the field with BT SIG and proprietary PHDs. We also keep the same 10-byte format for all time stamp variants for consistency of implementations even if a given element of the time stamp is not supported.

### Measurement Grouping: Header

For the measurement templates we note that several of the popular devices send more than one measurement at a given point in time, for example blood pressure cuffs send both the blood pressure and pulse rate. Therefore, the MPM has defined a measurement record as concatenation of all measurement templates with a common time stamp. The time stamp is 10 bytes, one of the longer fields. The common time stamp is placed in a header along with other possible attributes or fields that are common to all measurements instead of repeating them in each of the individual measurements. An additional flags field is placed in the header to show which of the possible common fields are present. This approach not only shortens the message for multiple-measurement devices, but semantically groups measurements that are taken simultaneously into a single package. Another advantage of this grouping is for PHDs that don&#39;t send time stamps but rely on the time of reception for the time stamp. In this manner the PHD can group all the measurements that should be considered as occurring at the same time into the single package. If the measurements are sent separately, they will be received at different times by the PHG and reported with different times. Using the headers the PHD can avoid this problem if desired. A disadvantage of the header approach is that single-measurement PHDs like the thermometer and weight scale will lose some efficiency here as they will have the baggage of the header without reaping the rewards of avoiding duplication.

In any case, the above describes how the MPM packets were derived from the IEEE 11073 20601 DIM Objects.

## **Measurement Records:**

In the end measurements are what 90% of users are interested in. Describing them is also the most complex part of the MPM as measurements can be extremely varied. In this MPM model, both stored and live data are sent in records. The records have the same format in both cases. A record contains all measurements with a common time stamp. Some live records may have no time stamp; in that case the time stamp is given by the time of reception which will have the advantage that all the measurements in the group will have the same reception time if that is desired. If this is NOT what the PHD wants, it puts the separate measurements into its own group.

A record is formatted as follows:

| command | header | Measurement 1 | Measurement 2 | … | Measurement _n_ |
| --- | --- | --- | --- | --- | --- |

The command entry is either &#39;stored record&#39; or &#39;live record&#39;. It is two bytes.

### Header:

The header is formatted as follows:

| flags | Length of PDU | Time stamp | Supplemental types | references | duration | Person id | AVAs | Group id | Number of measurements |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

#### Flags:

- Flags: (Two bytes)
  - Bit 0: When set, there is a time stamp
  - Bit 1: When set, the header has supplemental types which are common to every measurement. The individual measurement may have additional supplemental types but shall not duplicate those in the header.
  - Bit 2: When set, the header has references which are common to every measurement. The individual measurement may have additional references but shall not duplicate those in the header.
  - Bit 3: When set, the header has a duration which is common to every measurement. However, if an individual measurement also has a duration, that duration will take precedence.
  - Bit 4: When set, the header has a person identifier common to all measurements. It is assumed that two people cannot take a measurement on the same device at the same time so there is no individual measurement option for a person id.
  - Bit 5: When set, the measurements are settings (such as the height, age, and weight on many activity monitors). At this time there is no individual measurement settings option so settings and &#39;standard&#39; measurements cannot be mixed in a group.
  - Bit 6: When set, the header has AVA structs that are common to all measurements. The individual measurement may have additional AVA structs but they shall not duplicate those in the header.

The following behavior is currently supported but this is likely to be extended or even dropped if it does not find a real use:

  - _Bit 7: When set, this record is the first record in a sequence of records where the following records only have the measurement values. This special option is meant to support a more efficient means of sending live continuous measurements or a large database of stored measurements. When this field is set, there will be a group id field prior to the number of measurements. See the section on Optimized Measurement Transmission._
  - _Bit 8: When set, this record is one of the records that belong to the sequence of optimized records. All the static information is given in the first record of the sequence. Which sequence this record belongs to is given by the group id. When this field is set, there will be a group id in the header. See the section on Optimized Measurement Transmission._
- Length: The length of the remaining entries in the record. The length field occupies two bytes.
- Time stamp: This field is only present in this position if flags bit 0 is set. If present, it occupies 10 bytes.
- Supplemental Types: This field is only present in this position if flags bit 1 is set. It contains a one-byte count of the number of supplemental types followed by the list of supplemental types. Each supplemental-type is a four-byte nomenclature code.
- References: This field is a list of two-byte unsigned integers which are the ids of the referenced measurements. It is preceded by a one-byte field giving the number of references in the list. These are common to all measurements. The measurements referenced by this measurement must have already been received by the gateway during the current connection. Only present in this position if flags bit 2 is set.
- duration: This field is a four-byte Mder FLOAT giving the duration of the measurement in seconds. Only present in this position if flags bit 3 is set.
- Person id: This field contains a two-byte person id when flags bit 4 is set.
- AVAs: This field contains the appended AVA structs that are common to all measurements. It is preceded by a one-byte field giving the number of AVA structs. Only present in this position if flags bit 6 is set.
- Group id: This one-byte field is only used (but it always present) if flags bits 7 and 8 are set.
- Number of measurements: the number of measurements encapsulated in this record. Currently one byte which means a maximum of 256 measurements. This field is always present.

### **Measurement:**

Each measurement _n_ is formatted as follows:

| type | length | Measurement flags | id | Measurement Value | Supplemental Types | reference | duration | AVAs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |


### Flags:

- Type: What the measurement is as a four-byte nomenclature code. Every measurement has a type field.
- Length: The length of the remaining part of the _individual_ measurement. If the gateway does not understand the type, it can skip the measurement by jumping &#39;length&#39; bytes to the start of the next measurement. (two bytes)
- Measurement Flags: (two bytes) – The measurement types are mutually exclusive, so they are given by the value of the first four bits.
  - Bits 0, 1, 2, 3: When 0, the measurement is a numeric.
  - Bits 0, 1, 2, 3: When 1, the measurement is a compound numeric.
  - Bits 0, 1, 2, 3: When 2, the measurement is a coded enumeration.
  - Bits 0, 1, 2, 3: When 3, the measurement is an n-bit BITs enumeration.
  - Bits 0, 1, 2, 3: When 4  Reserved for future use
  - Bits 0, 1, 2, 3: When 5, the measurement is periodic sample or RTSA.
  - Bits 0, 1, 2, 3: When 6, Reserved for the string enumeration (currently not implemented)
  - Bits 0, 1, 2, 3: *When 7, the measurement is a sequence of AVAs (extended object, currently not implemented)*
  - Bits 0, 1, 2, 3: When 8, the measurement is a complex compound.
  - Bit 4: When set, the measurement has a supplemental types field.
  - Bit 5: When set, the measurement has a references field.
  - Bit 6: When set, the measurement has a duration field.
  - Bit 7: When set, the measurement has one or more AVA entries.
  - Bit 8: When set, measurement values are SFLOATs (two byte format) versus FLOATs (four byte format); applies to simple and compound numerics only.
- id: Identifier for this measurement. (Two-byte unsigned integer.) This value is used to reference this measurement from another measurement. The ids must be unique for every measurement sent _within a connection_.
- Measurement Value: This field depends upon which measurement value it is. The format of these fields is given in subsequent sections.
- Supplemental Types: This field is only present in this position if flags bit 4 is set. It contains a one-byte count of the number of supplemental types followed by the list of supplemental types. Each supplemental type is a four-byte nomenclature code.
- Reference: This field is a list of two-byte unsigned integers which are the ids of the referenced measurement. It is preceded by a one-byte field giving the number of references in the list. The measurements referenced by this measurement must have already been received by the PHG during the current connection. Only present in this position if flags bit 5 is set.
- Duration: This is a four-byte Mder FLOAT giving the duration of the measurement in seconds. Only present in this position if flags bit 6 is set.
- AVAs: Additional AVA elements. Only present in this position if flags bit 7 is set.

### Measurement Value Types:

The Measurement Value is formatted depending upon the type of measurement value it is. The MPM supports the following measurement value types; numeric, compound numeric, single coded enumeration, BITs, periodic samples (waveforms), strings, and complex compound. The measurement flags indicate which of these measurement values the measurement is.

#### Numeric Measurement:

The measurement is a numeric if measurement flags bit 0 is set. Most PHD measurements are numerics. The numeric Value is formatted as follows:

| units | value |
| --- | --- |

Units: Given as a two-byte nomenclature term code from the partition DIM (4).

Value: The value is either an Mder SFLOAT (two bytes) or FLOAT (four bytes). Which float option is used is determined by the measurement flags bit 8. If set, two-bit SFLOATs are used. See the secontion IEEE Mder FLOATs for more information.

#### Compound and Complex Compound Numeric Measurement:

The measurement is a compound numeric (a vector) if measurement flags bits 0, 1, 2, 3 is equal to 1. The compound numeric Value is formatted as follows:

| units | count | Sub-type _n_ | Sub-value _n_ |
| --- | --- | --- | --- |

The measurement is a complex compound numeric if measurement flags bits 0, 1, 2, 3 is equal to 8. The complex compound numeric Value is formatted as follows:

| count | Sub-type _n_ | Sub-value _n_ | Sub-units _n_ |
| --- | --- | --- | --- |

Count: one-byte unsigned integer giving the number of compound elements

Sub-type: the four-type nomenclature code giving the sub type of the component

Sub-value: the sub value as either an Mder SFLOAT (two bytes) or FLOAT (four bytes). Which float option is used is determined by the measurement flags bit 8. If set, two-bit SFLOATs are used.

Sub-units: the two-byte term code in partition DIM. Putting the units in the compound allows each compound entry to have a different unit.

For the (simple) Compound the units are all the same and factored out

#### Coded enumeration Measurement:

The measurement is a coded enum if measurement flags 0, 1, 2, 3 is equal to 2. The coded enumeration Value is formatted as follows:

| code |
| --- |

code: Four-byte nomenclature code.

#### n-byte n*8-bit BITs Measurement:

The measurement is an n*8-bit BITs measurement if measurement flags 0, 1, 2, 3 is equal to 3 where n = 1, 2, 3, 4. The BITs enumeration Value is formatted as follows:

| number of bytes | BITs value | BITs type mask | BITs support mask |
| --- | --- | --- | --- |

number of bytes: the number of bytes for the value: 1, 2, 3, 4.

BITs value: n*8 bits BITs value following the 20601 Mder encoding scheme for such measurements.

BITs type mask: n*8 bits mask indicating whether the given bit is a state (bit set) or event (bit cleared)

BITS support mask: n*8 bits mask indicating whether the PHD supports the given bit setting (bit set) or does not (bit cleared).

#### Periodic Sampled Data (Waveform) Measurement:

The measurement is a Waveform measurement if measurement flags 0, 1, 2, 3 is equal to 5 is set. The waveform measurement Value is formatted as follows:

| units | period | Scale factor | offset | Sample size | Number of samples | samples |
| --- | --- | --- | --- | --- | --- | --- |

Units: Given as a two-byte nomenclature term code from the partition DIM (4).

Period: four-byte Mder FLOAT giving the interval between each sample in seconds

Scale Factor: Four-byte Mder FLOAT giving the scale factor _m_ in the relation y_i_ = _m_x_i_ + _b_ where y_i_ is the original data and x_i_ is the scaled data.

Offset: Four-byte Mder FLOAT giving the offset _b_ in the relation y_i_ = _m_x_i_ + _b_ where y_i_ is the original data and x_i_ is the scaled data.

Sample size: one-byte value giving the number of bytes each sample takes. The supported values are 1, 2, and 4.

Number of Samples: two-byte unsigned integer giving the number of data samples.

Samples: the array of &#39;number of samples&#39; samples.

# **Remaining MPM packets**

We use the same fixed-format concept for the remaining MPM packets. The packet details are given below.

## **Current Time Info:**

The current time info will indicate whether the PHD supports time stamps at all and for those PHDs that do the information contained in the time stamp.

PHDs that only _generate_ measurements while connected to a gateway and never send stored measurements do not need to report time stamps. The time of reception is taken as the time stamp. These PHDs do not need to make their sense of current time accessible to the gateway as it serves no purpose. A gateway requesting the Current Time Info from a PHD that does not support a time clock will get an unsupported indication.

PHDs that store data or report time stamps in their measurements even if they do not store data must make their sense of current time accessible to the gateway. In this standard, when the gatewat requests the current time, the current time also contains a set of meta data which indicates the PHD&#39;s capabilities with respect to its time clock.

The format of the Current Time Info is as follows:

| command | flags | length | Current Time | AVA count | AVA structs |
| --- | --- | --- | --- | --- | --- |

The format of the Current Time is as follows:

| epoch | flags | offset | Time sync |
| --- | --- | --- | --- |

- Command: Always the &#39;Get Current Time Info&#39; command. (Two bytes)
- Main Flags: (Two bytes)
  - Bit 0: when set, the PHD supports the set time operation
  - Bit 1: when set, one or more AVA structs are appended to the end

Only one of the three clocks is allowed during any connection. Alternatively, if the PHD only streams live data, no clock may be present. In that case the flags field is 0 and the length field is 0 and the entire packet is only 6 bytes.

- Length: The length of the remaining entries in the Current Time Info (Two Bytes)
- Current Time: (ten bytes) contains the epoch, flags, offset, and time sync. The current time and all time stamps have exactly the same 10-byte format.
  - Epoch (6 bytes) gives the epoch in milliseconds which gives a range of 3,257,812.23 days or 8901.13 leap years (366-day years) or seconds. For relative times the 0 value means nothing. For all other times the 0 value is defined as 2000/01/01 00:00:00.000 UTC.
  - Flags: (1 byte)
    - Bits 0, 1: 0 = reserved
    - Bits 0, 1: 1 = supports a relative time where the epoch 0-value has no meaning
    - Bits 0, 1: 2 = supports an epoch where the 0-value is the UTC time 2000/01/01 00:00:00.0000 (if the offset field is 0x80, offsets are not supported and this clock is equivalent to the 20601 'absolute' time. The 7-byte BT SIG time is also an absolute time, just in a civil format.
    - Bits 0, 1: 3 = reserved
    - Bits 2, 3: 0 = clock resolution is seconds
    - Bits 2, 3: 1 = clock resolution is milliseconds
    - Bits 2, 3: 2 = clock resolition is tenths of a millisecond
    - Bits 2, 3: 3 = clock resolution is hundredths. A little weird here, but that is to be consistent with the BT-SIG GHS.
    - Bits 4, 5 reserved
    - Bit 5: 1 = If set the epoch value is not on the current timeline of the PHD clock. For the current time, bit 5 will never be set, but for stored data, it is possible to have that situation such as upon power up after a battery change.
  - Offset: (1 byte) gives the signed offset to UTC from the current time zone in units of 15 minutes. For UTC epoch only clocks the offset is set to 0x80 indicating the offset to UTC from local time is not supported. MOST PHDs fall into this category.
  - Time Sync (2 bytes) gives the MDC 16-bit term code from partition 8 for the time synchronization. When set to MDC\_TIME\_SYNC\_NONE this tells the PHG to set the time if the PHD supports the set time.
- AVA count: Only present if main flags bit 1 is set. One byte field giving the number of appended AVA structs.
- AVA: Only present if main flags bit 4 is set. The appended AVA structs. As of this writing, there are no currently defined attributes that could be used here. It is present for future extensibility and/or private use.

If there are no AVA structs, this message is 16 bytes which fits in a single Bluetooth notification when using the minimum MTU size of 23 bytes. Recall AVA structs are used to add additional information that can still be decoded blindly by a gateway. An example of an AVA struct that could be added would be the time sync accuracy. Such an attribute does exist in 20601 but it has never been used in market PHDs so it is not included in the &#39;fixed format&#39; part of the packet.

### The 10-byte Time:

Why do we have this 10-byte time format and this &#39;epoch&#39; concept? Why a UTC epoch? How can the PHD get UTC epochs?

#### Relative Time Only Epoch:

First, using epochs (just a counter) is very simple on embedded platforms. There is no need to convert counters to civil times for the protocol. Every platform has some counter which advances at some fixed rate. These counters make using relative times especially simple. Relative time only PHDs don&#39;t need to synchronize and put the work of displaying civil times on the gateway. Since the PHD must expose its current time, the gateway can use that current time tick of the PHD to map any time stamp to civil time, since the gateway knows the civil time at the time it got the current time tick value from the PHD.

One limitation of a relative time PHD is that the civil times generated by the gateway will always be in the time zone of the gateway. Since most PHDs operate only in a single time zone, the limitation is not usually a problem. On the other hand, even if the measurement was taken in another time zone, the reported time of the event is correct relative to the current gateway time zone. For example, if the measurement was taken at 4 AM in a time zone one hour east of the current time zone, it will be reported as 3 AM in the current time zone. Relative to the current time zone, that 3 AM time is correct.

A second limitation of a relative time only PHD is that the time stamp of stored data cannot be recovered if the current timeline is broken. There is no way for the gateway to map those time stamps to its civil times.

#### UTC Time Epoch:

The next step up is to use UTC epochs where the epoch 0-value means something. On embedded platforms it is no more difficult to implement than a relative time except that the PHD would likely want to support the set time. An implementer might ask &#39;how does such a device initialize that epoch before it talks to a gateway&#39;?

One approach is to simply set a factory default value and count from there. If someone takes a measurement before connecting to a gateway, that measurement will have the tick value based upon the factory default. It will not have the correct UTC epoch value. However, when it connects to the gateway, the gateway will read what the PHD _thinks_ is the current UTC epoch value and the gateway will note that it is wrong. The gateway can then set the correct UTC epoch value and the PHD will correct its current tick and stored data time stamps accordingly and when the gateway retrieves the stored data, the time stamp will be correct, at least relative to the time zone of the gateway.

The UTC-epoch PHD has the same limitation as a relative time only PHD; the civil time stamps will always be those in the current time zone of the gateway. The only way to solve that problem is to have a Base offset supporting PHD. The UTC epoch only has one advantage over the relative time epoch only PHD. If the current timeline is lost and the stored data time stamp indicates it was synchronized at the time the measurement was taken, the time stamp CAN be recovered. Unlike the relative time only variant, the epoch value means something by itself. It&#39;s the UTC epoch and the gateway can generate a civil time from that UTC epoch value in the current time zone of the gateway. If the timeline of the stored data was broken before synchronization, there is no way to recover the _correct_ time stamp – but since it is a UTC epoch, by definition, a time stamp can be generated but it is untrustworthy, and the gateway must handle it indicating a time fault. A time fault indicates to the reader that a time stamp was retrieved, but it could not be validated.

#### UI Concerns:

Note that there are basic C- methods that can convert epochs to civil time and vice versa. The user might need to enter a time zone to assure a correct display, but it can be done. A UI can even take advantage of the arbitrariness of the relative time only clock to set a tick value that the PHD display can interpret in its own manner to convert the ticks to local civil time. However, the gateway has no such knowledge of what the epoch 0 value means. Many PHDs do not display the current time as it is assumed the user knows the time when the measurement is taken.

## **System Info:**

The format of the System info is as follows:


| command | flags | length | System id | count | Specialization code _n_ | length | Manufacturer Name |
| --- | --- | --- | --- | --- | --- | --- | --- |

| length | Model number | Regulation status | length | Serial number | length | firmware |
| --- | --- | --- | --- | --- | --- | --- |

| length | software | length | hardware | length | UDI label | length | UDI device identifier |
| --- | --- | --- | --- | --- | --- | --- | --- |

| length | UDI issuer | length | UDI authority | number of AVAs | AVA _n_ |
| --- | --- | --- | --- | --- | --- |

The informational-content elements that have no flag settings are required. The elements that are not required are present in their shown positions if the corresponding flags bit is set.

- Command: Always the &#39;Get System Info&#39; command. (Two bytes)
- Flags: (Two bytes)
  - Bit 0: When set, there is a regulation status
  - Bit 1: When set, there is a serial number
  - Bit 2: when set, there is a firmware entry.
  - Bit 3: when set, there is a software entry.
  - Bit 4: when set, there is a hardware entry.
  - Bit 5: when set, there are AVA entries
  - Bit 6: when set, there is a UDI label
  - Bit 7: when set, there is a UDI device identifier
  - Bit 8: when set, there is a UDI issuing organization OID
  - Bit 9: when set, there is a UDI authorizing organization OID
- Length: The length of the remaining entries in the System Info (Two Bytes)
- System Id: The EUI-64 (Eight bytes). The system id is often created from the public Bluetooth address. The standard conversion is to take the most significant three bytes of the public Bluetooth address followed by two bytes of 0xFF, 0xFE followed by the three least significant bytes of the Bluetooth address. For example, F2:CB:40:AF:B3:E8 becomes F2CB40FFFEAFB3E8.
- Count: the number of specializations supported by the PHD. (one byte)
- Specializations. The list of specializations supported. Each entry is the two-byte MDC nomenclature term code for the specialization from partition eight (infra).
- Length: (one byte) the length of the manufacturer name entry.
- Manufacturer name: the manufacturer name as a UTF-8 string.
- Length: (one byte) the length of the model number entry.
- Model Number: the model number as a UTF-8 string.
- Regulation Status: the regulation status as a 16-bit BITs measurement (two bytes)
- Length: (one byte) the length of the serial number entry.
- Serial Number: the serial number as a UTF-8 string.
- Length: (one byte) the length of the firmware revision entry.
- Firmware: the firmware revision as a UTF-8 string.
- Length: (one byte) the length of the software revision entry.
- Software: the software revision as a UTF-8 string.
- Length: (one byte) the length of the hardware revision entry.
- Hardware: the hardware revision as a UTF-8 string.
- Udi Label: the Udi Label as a UTF-8 string.
- Length: (one byte) the length of the Udi Device Identifier entry.
- Udi Device Id: the Udi Device Identifier as a UTF-8 string.
- Length: (one byte) the length of the Udi Issuer Oid entry.
- Udi Issuer: the Udi Issuer Oid as a UTF-8 string.
- Length: (one byte) the length of the Udi Authorization organization Oid entry.
- Udi Auth: the Udi Authorization organization oid as a UTF-8 string.
- AVA count: (one byte) the number of appended AVA structs. In theory not needed but helpful for decoding and consistency checking.
- Any number of self-describing AVA structs

Given that the length field of the manufacturer name, model number, serial number, firmware, software, and hardware revisions is one byte, the corresponding entry value cannot exceed 256 bytes.

An example of an AVA struct that might be added is for the part number. Such a struct does exist but it has never been used in market PHDs thus it is not included in the &#39;fixed format&#39; part of the packet.

The System Info is going to be longer than what can fit into a single Bluetooth notification when the MTU size is the 23-byte minimum. Therefore, the PHD is going to require multiple notifications to transfer the data.

Given that the system info is static for the lifetime of the device, it can be coded once and placed in ROM. The gateway need only ask for it on a first time connect. However, it may ask for it on every connection.

UDI (Universal Device Identifier): The UDI is an up-and-coming concept, and we should probably add it to the fixed format part above as it will likely be required by many governments and regulation bodies. However, it is a complex struct and it is not clear how it should be best rendered. An AVA for the UDI has been defined in 20601 and it may be best to leave it as an AVA.

It should be noted that there are AVA and ASN.1 structs for all the informational content fields that are currently expressed in fixed format.

## **Config Info:**

This is a TODO. Not sure what this will entail. Currently, it is not needed. The config info may be a good place to put support information, for example, the PHD could indicate whether it supports stored data or the &#39;efficient measurement sequencing&#39; operation and perhaps other features. Don&#39;t know if it is worth it.

# **Command Structure**

The commands have no defined standard (yet). One proposal is to create a private partition in the nomenclature and define command term codes, each of which specify an ASN.1 structure, much like the AVA of the attributes. Most of these commands are simple with no arguments. At the moment we have just defined a set of codes out of the blue.

## Commands

In this trial MPM implementation the commands are specified as the following 16-bit unsigned integers:

- COMMAND\_GET\_SYS\_INFO_ = 0x000A
- COMMAND\_GET\_CONFIG\_INFO_ = 0x000B
- COMMAND\_GET\_CURRENT\_TIME = 0x000C
 
- COMMAND\_SET\_CURRENT\_TIME_ = 0x000D [time 10 bytes]
- COMMAND\_GET\_NUMBER\_OF\_STORED\_RECORDS = 0x000E
- COMMAND\_GET\_ALL\_STORED\_RECORDS = 0x000F
- COMMAND\_GET\_STORED\_RECORDS_BY\INDEX = 0x0010
- COMMAND\_GET\_STORED\_RECORDS_BY\TIME = 0x0011
- COMMAND\_DELETE\_ALL\_STORED\_RECORDS_ = 0x0012
- COMMAND\_SEND\_LIVE\_DATA_ = 0x0013
- COMMAND\_PROPRIETARY_ = 0xFFFF [parameters]

After the task is done, the PHD sends a packet that consists of the 16-bit command followed by the 16-bit result followed by any parameters [command][response][parameters].

Only the *get number of stored records* command has parameters - it returns the number of stored records and the epoch of the first and last entries.

## Command Results

The results are specified as the following 16-bit integers:

- CP\_RESULT\_COMMAND\_DONE_ = 0x0000
- CP\_RESULT\_RECORD\_DONE_ = 0x0001
- CP\_RESULT\_UNSUPPORTED\_COMMAND_ = 0x0002;
- CP\_RESULT\_UNKNOWN\_COMMAND_ = 0x0003;
- CP\_RESULT\_ERROR_ = 0x0004;

## Commands Response Parameters

The get number of stored records command is two bytes containing the number of stored records followed by the epoch of the first and last records in little endian format. If there are no stored records the epoch values are both zero. If there is only one record the two epochs have the same value.

# **IEEE Mder FLOATs**

This section describes Mder FLOATs; what they are and why they are used. 

Floating point numbers in this specification are represented as an IEEE 32-bit FLOAT or IEEE 16-bit SFLOAT. The reason this format is used is that it preserves precision (precision is different than accuracy). Though the decimals 2, 2.0, 2.00, and 2.000 are all numerically equal, the 2.000 representation indicates that the value obtained is precise to three decimal places. The SFLOAT or FLOAT representation assures that the recipient gets the measurement with the precision to which it was measured. This format is used in some BT SIG health device profiles.

## **IEEE 32-bit FLOAT**

The most significant 8-bits give the exponent and the remaining 24-bits the mantissa. Both are signed. The decimal number is given by _mantissa_ \* 10_exponent_.

- 2 as a FLOAT would be 0x00000002 (exponent 0 and mantissa 2)
- 2.0 as a FLOAT would be 0xF0000014 (exponent -1 and mantissa 20)
- 2.00 as a FLOAT would be 0xE00000C8 (exponent -2 and mantissa 200)
- 2.000 as a FLOAT would be 0xD00007D0 (exponent -3 and mantissa 2000)

## **IEEE 32-bit FLOAT Special Values**

The IEEE FLOAT reserves a set of values that represent special floating-point conditions. They are:

- NAN (not a number)
- PINF (positive infinity)
- NINF (negative infinity)
- NRES (not at this resolution)
- RSVD (reserved)

and are encoded as follows:

| **Special Value** | **Encoding** |
| --- | --- |
| NAN | exponent = 0 mantissa = 0x7FFFFF 32-bit value = 0x007FFFFF |
| PINF | exponent = 0 mantissa = 0x7FFFFE 32-bit value = 0x007FFFFE |
| NINF | exponent = 0 mantissa = 0x800002 32-bit value = 0x00800002 |
| NRES | exponent = 0 mantissa = 0x800000 32-bit value = 0x00800000 |
| RSVD | exponent = 0 mantissa = 0x800001 32-bit value = 0x00800001 |

## **IEEE 16-bit SFLOAT**

The most significant 4-bits give the exponent and the remaining 12-bits the mantissa. Both are signed. The decimal number is given by _mantissa_ \* 10_exponent_.

- 2 as an SFLOAT would be 0x0002 (exponent 0 and mantissa 2)
- 2.0 as an SFLOAT would be 0xF014 (exponent -1 and mantissa 20)
- 2.00 as an SFLOAT would be 0xE0C8 (exponent -2 and mantissa 200)
- 2.000 as an SFLOAT would be 0xD7D0 (exponent -3 and mantissa 2000)

## **IEEE 16-bit FLOAT Special Values**

The IEEE FLOAT reserves a set of values that represent special floating-point conditions. They are:

- NAN (not a number)
- PINF (positive infinity)
- NINF (negative infinity)
- NRES (not at this resolution)
- RSVD (reserved)

and are encoded as follows:

| **Special Value** | **Encoding** |
| --- | --- |
| NAN | exponent = 0 mantissa = 0x7FF 16-bit value = 0x07FF |
| PINF | exponent = 0 mantissa = 0x7FE 16-bit value = 0x07FE |
| NINF | exponent = 0 mantissa = 0x802 16-bit value = 0x0802 |
| NRES | exponent = 0 mantissa = 0x800 16-bit value = 0x0800 |
| RSVD | exponent = 0 mantissa = 0x801 16-bit value = 0x0801 |

In Bluetooth Low Energy IEEE FLOATs/SFLOATs are sent in little endian order.

# **Optimized Measurement Transmission**

This option is still in a testing phase. You may skip this section. It may or may not be adopted in the final MPM. It is also subject to change as more experimenting is done.

If a PHD sends a sequence of several measurements, perhaps continuously, like streamed pulse oximeter data, we know that several fields in the measurement packet are the same and thus sent repeatedly. The size of the packet could be made considerably smaller if the static unchanging fields could be sent once and any new or updated measurement would contain only the changed fields. The IEEE 11073 20601 protocol works in this fashion.

The MPM supports an option like 20601 but it is only valid during a single connection and is not as flexible. On the other hand, it is far simpler to implement because of its limitations. The full record is sent once first and then only the &#39;changed&#39; fields are sent in subsequent records. The sequence is identified by the group id. To support this option, the PHG need only track the group id and the associated initial record and only for a connection.

## **Initial Record**

The first record in the sequence is sent with all the fields including the static fields like a normal record except header flags bit 7 is set. This flag indicates that this record is the first in a sequence of records where the following records will only contain the updated fields of the individual measurement values. When bit 7 is set, the header will contain a one-byte group id field before the number of measurements field. The group id is used to link the following optimized records to the initial record so the PHG can obtain the static fields for the new record.

The first record with header flags bit 7 set.

| Command | flags | Length of PDU | Time stamp | Supplemental types | reference | duration |
| --- | --- | --- | --- | --- | --- | --- |
| Person id | AVAs | Group id | Number of measurements |
| --- | --- | --- | --- |

| type | length | Measurement flags | id | Measurement Value | Supplemental Types |
| --- | --- | --- | --- | --- | --- |
| reference | duration | AVAs |
| --- | --- | --- |

Measurement values:

| units | value |
| --- | --- |

| units | count | Sub-type _n_ | Sub-value _n_ |
| --- | --- | --- | --- |

| code |
| --- |

| 16 BITs value | 16 BITs type mask | 16 BITs support mask |
| --- | --- | --- |

| 32 BITs value | 32 BITs type mask | 32 BITs support mask |
| --- | --- | --- |

| units | period | Scale factor | offset | Sample size | Number of samples | samples |
| --- | --- | --- | --- | --- | --- | --- |

| count | Sub-type _n_ | Sub-value _n_ | Sub-unit _n_ |
| --- | --- | --- | --- |

## **Subsequent Records**

The subsequent records are identified by having header flags bit 8 set and a group id with the

same value as the group id of the initial record. The sequence can be illustrated as follows:

| Command | flags | Length of PDU | Time stamp | Group id |
| --- | --- | --- | --- | --- |

| Id\* | Measurement Value |
| --- | --- |

Measurement values:

| value |
| --- |

| Sub-values _n_ |
| --- |

| code |
| --- |

| 16 BITs value |
| --- |

| 32 BITs value |
| --- |

| Number of samples | samples |
| --- | --- |

| Sub-values _n_ |
| --- |

\*It should be noted that the id field is present at the start of every individual measurement value though it may not appear so from the figure.

The only flags bit that is of interest is bit 8, all the other flags are ignored. If there is a time stamp in the initial record, there will be a time stamp in all subsequent records. If there are 10 measurements in the first record, there are 10 measurements in all subsequent records. All other fields except the measurement value are assumed static and are therefore not repeated. If the other fields are not static, this optimization cannot be used.

Clearly the subsequent records in this sequence are shorter in length. The other values needed to decode this record are in the initial record. The initial record is identified by the initial record with the same group id. In this manner, one could support multiple sequences of record groups. The order of the individual measurements in the subsequent records shall be identical to the order of the individual measurements in the original record. If the initial record had six measurements, the subsequent records must have the same six measurements in the same order.

## **Changes in the Static Fields**

Unlike 20601, this scheme does not support the occasional updating of the non-value fields like the units. If, for some reason, the unit code in one of the individual measurements in the sequence of records is to change, a new initial record must be sent with the updated unit code along with everything else. The group id of the previous record sequence could be reused or a new one assigned. If the group id is reused, it must now point to the most recently received initial record with that group id.

## **Use Cases**

It is envisioned that this option would only be used in streaming type situations or where close to real time delivery is desired. It might also be used when sending large amounts of stored data to speed up transfer and cut down on power. A blood pressure cuff might use this approach to send the cuff pressure records as the cuff is operating but not the final blood pressure and pulse rate record.

## **Statics Flag Option**

Requiring everything to be static might be too restrictive. There are some optional fields like the duration, references, and supplemental types which are likely to vary every measurement. For example, the meal context supplemental type of the glucose concentration is likely to be different for every measurement. The same could be said for the duration of an activity session.

When the optimization start flag is present, an additional flags byte could be added where flags would be set to indicate that the optimized messages also contain supplemental types, references, and durations. If the supplemental flag is set, that means both the header and individual measurements that have supplemental types are now present in every optimized packet as well as the values. A simpler option might be just to include these flags in the flags bytes that exist. There is still room.

# **Examples**

The first example is a MPM Pulse Oximeter with stored SPOT data supporting SpO2, Pulse Rate, and Pulsatile Quality. Live data contains continuous streamed data with SpO2, Pulse Rate, Pulsatile Quality, and Pleth-waveforms and an occasional SPOT modality measurement with SpO2, Pulse Rate, and Pulsatile Quality. The continuous data is not time stamped.

The PHD time clock uses a relative time stamp (a sequence of ticks) with a resolution of 1 millisecond.

The MPM advertisement shall contain at least the MPM service UUID. It may contain other service UUIDs. It shall also contain a service data entry for the MPM UUID giving the list of supported specializations. In this case the supported specialization is the pulse oximeter whose MDC term code is 0x1004. All specialization MDC codes come from partition INFRA (8) so only the two-byte term code is used. The MPM advertisement shall also expose the friendly name (either full or shortened).

## Advertisement

The MPM advertisement is required to contain the friendly name (full or shortened) the MPM service UUID, and the specializations supported. The specializations supported is placed in the service data field. This information can be split between the advertisement and the scan response. The MPM shall respond to a scan request.

Below is an example advertisement for a MPM pulse oximeter:

[02 01 06] // advertisement flags

[0D 09 4D 50 4D 20 50 75 6C 73 65 20 4F 78] //Friendly name: [MPM Pulse Ox]

[03 03 90 F9] // Service UUID MPM: 0xF990

[07 16 90 F9 01 04 10 01 // Service data MPM UUID: [0xF990] one [0x01] Specialization Pulse Oximeter: [0x1004] and pairing required [0x01]

// pairing required

Note that we are assuming that Bluetooth SIG has defined the UUID 0xF990 for the MPM so the 16-bit UUID can be used. However, the MPM UUID cannot be used to identify what the device is since every MPM device has the same service UUID. The device type is given by the MDC specialization term code and is placed in the service data field. The service data field has the UUID of the service followed by the data for that service followed by pairing requirements.

Below is a list of some of the MDC term codes for specializations:

MDC\_DEV\_SPEC\_PROFILE\_HYDRA 4096 Multi-specialization
 MDC\_DEV\_SPEC\_PROFILE\_PULS\_OXIM 4100 Pulse Oximeter
 MDC\_DEV\_SPEC\_PROFILE\_MIN\_ECG 4102 Electrocardiogram (ECG)
 MDC\_DEV\_SPEC\_PROFILE\_BP 4103 Blood pressure
 MDC\_DEV\_SPEC\_PROFILE\_TEMP 4104 Thermometer
 MDC\_DEV\_SPEC\_PROFILE\_RESP\_RATE 4109 Respiration rate
 MDC\_DEV\_SPEC\_PROFILE\_SCALE 4111 Weight scale
 MDC\_DEV\_SPEC\_PROFILE\_GLUCOSE 4113 Glucose meter
 MDC\_DEV\_SPEC\_PROFILE\_COAG 4114 Coagulation meter
 MDC\_DEV\_SPEC\_PROFILE\_INSULIN\_PUMP 4115 Insulin pump
 MDC\_DEV\_SPEC\_PROFILE\_BCA 4116 Body composition analyzer
 MDC\_DEV\_SPEC\_PROFILE\_PEAK\_FLOW 4117 Peak flow meter
 MDC\_DEV\_SPEC\_PROFILE\_URINE 4118 Urine analyzer
 MDC\_DEV\_SPEC\_PROFILE\_SABTE 4120 Sleep apnea equipment
 MDC\_DEV\_SPEC\_PROFILE\_PSM 4124 Power service monitor

MDC\_DEV\_SPEC\_PROFILE\_CGM 4121 Continuous glucose meter
 MDC\_DEV\_SPEC\_PROFILE\_HF\_CARDIO 4137 Cardiovascular devices (activity monitors)
 MDC\_DEV\_SPEC\_PROFILE\_HF\_STRENGTH 4138 Strength and fitness devices
 MDC\_DEV\_SPEC\_PROFILE\_AI\_ACTIVITY\_HUB 4167 Assisted and independent living
 MDC\_DEV\_SPEC\_PROFILE\_AI\_MED\_MINDER 4168 Medication monitor
 MDC\_DEV\_SPEC\_PROFILE\_GENERIC 4169 generic device
 MDC\_DEV\_SUB\_SPEC\_PROFILE\_STEP\_COUNTER 4200 step counter (sub profile of cardio)

MDC\_DEV\_SUB\_SPEC\_PROFILE\_HR 4247 heart rate monitor (sub profile of ECG)

## Service Table

A PHG doing service discovery on a PHD that only supports the MPM in addition to the required Generic Access and Generic Attribute services would appear as follows:

Service UUID: 00001800-0000-1000-8000-00805f9b34fb // Generic Access Service

Characteristic UUID: 00002a00-0000-1000-8000-00805f9b34fb // Device Name characteristic

Characteristic UUID: 00002a01-0000-1000-8000-00805f9b34fb // Appearance characteristic

Characteristic UUID: 00002a04-0000-1000-8000-00805f9b34fb // Preferred connection param

Service UUID: 00001801-0000-1000-8000-00805f9b34fb // Generic Attribute service

Service UUID: 0000f990-0000-1000-8000-00805f9b34fb // MPM service

Characteristic UUID: 0000f991-0000-1000-8000-00805f9b34fb // MPM Control Point (CP)

Descriptor UUID: 00002902-0000-1000-8000-00805f9b34fb

Characteristic UUID: 0000f992-0000-1000-8000-00805f9b34fb // MPM Response

Descriptor UUID: 00002902-0000-1000-8000-00805f9b34fb

The MPM Control point is writable. It shall support indications. It is not readable.

The MPM Response characteristic shall support notifications. It is not readable or writable.

In a first-time connect, the MPM gateway enables the CP for indications and the Response characteristic for notifications before it begins the spin-up sequence.

## Spin-Up Sequence
TODO

# **Packet Structure Summary**

## Time Info Packet

| command | flags | length | Current Time | AVA count | AVA structs |
| --- | --- | --- | --- | --- | --- |


| epoch | flags | offset | Time sync |
| --- | --- | --- | --- |


- Time Info Flags: (two bytes)
  - Bit 0: When set, support set time.
  - Bit 1: When set, there is AVA entry fields.
- Current time / time stamp Flags: (one byte)
  - Bits 0,1: time stamp type relative epoch, UTC epoch
  - Bits 2,3,4: seconds, tenths, hundredths, millis, tenths-millis.
  - Bit 6: When set, time stamp is not on the current timeline (always cleared for the current time).

## System Info Packet

| command | flags | length | System id | count | Specialization code _n_ | length | Manufacturer Name |
| --- | --- | --- | --- | --- | --- | --- | --- |

| length | Model number | Regulation status | length | Serial number | length | firmware |
| --- | --- | --- | --- | --- | --- | --- |

| length | software | length | hardware | length | UDI label | length | UDI device identifier |
| --- | --- | --- | --- | --- | --- | --- | --- |

| length | UDI issuer | length | UDI authority | number of AVAs | AVA _n_ |
| --- | --- | --- | --- | --- | --- |

- Flags: (two bytes)
  - Bit 0: When set, there is a regulation status
  - Bit 1: When set, there is a serial number
  - Bit 2: when set, there is a firmware entry.
  - Bit 3: when set, there is a software entry.
  - Bit 4: when set, there is a hardware entry.
  - Bit 5: when set, there are AVA entries
  - Bit 6: when set, there is a UDI label
  - Bit 7: when set, there is a UDI device identifier
  - Bit 8: when set, there is a UDI issuing organization OID
  - Bit 9: when set, there is a UDI authorizing organization OID

## Header Packet

| command | header | Measurement 1 | Measurement 2 | … | Measurement _n_ |
| --- | --- | --- | --- | --- | --- |

| flags | Length of PDU | Time stamp | Supplemental types | references | duration | Person id | AVAs | Group id | Number of measurements |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| type | length | Measurement flags | id | Measurement Value | Supplemental Types | reference | duration | AVAs |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |


| units | float |
| --- | --- |


| units | count | Sub-type _n_ | Sub-value _n_ |
| --- | --- | --- | --- |


| code |
| --- |


| Bytes per value _n_ | BITs 8\*_n_ value | BITs 8\*_n_ type mask | BITs 8\*_n_ support mask |
| --- | --- | --- | --- |


| units | period | Scale factor | offset | Sample size | Number of samples | samples |
| --- | --- | --- | --- | --- | --- | --- |


| AVAs |
| --- |


| Number of AVAs | AVA _n_ |
| --- | --- |

| AVA type | length | value |
| --- | --- | --- |

| count | Sub-type _n_ | Sub-value _n_ | Sub-units _n_ |
| --- | --- | --- | --- |

- Header Flags: (two bytes)
  - Bit 0: When set, there is a time stamp.
  - Bit 1: When set, there is a common supplemental types field.
  - Bit 2: When set, there is a common reference (source-handle-reference) field.
  - Bit 3: When set, there is a common duration field.
  - Bit 4: When set, there is a person id.
  - Bit 5: When set, the measurements are settings.
  - Bit 6: When set, there is a common AVA entry field.

- Measurement Flags: (two bytes)
  - Bits 0, 1, 2, 3: When 0, the measurement is a numeric.
  - Bits 0, 1, 2, 3: When 1, the measurement is a compound numeric.
  - Bits 0, 1, 2, 3: When 2, the measurement is a coded enumeration.
  - Bits 0, 1, 2, 3: When 3, the measurement is a 8\*_n_-bit BITs enumeration.
  -
  - Bits 0, 1, 2, 3: When 6, the measurement is a string enumeration (currently not implemented)
  - Bits 0, 1, 2, 3: When 7, the measurement is a sequence of AVAs (extended object, currently not implemented)
  - Bits 0, 1, 2, 3: When 8, the measurement is a complex compound
  - Bit 4: When set, the measurement has a supplemental types field.
  - Bit 5: When set, the measurement has one of more measurement-unique references (source-handle-reference) field.
  - Bit 6: When set, the measurement has a measurement-unique duration field. Overrides any duration field in the header.
  - Bit 7: When set, the measurement has one or more measurement-unique AVA entry.
  - Bit 8: When set, measurement values are SFLOATs versus FLOATs; applies to simple and compound numerics values only.

## Optimized Header Packet Option

| command | header | Measurement 1 | Measurement 2 | … | Measurement _n_ |
| --- | --- | --- | --- | --- | --- |


| flags | Length of PDU | Time stamp | Group Id |
| --- | --- | --- | --- |

| id | Measurement Value |
| --- | --- |

 
| float |
| --- |


| Sub-value _n_ |
| --- |


| code |
| --- |


| BITs 8\*_n_ value |
| --- |


| Number of samples | samples |
| --- | --- |


| AVAs |
| --- |

| Sub-value _n_ |
| --- |

# **MPM Packet Library**

The idea of a packet library is to hide as much of the details of creating and populating the packets shown in the Packet Summary sent over the airwaves. For most peripheral devices, the measurements supported are fixed as they are determined by the sensor(s). Thus, a Blood Pressure cuff is likely to send a Blood Pressure and Pulse Rate measurement, and maybe a Device-Sensor status measurement. It is highly unlikely that it will add a temperature measurement at some future time.

The measurement packet for this blood pressure device would consist of a single group with a BP, PR, and optionally a status measurement. Many of the fields in this packet will be the same every time the measurement is sent. So, all the application needs to do is create this packet ahead of time as a template and populate those fields that vary when data is received from the sensor.

Below is an example of the packet for the Blood Pressure device with all the 'static' fields pre-populated:

0x00, 0x00, 0x01, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x80, 0x00, 0x1F, 0x01, 0x03, 0x04, 0x4A, 0x02, 0x00, 0x1E, 0x00, 0x11, 0x01, 0x00, 0x00, 0x20, 0x0F, 0x03, 0x05, 0x4A, 0x02, 0x00, 0x00, 0x00, 0x06, 0x4A, 0x02, 0x00, 0x00, 0x00, 0x07, 0x4A, 0x02, 0x00, 0x00, 0x00, 0x01, 0xF4, 0x06, 0x07, 0x00, 0x2A, 0x48, 0x02, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0xA0, 0x0A, 0x00, 0x00, 0xF0, 0x55, 0x80, 0x00, 0x10, 0x00, 0x23, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x02, 0x00, 0x00, 0x00, 0x00

Breaking it down the packet shows it contains the following where the **bold entries need to be populated** before being sent:

- **0x00, 0x00** , // Here we place the command we are responding to (either getting stored data or sending live data)
- 0x01, 0x00, // Header flags. (Supports set-time)
- 0x54, 0x00, // The length. This will not change unless we pop/push measurements onto the packet. Library calls can take care of the necessary updating.
- **0x00, 0x00, 0x00, 0x00, 0x00, 0x00** , // the epoch count of the time stamp (will need to be populated)
- 0x0D, // the flags (UTC epoch and supports milliseconds)
- 0x80, // time zone (unsupported)
- 0x00, 0x1F, // time sync none (may need to be updated when there is a set time)
- 0x01, // Group id
- 0x03, // Number of measurements

// BP measurement:

- 0x04, 0x4A, 0x02, 0x00, // Measurement type: non-invasive blood pressure
- 0x1E, 0x00, // Measurement length
- 0x11, 0x01, // Measurement flags (compound, has supplemental types, uses SFLOATs)
- **0x00, 0x00** , // measurement id
- 0x20, 0x0F, // units, mmHg
- 0x03, // number of components in compound
- 0x05, 0x4A, 0x02, 0x00, // sub-type systolic
- **0x00, 0x00** , // systolic sub-value as Mder SFLOAT
- 0x06, 0x4A, 0x02, 0x00, // sub-type diastolic
- **0x00, 0x00** , // diastolic sub-value as Mder SFLOAT
- 0x07, 0x4A, 0x02, 0x00, // sub-type MEAN
- **0x00, 0x00** , // mean sub-value as Mder SFLOAT
- 0x01, // number of supplemental types
- 0xF4, 0x06, 0x07, 0x00, // Supplemental type is upper arm location for cuff (static in this case)

// PR measurement:

- 0x2A, 0x48, 0x02, 0x00, // Measurement type: non invasive pulse rate
- 0x08, 0x00, // Measurement length
- 0x00, 0x01, // Measurement flags (numeric, uses Mder SFLOATs)
- **0x00, 0x00** , // measurement id
- 0xA0, 0x0A, // Units beats per minute
- **0x00, 0x00** , // Pulse rate as Mder SFLOAT

// Status measurement:

- 0xF0, 0x55, 0x80, 0x00, // Measurement type: device and sensor status
- 0x10, 0x00, // Measurement length
- 0x23, 0x00, // Flags (BITs-16, (2-byte) has references)
- **0x00, 0x00** , // Measurement id
- 0x02 // Number of bytes for BITs
- **0x00, 0x00** , // Measurement value
- 0x00, 0x00, // Whether bit is a state (1) or event (0). Here all are events.
- 0x00, 0xFC, // Which events are supported by the device (Here all defined are supported)
- 0x02, // Number of references
- **0x00, 0x00** , // Reference 1 (contains BP measurement id)
- **0x00, 0x00** // Reference 2 (contains PR measurement id)

When a measurement is obtained from the sensor, only the 'bold' entries need to be populated. To populate them, one needs to know the index in the data array of the bold fields. The application can do that work itself or use the library to do it. When the library is used to create the data array, those static fields that can be populated are populated and a new structure is created that contains the meta data about this data array so those bold fields can be updated by additional library calls.

That is the concept of the MPM library. The library should hide as much of the tedious details of creating the data packet and updating the packet as necessary when data is received from the sensor MCU (usually via an internal serial port or SPI interface).

The library should also provide methods that simplify packet management for the common use cases one sees in the field. The above packet is a perfect illustration of this need. The device-and-sensor status measurement is, hopefully, an infrequent happening, as that generally indicates that there was some problem taking the measurement. There are three options to deal with this situation and all three will work.

1. Set the measurement value of the device and sensor status measurement to 0. The downside of that approach is the extra bandwidth required to send a measurement containing no useful information. However, if done, the PHG will handle it correctly.
2. Make two data packets, one with the device and sensor status measurement and one without. This approach saves bandwidth but requires more resources to store both packets.
3. Create a library call which 'pops' off the status measurement and then call a method which pushes the status measurement back when needed. This approach uses less bandwidth and fewer resources. But it also requires that the application place the status measurement in the last position. Providing such a library method is quite specialized but it is a use case that occurs frequently enough in the common devices having the method is considered worth it.

## Measurement APIs

The library involves two stages. Prior to advertising, the device type, features and measurements are configured. Once configured, the data arrays used during the connection are created. Once connected, update methods are used to populate the data arrays with data obtained from the sensor.

### Configuring the Device Measurements

The application needs to decide what type of device it is and what measurements it reports. Structures are defined to support this process, but the application does not need to know what is in these structures. The library uses the structures internally to do its tasks.

An example library has been implemented for the Nordic nRF51 and nRF52 series of MCUs:

The first step in the configuration process is to create a measurement group as follows:

- createMsmtGroup(s\_MsmtGroup \*\*msmtGroup, bool hasTimeStamp, unsigned char numberOfMetMsmts, unsigned char groupId)

The application defines a pointer to the s\_MsmtGroup structure and sets it to NULL and passes a pointer to this pointer when it makes the call. In this manner, the library allocates memory for the structure, populates it, and now the application has access to the populated structure. However, the application does not need to modify the structure (though it can) but uses it when making other library calls. Clearly the pointer defined by the application must remain in scope until the structure is no longer needed. At the time it is no longer needed, the application needs to call a cleanup method which frees the resources allocated for this s\_MsmtGroup.

Clearly there are other parameters in this method which will not be discussed here.

There are, of course, additional methods that allow the application to further specify features about the group. For example, it may be the case that every measurement happens to need the same supplemental types information, for example the supplemental types indicates that the measurement is an average and it applies to all measurements. One could add the supplemental types to each individual measurement, but it would be more efficient to place it in just the header once. The client will understand that it needs to add this supplemental types value to every measurement in the header, just as it adds the time stamp to every measurement in the group.

There is still the option to add individual supplemental types that are unique to just a given measurement in addition to the one that is common to all measurements. The client will know to add both the common supplemental types and the measurement-specific supplemental types to the measurement.

The following APIs are available to further configure the header fields that are common to all measurements in the group:

- setHeaderSupplementalTypes(s\_MsmtGroup \*\*msmtGroup, unsigned short numberOfSupplementalTypes )
- setHeaderRefs(s\_MsmtGroup \*\*msmtGroup, unsigned short numberOfRefs )
- setHeaderOptions(s\_MsmtGroup \*\*msmtGroup, bool areSettings, bool hasPersonId, unsigned short personId)
- setHeaderDuration(s\_MsmtGroup \*\*msmtGroup)

Note that none of these methods set the value or values of the fields. All they do is tell the generator of the final data array to reserve space for these fields so they can be populated when needed. If the field happens to be static, once the data array is created, one can call the update methods to populate the fields still within the configuration process; that is before the device starts advertising. An example of a static supplemental types might be the MDC\_MODALITY\_SPOT for pulse ox spot measurements. An example where the supplemental types would be dynamic is in the meal, tester, location, and health values of the glucose concentration measurement.

There are similar methods for the supplemental types, references, and duration for the individual measurement. However, this API requires that a person id must apply to all measurements within the group. This limitation is not restrictive given that it is unlikely that two people can take a measurement on the same device at the same time. However, it also requires that a group cannot contain a mixture of settings measurements and dynamic measurements. Scoping measurements in a settings group does seem semantically appropriate currently. That restriction may change if use cases demonstrate otherwise.

#### Adding Measurements

Once the group is created, one needs to decide what measurements the group is to contain. For a blood pressure cuff, one might add the blood pressure and pulse rate measurements. For a pulse oximeter it might be the SpO2 and pulse rate measurements. For a spirometer, one may have twenty or more measurements.

The library provides creators for seven types of measurement values (the compound may be dropped):

- createNumericMsmt(s\_MetMsmt \*\*metMsmt, unsigned long type, bool isSfloat, unsigned short units)
- createCompoundNumericMsmt(s\_MetMsmt\*\* metMsmt, unsigned long type, bool isSfloat, unsigned short units, unsigned short numberOfComponents, s\_Compound \*compounds)createCodedMeasurement()
- createComplexCompoundNumericMsmt(s\_MetMsmt\*\* metMsmt, unsigned long type, bool isSfloat, unsigned short numberOfComponents, s\_Compound \*compounds)
- createCodedMsmt(s\_MetMsmt\*\* metMsmt, unsigned long type)
- createBitEnumMsmt(s\_MetMsmt\*\* metMsmt, unsigned short byteCount, unsigned long type, unsigned long state, unsigned long support)
- createRtsaMsmt(s\_MetMsmt\*\* metMsmt, unsigned long type, unsigned short units, s\_MderFloat\* period, s\_MderFloat\* scaleFactor, s\_MderFloat\* offset, unsigned short numberOfSamples, unsigned char sampleSize)

As with the measurement group, the application defines a pointer to an s\_MetMsmt structure and sets it to NULL. The application passes in a pointer to this pointer and the library allocates memory for the structure and populates it with the provided information. In this manner the application can use the pointer to call additional methods that further configure the measurement. The application does not need to modify the structure though it can. In the end, the application will add the measurement to the group with the following:

- short addMetMsmtToGroup(s\_MetMsmt \*metMsmt, s\_MsmtGroup \*\*msmtGroupPtr)

This method returns a critical measurement index value which the application _must_ retain. They must remain in scope for the connection. The application will need that index when updating this measurement in the data array when data is received from the sensor. The s\_MetMsmt pointer defined by the application must remain in scope until the structure is no longer needed. Once the data array for the group has been created, the structure is no longer needed. When the application calls the method to clean up the s\_MsmtGroup, the resources for the measurement structures added to the group are also released. The application does not need to call separate methods to release these resources.

#### The Data Array

The final step in the configuration process is to create the data array structure which contains the data array to be sent to the peer and the support meta data so the application can call the update methods and not deal with the details of populating the data array. Again, the application creates a pointer to an s\_MsmtGroupData structure and sets it to NULL. The application calls

createMsmtGroupDataArray(s\_MsmtGroupData\*\* msmtGroupData, s\_MsmtGroup \*msmtGroup, s\_MetTime \*sMetsTime, unsigned short packetType);

passing in a pointer to its s\_MsmtGroupData pointer. The method returns a populated structure on element of which includes the data array to be sent to the peer. Once this method is called, the s\_MsmtGroup structure and all s\_MetMsmt structures are no longer needed and can be cleaned up by calling the

cleanUpMsmtGroup(s\_MsmtGroup\*\* msmtGroup)

method. This cleans up all resources allocated including all s\_MetMsmt structs added to the s\_MsmtGroup.

The big difference here is that the application must keep the s\_MsmtGroupData pointer in scope for the duration of the connection. Once disconnected, a cleanup routine can be called to free up the resources allocated to this structure.

The application may need to create multiple groups. For example, a pulse oximeter supporting both continuous measurements and spot measurements will likely create a group for each case, as the continuous measurement typically has no time stamp and is sent much more frequently than the spot measurement, and the spot measurement has a time stamp. The application could be clever and create a single group if both measurements are to have a time stamp (or not have a time stamp) by using the pop and push methods. It would require some clever placing of the measurements in the group, but it could be done. Whether or not the additional complexity is justified is up to the application.

At this point, the application has the data array struct and measurement indices it needs to populate the data array with the data received from the sensor MCUs.

#### Updating the Data Array

It is assumed that data is received from the sensor MCU by some classic method such as UART events/interrupts or a SPI interface. The application's task is to take that data and populate the data array with it. Assuming the application has already mastered the Bluetooth APIs of the given MCU, this effort is probably the most difficult. For one thing, there is not going to be one single approach a sensor MCU delivers its data.

This library supports an approach where the application can queue an arbitrary structure containing the sensor information and dequeue it when ready to send. For example, a thermometer implementation might define a thermometer struct as follows:

typedef struct

{

bool hasTimeStamp;

s\_MetTime sMetTime;

unsigned short temp;

unsigned short ambient;

}s\_MsmtDataTherm;

The thermometer may be able to report the temperature to two decimal places of precision but passing floats and maintaining precision is difficult. What an application may decide to do is report the temperature value times 100 and use integers. Note that the above thermometer also provides the ambient temperature as well as the body temperature.

When the application receives the data from the sensor, it can add the time stamp if not provided by the sensor. Here it is convenient to map the time stamp to the library s\_MetTime struct elements. When the measurement is dequeued, the application uses this data and calls three update routines to place the time stamp, body temperature, and ambient temperature into the data array. To update the time stamp the method calls

updateTimeStampEpoch(s\_MsmtGroupData \*\*msmtGroupDataPtr, unsigned long long epoch)

It might make more sense to create an additional method which passes in the s\_MetTime struct so the application doesn't need to worry about what's in the struct, though this struct will require, at some point, a detailed understanding by the application. Time clocks and counters are not platform independent, and the application is going to have to bridge that gap.

To update the temperature value, the application calls the method

updateDataNumeric(s\_MsmtGroupData\*\* msmtGroupData, unsigned short msmtIndex, s\_MderFloat\* value, unsigned short msmt\_id);

Here the application uses the measurement index it obtained when it added the body temperature to the group. Since the original measurement was a numeric, the update numeric method is called. The application also passes in a measurement id which is an unsigned short integer that must be unique for every measurement instance _during a connection_. The most difficult conceptual part is passing in the numeric value 'temp'. Note that the input parameter is an s\_MderFloat struct. Check the section IEEE Mder FLOAT earlier in this document to understand the details of why this structure is used. In short, its use is to preserve precision. In this case the temperature value has two decimal places precision (scaled by 100) and to maintain that precision the s\_MderFloat.exponent is set to -2 and the s\_MderFloat.mantissa is set to s\_MsmtDataThem.temp. The value is given by mantissa \* 10exponent. The application must also recall if a 2-byte (SFLOAT) or 4-byte (FLOAT) representation is used when the measurement was created. There is also two additional fields to indicate whether the structure represents a NAN, NINR, or PINF or it a normal number. The final population of the s\_MderFloat struct might look as follows:

s\_MderFloat mder;

mder.mderFloatType = MDER\_FLOAT; // Four-byte float

mder.specialValue = MDER\_NUMBER; // Normal number

mder.exponent = -2;

mder.mantissa = temp;

To update the ambient temperature field, the same method is used the only difference being that the measurement index to the ambient temperature, a new value of the measurement id, and a new value of the mder struct needs to be entered. Since the time stamp is valid for all the measurements in the group, no further calls are needed for the time stamp.

The data array is now ready to notify over the MPM response characteristic.

#### Update Methods

There are clearly several update methods as methods are needed to update each measurement value type and all the options within the header or individual measurements, such as the supplemental types, references, and duration. These details are saved for an API reference guide.

#### Library Development

Since the entire packet handling does not involve any platform dependent methods, one can take the entire 'C' library off the embedded platform and develop and test it on a PC. The tested code can then be returned to the embedded platform.
