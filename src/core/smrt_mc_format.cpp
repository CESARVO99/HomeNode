/**
 * @file    smrt_mc_format.cpp
 * @brief   Utilidades de conversion de tipos de datos y manipulacion de strings
 * @project HOMENODE
 * @version 1.0.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_mc_format.h"
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Byte swapping functions
//-----------------------------------------------------------------------------

/**
 * @brief  Swaps bytes in a 2-byte buffer (in-place)
 * @param  u16val  Pointer to 2-byte buffer to be swapped
 * @return void
 */
#ifndef exclude_smrtTypeSwap16
void smrtTypeSwap16(uint8 *u16val) {
    uint8 b = u16val[0];
    u16val[0] = u16val[1];
    u16val[1] = b;
}
#endif // exclude_smrtTypeSwap16

/**
 * @brief  Swaps bytes in a 4-byte buffer (in-place)
 * @param  u32val  Pointer to 4-byte buffer to be swapped
 * @return void
 */
#ifndef exclude_smrtTypeSwap32
void smrtTypeSwap32(uint8 *u32val) {
    uint8 b = u32val[0];
    u32val[0] = u32val[3];
    u32val[3] = b;
    b = u32val[1];
    u32val[1] = u32val[2];
    u32val[2] = b;
}
#endif // exclude_smrtTypeSwap32

//-----------------------------------------------------------------------------
// String case conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts an ASCII string to uppercase
 * @param  str     String buffer to convert
 * @param  strLen  Length in characters of the string
 * @return void
 */
#ifndef exclude_smrtTypeSetUpper
void smrtTypeSetUpper(char *str, uint32 strLen) {
    uint32 i;
    for (i = 0; i < strLen; i++) {
        str[i] = SMRT_TO_UPPER(str[i]);
    }
}
#endif // exclude_smrtTypeSetUpper

/**
 * @brief  Converts an ASCII string to lowercase
 * @param  str     String buffer to convert
 * @param  strLen  Length in characters of the string
 * @return void
 */
#ifndef exclude_smrtTypeSetLower
void smrtTypeSetLower(char *str, uint32 strLen) {
    uint32 i;
    for (i = 0; i < strLen; i++) {
        str[i] = SMRT_TO_LOWER(str[i]);
    }
}
#endif // exclude_smrtTypeSetLower

//-----------------------------------------------------------------------------
// ON/OFF conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts a boolean value into "ON" or "OFF" string
 * @param  str    Destination string buffer
 * @param  value  Boolean value to convert
 * @return String length (2 for "ON", 3 for "OFF")
 */
#ifndef exclude_smrtTypeSetOnOff
uint8 smrtTypeSetOnOff(char *str, bit value) {
    str[0] = 'O';
    if (value) {
        str[1] = 'N';
        return (uint8)2;
    } else {
        str[1] = 'F';
        str[2] = 'F';
        return (uint8)3;
    }
}
#endif // exclude_smrtTypeSetOnOff

/**
 * @brief  Validates "ON"/"OFF" string and extracts boolean value
 * @param  str     String to check
 * @param  pValue  Pointer to store the boolean result (1=ON, 0=OFF)
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckOnOff
bit smrtTypeCheckOnOff(const char *str, uint8 *pValue) {
    if (str[0] != 'O') {
        return (bit)false;
    }
    if (str[1] == 'N') {
        (*pValue) = true;
        return (bit)true;
    } else if ((str[1] == 'F') && (str[2] == 'F')) {
        (*pValue) = false;
        return (bit)true;
    }
    return (bit)false;
}
#endif // exclude_smrtTypeCheckOnOff

//-----------------------------------------------------------------------------
// TRUE/FALSE conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts a boolean value into "TRUE" or "FALSE" string
 * @param  str    Destination string buffer
 * @param  value  Boolean value to convert
 * @return String length (4 for "TRUE", 5 for "FALSE")
 */
#ifndef exclude_smrtTypeSetTrueFalse
uint8 smrtTypeSetTrueFalse(char *str, bit value) {
    if (value) {
        str[0] = 'T';
        str[1] = 'R';
        str[2] = 'U';
        str[3] = 'E';
        return (uint8)4;
    } else {
        str[0] = 'F';
        str[1] = 'A';
        str[2] = 'L';
        str[3] = 'S';
        str[4] = 'E';
        return (uint8)5;
    }
}
#endif // exclude_smrtTypeSetTrueFalse

