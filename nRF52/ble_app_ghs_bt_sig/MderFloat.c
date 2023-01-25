/* File MderFloat.c */

#include "MderFloat.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

s_MderFloat* copyMderFloat(s_MderFloat* destination, s_MderFloat* source)
{
    if (source == NULL || destination == NULL)
    {
        return destination;
    }
    destination->exponent = source->exponent;
    destination->mantissa = source->mantissa;
    destination->mderFloatType = source->mderFloatType;
    destination->specialValue = source->specialValue;
    return destination;
}


/* Returns a length for the buffer used in mderFloatToStringSimp
 * @param smderFloat the Mder float value to be printed as a string
 * @return the length of the buffer, -1 if smderFloat is NULL. */

short int lengthOfFloatString(const s_MderFloat *smderFloat)
{
    if (smderFloat == NULL)
    {
        return -1;
    }
    if (smderFloat->specialValue != MDER_NUMBER)
    {
        return 4;
    }
    if (smderFloat->mderFloatType == MDER_SFLOAT)
    {
        return (smderFloat->exponent >= 0) ? 6 + smderFloat->exponent : 6 - smderFloat->exponent;
    }

    return (smderFloat->exponent >= 0) ? 10 + smderFloat->exponent : 10 - smderFloat->exponent;
}

/*
 * This method creates a string from the mderFloat that represents the
 * precision exactly as defined by the device. The precision is determined
 * by the exponent, where a negative exponent indicates the number of places
 * to the right of the decimal point. Thus a -2 exponent will generate a
 * value of 23.00 and not 23.0 or 23.
 * Positive exponents will always represent integers.
 * Returns a pointer to the input buffer. If there is an error, returns NULL.
 */

char *mderFloatToStringSimp(const s_MderFloat *smderFloat, char *buf, unsigned long bufLength)
{
    size_t length;
    int startPos;
    unsigned char isNegative = 0;
    long int mantissa = smderFloat->mantissa;

    if((smderFloat == NULL) || (buf == NULL))
    {
        return NULL;
    }
    if (smderFloat->specialValue != MDER_NUMBER)
    {
        strcpy(buf, getSpecialValuePcdString(smderFloat));
        return buf;
    }
    /* This ends up catching something like exponent = -2 and mantissa = -1 */
    if (mantissa < 0)
    {
        mantissa = -mantissa;
        isNegative = 1;
    }
    sprintf(buf, "%ld", mantissa);
    if(smderFloat->exponent != 0)
    {
        length = strlen(buf);
        if(smderFloat->exponent < 0)
        {
            if(bufLength >= (length + 1))
            {
                startPos = (int)length + smderFloat->exponent;
                if (startPos < 0) /* Need some 0's between decimal point and digits */
                {
                    memmove(buf - startPos, buf, length + 1);
                    memset(buf, '0', -startPos);
                    length = strlen(buf);
                    startPos = 0;
                }
                if (bufLength >= (length + 1))
                {
                    memmove(buf + startPos + 1, buf + startPos, (size_t)(-smderFloat->exponent) + 1);
                    buf[length + smderFloat->exponent] = '.';
                }
            }
            else
            {
                strcpy(buf, "overflow");
            }
        }
        else if(bufLength >= (length + smderFloat->exponent))
        {
            memset(buf + length, '0', smderFloat->exponent);
            buf[length + smderFloat->exponent] = 0;
        }
        else
        {
            strcpy(buf, "overflow");
        }
    }
    if (isNegative != 0)
    {
        if (bufLength < strlen(buf) + 1)
        {
            memmove(buf, buf + 1, strlen(buf));
            *buf = '-';
        }
        else
        {
            strcpy(buf, "overflow");
        }
    }
    return buf;
}

char *getSpecialValuePcdString(const s_MderFloat *smderFloat)
{
    if(smderFloat == NULL)
    {
        return NULL;
    }
    if(smderFloat->specialValue == MDER_NAN)
    {
        return "NAN";
    }
    if(smderFloat->specialValue == MDER_PINF)
    {
        return "PINF";
    }
    if(smderFloat->specialValue == MDER_NINF)
    {
        return "NINF";
    }
    if(smderFloat->specialValue == MDER_NRES)
    {
        return "OTH";
    }
    if(smderFloat->specialValue == MDER_RSVD)
    {
        return "OTH";
    }
    return "";
}

