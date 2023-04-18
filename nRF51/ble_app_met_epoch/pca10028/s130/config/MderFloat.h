/* File MderFloat.h */
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

/* Public */
#ifndef __MDERFLOAT_H_
#define __MDERFLOAT_H_

#include <stdio.h>
#include <stdbool.h>

#define MANTISSA_DIGITS 20
#define MAX_DEST_LEN (MANTISSA_DIGITS + 10)

/**
 * Mder FLOATs are not the easiest to understand. However, what it does provide is a means
 * to specify measurement precision and significant figures. It also uses a set of special
 * reserved values to indicate cases such as NaNs and +/- Infinity cases.
 *
 * There are two types of Mder Floats, A 16-bit version called an SFLOAT and a 32-bit version
 * called a FLOAT. Both have an exponent and a mantissa. The exponent gives the precision of
 * the measurement and the mantissa gives the number of significant figures. The number itself
 * is given by the mantissa * 10 ** exponent.
 *
 * If the exponent is less than 0, it gives the number of figures to the right of the decimal point
 * For example a mantissa of 2 and an exponent of -2 gives 0.02 which is a precision of 2 decimal places.
 * But here is a better illustration of precision; a mantissa of 20 and exponent -1 gives 2.0 while
 * a mantissa of 200 and exponent of -2 gives 2.00. Both are the same number mathematically but the
 * 2.00 indicates the measurement has two decimal places of precision. Thus in a PCD-01 message,
 * the former will be reported as 2.0 (and not 2) while the latter will be reported as 2.00 (and not
 * 2.0 or 2).
 *
 * With respect to significant figures, think of exponent 0 and exponent 1230 and exponent 1 and
 * mantissa 123. Both give 1230. But the former indicates 4 significant figures and the latter
 * only 3. However, this information cannot be sent in a PCD-01 message; both cases will be sent
 * as 1230 without a decimal point.
 *
 * 16-bit SFLOATs use the upper four bits for the exponent and 32-bit FLOATs use the upper 8 bits for
 * the exponent. The remaining bits are the mantissa. Both are signed.
 *
 */
/* Enumerations for Mder FLOAT type */
enum MderFloatType
{
    MDER_EMPTY = 0, //!< EMPTY
    MDER_SFLOAT = 1,//!< MDER_SFLOAT
    MDER_FLOAT      //!< MDER_FLOAT
};

/* The special values that can be represented by Mder Floats */
enum MderSpecialValue
{
    MDER_NUMBER = 1, // Normal number
    MDER_NAN,    // Not a Number
    MDER_PINF,   // Positive infinity
    MDER_NINF,   // Negative infinity
    MDER_NRES,   // Not at this resolution
    MDER_RSVD    // Reserved
};


typedef struct sMderFloat_tag
{
    short int exponent;     /* The Mder Float exponent */
    long int mantissa;      /* The Mder Float mantissa */
    enum MderFloatType mderFloatType;       /* Mder Float type */
    enum MderSpecialValue specialValue;     /* Mder Float special type or NUMBER if not a special value */

} s_MderFloat;

s_MderFloat* copyMderFloat(s_MderFloat* source, s_MderFloat* destination);

/**
 * Method gives the number of characters needed to represent the Mder Float as a string. An application
 * may need this method if it wants to display a measurement from the metric intermediary as the measurement
 * is stored as an MderFloat. Otherwise it is only used indirectly by the PCD-01 generator. If the Mder Float
 * is a special value, that will be taken into account as well as the string returned will be patterned after
 * one of the enums as above.
 * @param smderFloat a pointer to the sMderFloat struct one want to find the string length of.
 * @return the length of the string
 */
short int lengthOfFloatString(const s_MderFloat *smderFloat);

/**
 * Converts an Mder Float value to a printable string to the precision given by the Mder Float encoding. If a
 * special value, a string matching one of the enum special values 'strings' above will be returned. The
 * special value strings match those used by the PCD-01 and FHIR standards for NAN, PINF, and NINF.
 * @param smderFloat a pointer to the Mder Float struct to convert
 * @param buf a pointer to a buffer to contain the converted string. Must be long enough to hold the terminating 0x00 value
 * @param bufLength the length of the provided buffer
 * @return a pointer to the buffer. Returns NULL on error.
 */
char *mderFloatToStringSimp(const s_MderFloat *smderFloat, char *buf, unsigned long bufLength);

/**
 * This gets a special value encoding for an Mder Float. If NUMBER is returned, the Mder Float is not a special value.
 * @param smderFloat a pointer to the Mder Float struct to find the special value of
 * @return a pointer to the string
 */
char *getSpecialValuePcdString(const s_MderFloat *smderFloat);

/**
 * The application should not need to use this method. However, a BTLE Adapter will. This method generates an sMderFloat
 * struct from an exponent and mantissa input values. The caller will need to specify whether the Mder Float is to
 * be encoded as an MDER_SFLOAT (16-bits) or MDER_FLOAT (32-bits). Some BTLE measurements are already formatted as
 * raw 16-bit or 32-bit Mder Floats. Others are integers or scaled integers.
 * @param smderFloat pointer to an application provided sMderFloat struct to be populated
 * @param exponent the exponent
 * @param mantissa the mantissa
 * @param floatType the Mder Float type to generate
 * @param value the special value type to be generated. If not 'NUMBER', the exponent and mantissa will be ignored.
 * @return true if successful
 */
bool createMderFloatFromIntegers(s_MderFloat *smderFloat, short int exponent, long int mantissa, enum MderFloatType floatType, enum MderSpecialValue value);

/**
 * The application should not need to use this method. This creates an sMderFloat struct from a raw Mder SFLOAT
 * @param smderFloat pointer to an application provided sMderFloat struct to be populated
 * @param ieeeSMder the raw Mder SFLOAT value
 * @return true if successful
 */
bool createMderFloatFromSFloat(s_MderFloat *smderFloat, unsigned short ieeeSMder);

/**
 * The application should not need to use this method. This creates an sMderFloat struct from a raw Mder FLOAT
 * @param smderFloat pointer to an application provided sMderFloat struct to be populated
 * @param ieeeMder the raw Mder FLOAT value
 * @return true if successful
 */
bool createMderFloatFromFloat(s_MderFloat *smderFloat, unsigned long ieeeMder);

/**
 * The application should not need to use this method. This creates an sMderFloat struct from a raw Mder SFLOAT
 * @param smderFloat pointer to an application provided sMderFloat struct to be populated
 * @param ieeeSMder the raw Mder SFLOAT value
 * @return true if successful
 */
bool createIeeeSFloatFromMderFloat(s_MderFloat* smderFloat, unsigned short *rawIeeeSfloat);
bool createIeeeFloatFromMderFloat(s_MderFloat* smderFloat, unsigned long *rawIeeeFloat);

unsigned long getIeeeFloatFromString(char* floatAsString);
unsigned long getIeeeSFloatFromString(char* floatAsString);
bool getMderFloatFromString(s_MderFloat* mderFloat, char* floatAsString);

#endif