/**
 * @brief  Validates "TRUE"/"FALSE" string and extracts boolean value
 * @param  str     String to check
 * @param  pValue  Pointer to store the boolean result
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckTrueFalse
bit smrtTypeCheckTrueFalse(const char *str, uint8 *pValue) {
    if ((str[0] == 'T') && (str[1] == 'R') && (str[2] == 'U') && (str[3] == 'E')) {
        (*pValue) = true;
        return (bit)true;
    } else if ((str[0] == 'F') && (str[1] == 'A') && (str[2] == 'L') && (str[3] == 'S') && (str[4] == 'E')) {
        (*pValue) = false;
        return (bit)true;
    }
    return (bit)false;
}
#endif // exclude_smrtTypeCheckTrueFalse

//-----------------------------------------------------------------------------
// Unsigned decimal conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts an unsigned integer into its ASCII decimal representation.
 *         Fills the string least-significant-digit first.
 * @param  str     Destination string buffer
 * @param  value   Unsigned integer value to convert
 * @param  digits  Number of decimal digits in output
 * @return String length (equals digits)
 */
#ifndef exclude_smrtTypeSetUnsigned
uint8 smrtTypeSetUnsigned(char *str, uint32 value, uint8 digits) {
    uint8 i;
    for (i = digits - 1; i < 0xFF; i--) {
        str[i] = '0' + (char)(value % 10);
        value = value / 10;
    }
    return digits;
}
#endif // exclude_smrtTypeSetUnsigned

/**
 * @brief  Validates unsigned decimal string and extracts numeric value.
 *         Checks character validity and performs range validation.
 * @param  str     Numeric string to check
 * @param  pValue  Pointer to store the resulting value
 * @param  digits  Number of digits expected in string
 * @param  max     Maximum valid value for range check
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckUnsigned
bit smrtTypeCheckUnsigned(const char *str, uint32 *pValue, uint8 digits, uint32 max) {
    uint32 base = 1;
    uint8 i;

    // Check character validity during forward string scan
    for (i = 0; i < digits; i++) {
        if ((str[i] < '0') || (str[i] > '9')) {
            return (bit)false;
        }
    }

    (*pValue) = 0;

    // Convert to integer during backward scan
    for (i--; i < 0xFF; i--) {
        (*pValue) += base * (str[i] - '0');
        base *= 10;
    }
    return (bit)((*pValue) <= max);
}
#endif // exclude_smrtTypeCheckUnsigned

//-----------------------------------------------------------------------------
// Signed decimal conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts a signed integer into its ASCII decimal representation.
 *         Prepends '+' or '-' sign character.
 * @param  str     Destination string buffer
 * @param  value   Signed integer value to convert
 * @param  digits  Number of decimal digits (sign excluded)
 * @return String length including sign character
 */
#ifndef exclude_smrtTypeSetSigned
uint8 smrtTypeSetSigned(char *str, int32 value, uint8 digits) {
    uint32 abs_value;
    uint8 is_negative;

    is_negative = (value < 0) ? 1 : 0;
    abs_value = is_negative ? (uint32)(-value) : (uint32)value;
    str[0] = is_negative ? '-' : '+';
    return (uint8)(smrtTypeSetUnsigned(&str[1], abs_value, digits) + 1);
}
#endif // exclude_smrtTypeSetSigned