bool createMderFloatFromIntegers(s_MderFloat *smderFloat, short int exponent, long mantissa, enum MderFloatType floatType, enum MderSpecialValue value)
{   // TODO Check for input errors
    smderFloat->exponent = exponent;
    smderFloat->mantissa = mantissa;
    smderFloat->mderFloatType = floatType;
    smderFloat->specialValue = value;
    return true;
}

bool createMderFloatFromSFloat(s_MderFloat *smderFloat, unsigned short ieeeSMder)
{
    short int mant;
    // TODO Check for input errors
    /* MDER_SFLOAT is tricky at the MS 4 bits are the exponent and the least significat 12 bits are the
     * mantissa. Both the exponent and manitssa are signed using a 4-bit signed value and a 12-bit
     * signed value. Thus the exponent of 0 t0 7 is positive and F to 8 is negative. For the mantissa
     * 0 to 2047 is positive and FFF to 2048 is negative.
     */
    smderFloat->exponent = ((short int)(ieeeSMder & 0xF000)) >> 12;
    /* Roll mantissa one half-byte to get sign of 12 bits, then 'divide' by 16 to keep sign of original value */
    mant = (short int)(ieeeSMder << 4);
    smderFloat->mantissa = (int)(mant >> 4);
    smderFloat->mderFloatType = MDER_SFLOAT;
    if((ieeeSMder & 0xFFF) == 0x07FF)
    {
        smderFloat->specialValue = MDER_NAN;
    }
    else if((ieeeSMder & 0xFFF) == 0x07FE)
    {
        smderFloat->specialValue = MDER_PINF;
    }
    else if((ieeeSMder & 0xFFF) == 0x0802)
    {
        smderFloat->specialValue = MDER_NINF;
    }
    else if((ieeeSMder & 0xFFF) == 0x0801)
    {
        smderFloat->specialValue = MDER_NRES;
    }
    else if((ieeeSMder & 0xFFF) == 0x0800)
    {
        smderFloat->specialValue = MDER_RSVD;
    }
    else
    {
        smderFloat->specialValue = MDER_NUMBER;
    }

    return true;
}

bool createMderFloatFromFloat(s_MderFloat *smderFloat, unsigned long ieeeMder)
{
    // TODO Check for input errors
    smderFloat->exponent =(short int)(((long)(ieeeMder & 0xFF000000)) >> 24);
    /* Roll mantissa one byte to get sign of 24 bits, then 'divide' by 256 to keep sign with original value */
    smderFloat->mantissa = ((((long)ieeeMder) << 8) >> 8);
    smderFloat->mderFloatType = MDER_FLOAT;
    if((ieeeMder & 0xFFFFFF) == 0x007FFFFF)
    {
        smderFloat->specialValue = MDER_NAN;
    }
    else if((ieeeMder & 0xFFFFFF) == 0x007FFFFE)
    {
        smderFloat->specialValue = MDER_PINF;
    }
    else if((ieeeMder & 0xFFFFFF) == 0x00800002)
    {
        smderFloat->specialValue = MDER_NINF;
    }
    else if((ieeeMder & 0xFFFFFF) == 0x00800001)
    {
        smderFloat->specialValue = MDER_NRES;
    }
    else if((ieeeMder & 0xFFFFFF) == 0x00800000)
    {
        smderFloat->specialValue = MDER_RSVD;
    }
    else
    {
        smderFloat->specialValue = MDER_NUMBER;
    }

    return true;
}