/**
 * @brief  Validates signed decimal string and extracts numeric value.
 *         Expects leading '+' or '-' sign character.
 * @param  str     Numeric signed string to check
 * @param  pValue  Pointer to store the resulting value
 * @param  digits  Number of digits expected (sign excluded)
 * @param  max     Maximum valid value for range check
 * @param  min     Minimum valid value for range check
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckSigned
bit smrtTypeCheckSigned(const char *str, int32 *pValue, uint8 digits, int32 max, int32 min) {
    uint32 u_value;
    int32 i_value;

    // Check sign character
    if ((str[0] != '-') && (str[0] != '+')) {
        return (bit)false;
    }

    // Check and extract absolute value
    if (!smrtTypeCheckUnsigned(&str[1], &u_value, digits, 0xFFFFFFFF)) {
        return (bit)false;
    }

    // Apply sign to absolute integer
    i_value = (int32)u_value;
    if (str[0] == '-') {
        i_value = -i_value;
    }

    // Check range
    if ((i_value > max) || (i_value < min)) {
        return (bit)false;
    }

    (*pValue) = i_value;
    return (bit)true;
}
#endif // exclude_smrtTypeCheckSigned

//-----------------------------------------------------------------------------
// Hexadecimal conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts a hex value into its ASCII hexadecimal representation
 * @param  str     Destination string buffer
 * @param  value   Hex value to convert
 * @param  digits  Number of hex digits in output
 * @return String length
 */
#ifndef exclude_smrtTypeSetHexadecimal
uint8 smrtTypeSetHexadecimal(char *str, uint32 value, uint8 digits) {
    uint8 i, j;
    for (i = 0; i < digits; i++) {
        if (i < 8) {
            j = (uint8)(value >> (i << 2)) & 0x0F;
            if (j <= 9) {
                str[digits - i - 1] = '0' + j;
            } else {
                str[digits - i - 1] = 'A' + (j - 10);
            }
        } else {
            str[digits - i - 1] = '0';
        }
    }
    return i;
}
#endif // exclude_smrtTypeSetHexadecimal

/**
 * @brief  Validates hexadecimal string and extracts numeric value.
 *         Accepts characters 0-9 and A-F (uppercase only).
 * @param  str     Hex string to check
 * @param  pValue  Pointer to store the resulting value
 * @param  digits  Number of hex digits expected
 * @param  max     Maximum valid value for range check
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckHexadecimal
bit smrtTypeCheckHexadecimal(const char *str, uint32 *pValue, uint8 digits, uint32 max) {
    uint8 i;
    uint32 value = 0;

    for (i = 0; i < digits; i++) {
        if ((str[i] < '0') || (str[i] > 'F') || ((str[i] > '9') && (str[i] < 'A'))) {
            return (bit)false;  // Non-hex character detected
        }
        value <<= 4;
        if (str[i] <= '9') {
            value |= str[i] - '0';
        } else {
            value |= (str[i] - 'A') + 10;
        }
    }

    // If digits > 8, no range check is performed
    if ((digits <= 8) && (value > max)) {
        return (bit)false;
    }

    (*pValue) = value;
    return (bit)true;
}
#endif // exclude_smrtTypeCheckHexadecimal

//-----------------------------------------------------------------------------
// Fixed-point unsigned decimal conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts an unsigned fixed-point value into decimal string (XX.XXX).
 *         Fills string least-significant-digit first.
 * @param  str      Destination string buffer
 * @param  value    Unsigned integer value to convert
 * @param  idigits  Number of integer digits
 * @param  ddigits  Number of decimal digits
 * @return String length (idigits + 1 + ddigits)
 */
#ifndef exclude_smrtTypeSetFixedUnsigned
uint8 smrtTypeSetFixedUnsigned(char *str, uint32 value, uint8 idigits, uint8 ddigits) {
    uint8 i;

    // Fill decimal digits (right of the point)
    for (i = ddigits + idigits; i > idigits; i--) {
        str[i] = '0' + (char)(value % 10);
        value = value / 10;
    }
    str[i--] = '.';

    // Fill integer digits (left of the point)
    for (; i < 0xFF; i--) {
        str[i] = '0' + (char)(value % 10);
        value = value / 10;
    }
    return (idigits + ddigits + 1);
}
#endif // exclude_smrtTypeSetFixedUnsigned

/**
 * @brief  Validates unsigned decimal string (XX.XXX) and extracts value.
 *         Checks digit validity and decimal point position.
 * @param  str      Decimal string to check
 * @param  pValue   Pointer to store the resulting value
 * @param  idigits  Number of integer digits expected
 * @param  ddigits  Number of decimal digits expected
 * @param  max      Maximum valid value for range check
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckFixedUnsigned
/**
 * @brief  Validates that a range of characters are ASCII digits ('0'-'9')
 * @param  str    String buffer to validate
 * @param  start  Start index (inclusive)
 * @param  end    End index (exclusive)
 * @return true if all characters are digits, false otherwise
 */