bool createIeeeFloatFromMderFloat(s_MderFloat* smderFloat, unsigned long *ieeeMder)
{
    // TODO Check for input errors
    if (smderFloat == NULL || ieeeMder == NULL)
    {
        return false;
    }
    if (smderFloat->specialValue == MDER_NUMBER)
    {
        unsigned long exponent = ((((unsigned long)smderFloat->exponent) << 24) & 0xFF000000);
        unsigned long mantissa = smderFloat->mantissa & 0x00FFFFFF;
        *ieeeMder = (exponent | mantissa);
    }
    else
    {
        if (smderFloat->specialValue == MDER_NAN)
        {
            *ieeeMder = 0x007FFFFF;
        }
        else if (smderFloat->specialValue == MDER_PINF)
        {
            *ieeeMder = 0x007FFFFE;
        }
        else if (smderFloat->specialValue == MDER_NINF)
        {
            *ieeeMder = 0x00800002;
        }
        else if (smderFloat->specialValue == MDER_NRES)
        {
            *ieeeMder = 0x00800001;
        }
        else if (smderFloat->specialValue == MDER_RSVD)
        {
            *ieeeMder = 0x00800000;
        }
    }
    return true;
}

bool createIeeeSFloatFromMderFloat(s_MderFloat* smderFloat, unsigned short* ieeeMder)
{
    // TODO Check for input errors
    if (smderFloat == NULL || ieeeMder == NULL)
    {
        return false;
    }
    if (smderFloat->specialValue == MDER_NUMBER)
    {
        unsigned short exponent = ((smderFloat->exponent << 12) & 0xF000);
        unsigned short mantissa = smderFloat->mantissa & 0x0FFF;
        *ieeeMder = (exponent | mantissa);
    }
    else
    {
        if (smderFloat->specialValue == MDER_NAN)
        {
            *ieeeMder = 0x07FF;
        }
        else if (smderFloat->specialValue == MDER_PINF)
        {
            *ieeeMder = 0x07FE;
        }
        else if (smderFloat->specialValue == MDER_NINF)
        {
            *ieeeMder = 0x0802;
        }
        else if (smderFloat->specialValue == MDER_NRES)
        {
            *ieeeMder = 0x0801;
        }
        else if (smderFloat->specialValue == MDER_RSVD)
        {
            *ieeeMder = 0x0800;
        }
    }
    return true;
}

unsigned long getIeeeFloatFromString(char* floatAsString)
{
    unsigned long ieeeFloat = 0;
    if (floatAsString != NULL)
    {
        int length = (int)strlen(floatAsString);
        int sign = (floatAsString[0] == '-') ? -1 : 1;
        int i = (sign == -1) ? 1 : 0;
        long int j = -1;
        for (i = 0; i < length; i++)
        {
            if (floatAsString[i] == '.')
            {
                j = 0;
                continue;
            }
            ieeeFloat = ieeeFloat * 10 + (floatAsString[i] - 0x30);
            if (j >= 0)
            {
                j++;
            }
        }
        if (j <= 0)
        {
            ieeeFloat = ((ieeeFloat * sign) * 0xFF000000); // negate only the lower 24 bits if negative
        }
        if (j > 0)
        {
            j = -j; // The number of decimal places
            j = (j << 24);
            ieeeFloat = ((j & 0xFF000000) | ieeeFloat);
        }
    }
    return ieeeFloat;
}

unsigned long getIeeeSFloatFromString(char* floatAsString)
{
    unsigned short ieeeFloat = 0;
    if (floatAsString != NULL)
    {
        int length = (int)strlen(floatAsString);
        int sign = (floatAsString[0] == '-') ? -1 : 1;
        int i = (sign == -1) ? 1 : 0;
        short int j = -1;
        for (i = 0; i < length; i++)
        {
            if (floatAsString[i] == '.')
            {
                j = 0;
                continue;
            }
            ieeeFloat = ieeeFloat * 10 + (floatAsString[i] - 0x30);
            if (j >= 0)
            {
                j++;
            }
        }
        if (j <= 0)
        {
            ieeeFloat = ((ieeeFloat * sign) * 0xF000); // negate only the lower 12 bits if negative
        }
        if (j > 0)
        {
            j = -j; // The number of decimal places
            j = (j << 12);
            ieeeFloat = ((j & 0xF000) | ieeeFloat);
        }
    }
    return ieeeFloat;
}