static bit smrtTypeValidateDigitRange(const char *str, uint8 start, uint8 end) {
    uint8 i;
    for (i = start; i < end; i++) {
        if ((str[i] < '0') || (str[i] > '9')) {
            return (bit)false;
        }
    }
    return (bit)true;
}

/**
 * @brief  Converts a range of digit characters to an integer value
 * @param  str    String buffer containing digits
 * @param  start  Start index (inclusive)
 * @param  end    End index (exclusive)
 * @return Converted unsigned integer value
 */
static uint32 smrtTypeConvertDigitRange(const char *str, uint8 start, uint8 end) {
    uint32 value = 0;
    uint32 base = 1;
    uint8 i;
    for (i = end - 1; i >= start && i < 0xFF; i--) {
        value += base * (str[i] - '0');
        base *= 10;
    }
    return value;
}

bit smrtTypeCheckFixedUnsigned(const char *str, uint32 *pValue, uint8 idigits, uint8 ddigits, uint32 max) {
    // Validate decimal digits (after the point)
    if (!smrtTypeValidateDigitRange(str, idigits + 1, idigits + 1 + ddigits)) {
        return (bit)false;
    }

    // Validate decimal point
    if (str[idigits] != '.') {
        return (bit)false;
    }

    // Validate integer digits (before the point)
    if (!smrtTypeValidateDigitRange(str, 0, idigits)) {
        return (bit)false;
    }

    // Convert: integer digits + decimal digits (skip the point)
    (*pValue) = smrtTypeConvertDigitRange(str, 0, idigits);
    uint32 dec_base = 1;
    uint8 k;
    for (k = 0; k < ddigits; k++) { dec_base *= 10; }
    (*pValue) = (*pValue) * dec_base + smrtTypeConvertDigitRange(str, idigits + 1, idigits + 1 + ddigits);

    return (bit)((*pValue) <= max);
}
#endif // exclude_smrtTypeCheckFixedUnsigned

//-----------------------------------------------------------------------------
// Fixed-point signed decimal conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts a signed fixed-point value into decimal string (+XX.XXX)
 * @param  str      Destination string buffer
 * @param  value    Signed integer value to convert
 * @param  idigits  Number of integer digits
 * @param  ddigits  Number of decimal digits
 * @return String length (1 + idigits + 1 + ddigits)
 */
#ifndef exclude_smrtTypeSetFixedSigned
uint8 smrtTypeSetFixedSigned(char *str, int32 value, uint8 idigits, uint8 ddigits) {
    if (value >= 0) {
        str[0] = '+';
    } else {
        str[0] = '-';
        value = -value;
    }
    return smrtTypeSetFixedUnsigned(&str[1], (uint32)value, idigits, ddigits) + 1;
}
#endif // exclude_smrtTypeSetFixedSigned

/**
 * @brief  Validates signed decimal string (+XX.XXX) and extracts value
 * @param  str      Decimal string to check
 * @param  pValue   Pointer to store the resulting value
 * @param  idigits  Number of integer digits expected
 * @param  ddigits  Number of decimal digits expected
 * @param  max      Maximum valid value for range check
 * @param  min      Minimum valid value for range check
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckFixedSigned
bit smrtTypeCheckFixedSigned(const char *str, int32 *pValue, uint8 idigits, uint8 ddigits, int32 max, int32 min) {
    uint32 u_value;

    if (!smrtTypeCheckFixedUnsigned(&str[1], &u_value, idigits, ddigits, 0xFFFFFFFF)) {
        return (bit)false;
    }
    *pValue = (int32)u_value;
    if (str[0] == '-') {
        *pValue = -(*pValue);
    } else if (str[0] != '+') {
        return (bit)false;
    }
    if (((*pValue) < min) || ((*pValue) > max)) {
        return (bit)false;
    }
    return (bit)true;
}
#endif // exclude_smrtTypeCheckFixedSigned

//-----------------------------------------------------------------------------
// IP address conversion functions
//-----------------------------------------------------------------------------

/**
 * @brief  Converts a 32-bit IP address (0xb0b1b2b3) into "XXX.XXX.XXX.XXX" string
 * @param  str   Destination string buffer (min 16 bytes)
 * @param  addr  IP address as 32-bit integer
 * @return String length (always 15)
 */