bool getMderFloatFromString(s_MderFloat *mderFloat, char* floatAsString)
{
    if (mderFloat->mderFloatType != MDER_FLOAT && mderFloat->mderFloatType != MDER_SFLOAT)
    {
        printf("ERROR! Float type not specified by the caller!\n");
        return false;
    }
    if (floatAsString != NULL && mderFloat != NULL)
    {
        if (strcmp(floatAsString, "NAN") == 0)
        {
            mderFloat->specialValue = MDER_NAN;
            mderFloat->exponent = 0;
            mderFloat->mantissa = (mderFloat->mderFloatType == MDER_SFLOAT) ? 0x07FF : 0x007FFFFF;
        }
        else if(strcmp(floatAsString, "PINF") == 0)
        {
            mderFloat->specialValue = MDER_PINF;
            mderFloat->exponent = 0;
            mderFloat->mantissa = (mderFloat->mderFloatType == MDER_SFLOAT) ? 0x07FE : 0x007FFFFE;
        }
        else if (strcmp(floatAsString, "NINF") == 0)
        {
            mderFloat->specialValue = MDER_NINF;
            mderFloat->exponent = 0;
            mderFloat->mantissa = (mderFloat->mderFloatType == MDER_SFLOAT) ? 0x0802 : 0x00800002;
        }
        else if (strcmp(floatAsString, "NRES") == 0)
        {
            mderFloat->specialValue = MDER_NRES;
            mderFloat->exponent = 0;
            mderFloat->mantissa = (mderFloat->mderFloatType == MDER_SFLOAT) ? 0x0801 : 0x00800001;
        }
        else if (strcmp(floatAsString, "RSVD") == 0)
        {
            mderFloat->specialValue = MDER_RSVD;
            mderFloat->exponent = 0;
            mderFloat->mantissa = (mderFloat->mderFloatType == MDER_SFLOAT) ? 0x0800 : 0x00800000;
        }
        else
        {
            mderFloat->specialValue = MDER_NUMBER;
            int length = (int)strlen(floatAsString);
            int sign = (floatAsString[0] == '-') ? -1 : 1;
            int i = (sign == -1) ? 1 : 0;
            short j = -1;
            mderFloat->mantissa = 0;
            for (i = 0; i < length; i++)
            {
                if (floatAsString[i] == '.')
                {
                    j = 0;
                    continue;
                }
                if ((floatAsString[i] - 0x30) < 0 || (floatAsString[i] - 0x30) > 9)
                {
                    return false;
                }
                mderFloat->mantissa = mderFloat->mantissa * 10 + (floatAsString[i] - 0x30);
                if (j >= 0)
                {
                    j++;
                }
            }
            mderFloat->mantissa = mderFloat->mantissa * sign;
            if (mderFloat->mderFloatType == MDER_SFLOAT)
            {
                if ((mderFloat->mantissa > 2047) || (mderFloat->mantissa < -2048))
                {
                    printf("Input value cannot be represented in an Mder SFLOAT - overflow or underflow. Try FLOAT.\n");
                    mderFloat->specialValue = MDER_NRES;
                    mderFloat->exponent = 0;
                    mderFloat->mantissa = 0x0801;
                    return true;
                }
            }
            else if (mderFloat->mderFloatType == MDER_FLOAT)
            {
                if ((mderFloat->mantissa > 8388607) || (mderFloat->mantissa < -8388608))
                {
                    printf("Input value cannot be represented in an Mder FLOAT - overflow or underflow\n");
                    mderFloat->specialValue = MDER_NRES;
                    mderFloat->exponent = 0;
                    mderFloat->mantissa = 0x00800001;
                    return true;
                }
            }
            if (j < 0)
            {
                mderFloat->exponent = 0;
            }
            if (j > 0)
            {
                mderFloat->exponent = -j; // The number of decimal places (needs to be negated)
            }
            if (j == 0)             // The user entered a decimal point with no decimal entries, for example, '92.'
            {
                mderFloat->mantissa = mderFloat->mantissa * 10;
                mderFloat->exponent = -1; // The number of decimal places (needs to be negated)
            }
        }
        return true;
    }
    return false;
}