#ifndef exclude_smrtTypeSetIP
uint8 smrtTypeSetIP(char *str, uint32 addr) {
    uint8 i, byte_pos;

    for (i = 0, byte_pos = 0; byte_pos < 4; byte_pos++) {
        uint8 byte_value = (uint8)(0x0FF & (addr >> ((4 - byte_pos - 1) << 3)));
        i += smrtTypeSetUnsigned(&str[i], byte_value, 3);
        str[i++] = '.';
    }
    return i;
}
#endif // exclude_smrtTypeSetIP

/**
 * @brief  Validates IP address string "XXX.XXX.XXX.XXX" and extracts 32-bit value
 * @param  str    IP address string to check
 * @param  pAddr  Pointer to store the 32-bit IP address
 * @return true on success, false on error
 */
#ifndef exclude_smrtTypeCheckIP
bit smrtTypeCheckIP(const char *str, uint32 *pAddr) {
    uint8 i;
    uint32 addr = 0, byte32;

    for (i = 0; i < 15; i += 4) {
        if (!smrtTypeCheckUnsigned(&str[i], &byte32, 3, 255)) {
            return (bit)false;
        }
        addr = addr << 8;
        addr = addr + byte32;
    }

    (*pAddr) = addr;
    return (bit)true;
}
#endif // exclude_smrtTypeCheckIP

//-----------------------------------------------------------------------------
// Memory fill functions
//-----------------------------------------------------------------------------

/**
 * @brief  Fills a memory region using a 16-bit pattern
 * @param  buffer  Memory buffer to fill
 * @param  value   16-bit pattern to use
 * @param  count   Number of words (2-byte units) to fill
 * @return void
 */
#ifndef exclude_smrtTypeMemFill16
void smrtTypeMemFill16(uint16 *buffer, uint16 value, uint32 count) {
    while (count--) {
        (*buffer) = value;
        buffer = &buffer[1];
    }
}
#endif // exclude_smrtTypeMemFill16

/**
 * @brief  Fills a memory region using a 32-bit pattern
 * @param  buffer  Memory buffer to fill
 * @param  value   32-bit pattern to use
 * @param  count   Number of double-words (4-byte units) to fill
 * @return void
 */
#ifndef exclude_smrtTypeMemFill32
void smrtTypeMemFill32(uint32 *buffer, uint32 value, uint32 count) {
    while (count--) {
        (*buffer) = value;
        buffer = &buffer[1];
    }
}
#endif // exclude_smrtTypeMemFill32

//-----------------------------------------------------------------------------
// String copy functions
//-----------------------------------------------------------------------------

/**
 * @brief  Copies a string until null terminator is found
 * @param  dst  Destination string buffer
 * @param  src  Source string buffer
 * @return Number of characters copied (excluding null terminator)
 */
#ifndef exclude_smrtTypeStrCpy
uint32 smrtTypeStrCpy(char *dst, const char *src) {
    uint32 i;
    for (i = 0; src[i] != 0; i++) {
        dst[i] = src[i];
    }
    dst[i] = src[i];  // Copy the '\0' end-of-string character
    return i;
}
#endif // exclude_smrtTypeStrCpy

/**
 * @brief  Copies a string with maximum length limit
 * @param  dst      Destination string buffer
 * @param  src      Source string buffer
 * @param  max_len  Maximum number of characters to copy
 * @return Number of characters copied (excluding null terminator if present)
 */
#ifndef exclude_smrtTypeStrCpy
uint32 smrtTypeStrnCpy(char *dst, const char *src, uint32 max_len) {
    uint32 i;

    for (i = 0; (i < max_len) && (src[i] != 0); i++) {
        dst[i] = src[i];
    }
    if (i < max_len) {
        dst[i] = 0;  // Copy the '\0' end-of-string character
    }

    return i;
}
#endif // exclude_smrtTypeStrCpy
