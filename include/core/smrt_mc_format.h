/**
 * @file    smrt_mc_format.h
 * @brief   Tipos de datos custom, macros de conversion y prototipos de utilidades
 * @project HOMENODE
 * @version 1.0.0
 */

#ifndef SMRT_MC_FORMAT_H
#define SMRT_MC_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * CUSTOM NON-AMBIGUOUS NUMERIC TYPE DEFINITIONS *
 ************************************************/

#ifndef _int8_
    #define _int8_
    typedef char            int8;       /**< Platform independent 8-bit signed integer */
#endif // _int8_

#ifndef _uint8_
    #define _uint8_
    typedef unsigned char   uint8;      /**< Platform independent 8-bit unsigned integer */
#endif // _uint8_

#ifndef _int16_
    #define _int16_
    typedef short           int16;      /**< Platform independent 16-bit signed integer */
#endif // _int16_

#ifndef _uint16_
    #define _uint16_
    typedef unsigned short  uint16;     /**< Platform independent 16-bit unsigned integer */
#endif // _uint16_

#ifndef _int32_
    #define _int32_
    typedef long            int32;      /**< Platform independent 32-bit signed integer */
#endif // _int32_

#ifndef _uint32_
    #define _uint32_
    typedef unsigned long   uint32;     /**< Platform independent 32-bit unsigned integer */
#endif // _uint32_

#ifndef NULL
    #define NULL 0
#endif // NULL

/***********************************************
 * CUSTOM BOOLEAN TYPE AND CONSTANT DEFINITIONS *
 ***********************************************/

#if !defined(_bit_)
    #define _bit_
    typedef uint8 bit;                  /**< Boolean type (1-bit on C51, 8-bit otherwise) */
#endif // _bit_

#ifndef true
    #define true ((bit)1)               /**< Boolean TRUE constant */
#endif // true

#ifndef false
    #define false ((bit)0)              /**< Boolean FALSE constant */
#endif // false

/****************************************************
 * CUSTOM DATA TYPE BYTE SWAPPING MACRO DEFINITIONS *
 ****************************************************/

/** @brief Performs byte swapping over a 16-bit data */
#define SMRT_TYPE_SWAP_16(val16) \
    (((val16 >> 8) & 0x0FF) | ((val16 & 0x0FF) << 8))

/** @brief Performs byte swapping over a 32-bit data */
#define SMRT_TYPE_SWAP_32(val32) \
    (((val32 >> 24) & 0x0FF) | ((val32 >> 8) & 0x0FF00) | \
     ((val32 << 8) & 0x0FF0000) | ((val32 & 0x0FF) << 24))

/*************************************************************
 * CHARACTER CONVERSION MACRO DEFINITIONS                     *
 *************************************************************/

/** @brief Converts a character to uppercase */
#define SMRT_TO_UPPER(x)    ((((x) >= 'a') && ((x) <= 'z')) ? ((x) & 0xDF) : (x))

/** @brief Converts a character to lowercase */
#define SMRT_TO_LOWER(x)    ((((x) >= 'A') && ((x) <= 'Z')) ? ((x) | 0x20) : (x))

/*************************************************************
 * MIN AND MAX MACRO DEFINITIONS                             *
 *************************************************************/

/** @brief Returns the minimum of two values */
#define SMRT_MIN(X, Y)      (((X) <= (Y)) ? (X) : (Y))

/** @brief Returns the maximum of two values */
#define SMRT_MAX(X, Y)      (((X) >= (Y)) ? (X) : (Y))

/*************************************************************
 * COMPILE-TIME FUNCTION EXCLUSION DEFINES                   *
 *                                                           *
 * Define these macros to exclude unused functions from the   *
 * build, reducing firmware size.                            *
 * Note: In UNIT_TEST mode all functions are compiled to     *
 * allow full test coverage.                                 *
 *************************************************************/
#ifndef UNIT_TEST
#define exclude_smrtTypeSwap16
#define exclude_smrtTypeSwap32
#define exclude_smrtTypeSetUpper
#define exclude_smrtTypeSetLower
#define exclude_smrtTypeSetTrueFalse
#define exclude_smrtTypeCheckTrueFalse
#define exclude_smrtTypeSetSigned
#define exclude_smrtTypeCheckSigned
#define exclude_smrtTypeCheckUnsigned
#define exclude_smrtTypeSetFixedUnsigned
#define exclude_smrtTypeCheckFixedUnsigned
#define exclude_smrtTypeSetFixedSigned
#define exclude_smrtTypeCheckFixedSigned
#define exclude_smrtTypeSetIP
#define exclude_smrtTypeCheckIP
#define exclude_smrtTypeMemFill16
#define exclude_smrtTypeMemFill32
#define exclude_smrtTypeSetOnOff
#define exclude_smrtTypeCheckOnOff
#define exclude_smrtTypeSetUnsigned
// #define exclude_smrtTypeSetHexadecimal
// #define exclude_smrtTypeCheckHexadecimal
#define exclude_smrtTypeStrCpy
#endif // UNIT_TEST

/**********************************
 * BYTE SWAPPING FUNCTION PROTOTYPES *
 **********************************/

/**
 * @brief  Swaps bytes in a 2-byte buffer (in-place)
 * @param  u16val  Pointer to 2-byte buffer to be swapped
 * @return void
 */
void smrtTypeSwap16(uint8 *u16val);

/**
 * @brief  Swaps bytes in a 4-byte buffer (in-place)
 * @param  u32val  Pointer to 4-byte buffer to be swapped
 * @return void
 */
void smrtTypeSwap32(uint8 *u32val);

/****************************************************************
 * STRING CONVERSION FUNCTION PROTOTYPES                        *
 ****************************************************************/

/**
 * @brief  Converts an ASCII string to uppercase
 * @param  str     String buffer to convert
 * @param  strLen  Length in characters of the string
 * @return void
 */
void smrtTypeSetUpper(char *str, uint32 strLen);

/**
 * @brief  Converts an ASCII string to lowercase
 * @param  str     String buffer to convert
 * @param  strLen  Length in characters of the string
 * @return void
 */
void smrtTypeSetLower(char *str, uint32 strLen);

/****************************************************************
 * ON/OFF CONVERSION FUNCTION PROTOTYPES                        *
 ****************************************************************/

/**
 * @brief  Converts a boolean value into "ON" or "OFF" string
 * @param  str    Destination string buffer
 * @param  value  Boolean value to convert
 * @return String length (2 for "ON", 3 for "OFF")
 */
uint8 smrtTypeSetOnOff(char *str, bit value);

/**
 * @brief  Validates "ON"/"OFF" string and extracts boolean value
 * @param  str     String to check
 * @param  pValue  Pointer to store the boolean result (1=ON, 0=OFF)
 * @return true on success, false on error
 */
bit smrtTypeCheckOnOff(const char *str, uint8 *pValue);

/****************************************************************
 * TRUE/FALSE CONVERSION FUNCTION PROTOTYPES                    *
 ****************************************************************/

/**
 * @brief  Converts a boolean value into "TRUE" or "FALSE" string
 * @param  str    Destination string buffer
 * @param  value  Boolean value to convert
 * @return String length (4 for "TRUE", 5 for "FALSE")
 */
uint8 smrtTypeSetTrueFalse(char *str, bit value);

/**
 * @brief  Validates "TRUE"/"FALSE" string and extracts boolean value
 * @param  str     String to check
 * @param  pValue  Pointer to store the boolean result
 * @return true on success, false on error
 */
bit smrtTypeCheckTrueFalse(const char *str, uint8 *pValue);

/****************************************************************
 * UNSIGNED DECIMAL CONVERSION FUNCTION PROTOTYPES              *
 ****************************************************************/

/**
 * @brief  Converts an unsigned integer into its ASCII decimal representation
 * @param  str     Destination string buffer
 * @param  value   Unsigned integer value to convert
 * @param  digits  Number of decimal digits in output
 * @return String length
 */
uint8 smrtTypeSetUnsigned(char *str, uint32 value, uint8 digits);

/**
 * @brief  Validates unsigned decimal string and extracts numeric value
 * @param  str     Numeric string to check
 * @param  pValue  Pointer to store the resulting value
 * @param  digits  Number of digits expected in string
 * @param  max     Maximum valid value for range check
 * @return true on success, false on error
 */
bit smrtTypeCheckUnsigned(const char *str, uint32 *pValue, uint8 digits, uint32 max);

/****************************************************************
 * SIGNED DECIMAL CONVERSION FUNCTION PROTOTYPES                *
 ****************************************************************/

/**
 * @brief  Converts a signed integer into its ASCII decimal representation
 * @param  str     Destination string buffer
 * @param  value   Signed integer value to convert
 * @param  digits  Number of decimal digits (sign excluded)
 * @return String length including sign character
 */
uint8 smrtTypeSetSigned(char *str, int32 value, uint8 digits);

/**
 * @brief  Validates signed decimal string and extracts numeric value
 * @param  str     Numeric signed string to check
 * @param  pValue  Pointer to store the resulting value
 * @param  digits  Number of digits expected (sign excluded)
 * @param  max     Maximum valid value for range check
 * @param  min     Minimum valid value for range check
 * @return true on success, false on error
 */
bit smrtTypeCheckSigned(const char *str, int32 *pValue, uint8 digits, int32 max, int32 min);

/****************************************************************
 * HEXADECIMAL CONVERSION FUNCTION PROTOTYPES                   *
 ****************************************************************/

/**
 * @brief  Converts a hex value into its ASCII hexadecimal representation
 * @param  str     Destination string buffer
 * @param  value   Hex value to convert
 * @param  digits  Number of hex digits in output
 * @return String length
 */
uint8 smrtTypeSetHexadecimal(char *str, uint32 value, uint8 digits);

/**
 * @brief  Validates hexadecimal string and extracts numeric value
 * @param  str     Hex string to check
 * @param  pValue  Pointer to store the resulting value
 * @param  digits  Number of hex digits expected
 * @param  max     Maximum valid value for range check
 * @return true on success, false on error
 */
bit smrtTypeCheckHexadecimal(const char *str, uint32 *pValue, uint8 digits, uint32 max);

/****************************************************************
 * FIXED-POINT DECIMAL CONVERSION FUNCTION PROTOTYPES           *
 ****************************************************************/

/**
 * @brief  Converts an unsigned fixed-point value into decimal string (XX.XXX)
 * @param  str      Destination string buffer
 * @param  value    Unsigned integer value to convert
 * @param  idigits  Number of integer digits
 * @param  ddigits  Number of decimal digits
 * @return String length (idigits + 1 + ddigits)
 */
uint8 smrtTypeSetFixedUnsigned(char *str, uint32 value, uint8 idigits, uint8 ddigits);

/**
 * @brief  Validates unsigned decimal string (XX.XXX) and extracts value
 * @param  str      Decimal string to check
 * @param  pValue   Pointer to store the resulting value
 * @param  idigits  Number of integer digits expected
 * @param  ddigits  Number of decimal digits expected
 * @param  max      Maximum valid value for range check
 * @return true on success, false on error
 */
bit smrtTypeCheckFixedUnsigned(const char *str, uint32 *pValue, uint8 idigits, uint8 ddigits, uint32 max);

/**
 * @brief  Converts a signed fixed-point value into decimal string (+XX.XXX)
 * @param  str      Destination string buffer
 * @param  value    Signed integer value to convert
 * @param  idigits  Number of integer digits
 * @param  ddigits  Number of decimal digits
 * @return String length (1 + idigits + 1 + ddigits)
 */
uint8 smrtTypeSetFixedSigned(char *str, int32 value, uint8 idigits, uint8 ddigits);

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
bit smrtTypeCheckFixedSigned(const char *str, int32 *pValue, uint8 idigits, uint8 ddigits, int32 max, int32 min);

/****************************************************************
 * IP ADDRESS CONVERSION FUNCTION PROTOTYPES                    *
 ****************************************************************/

/**
 * @brief  Converts a 32-bit IP address (0xb0b1b2b3) into "XXX.XXX.XXX.XXX" string
 * @param  str   Destination string buffer (min 16 bytes)
 * @param  addr  IP address as 32-bit integer
 * @return String length (always 15)
 */
uint8 smrtTypeSetIP(char *str, uint32 addr);

/**
 * @brief  Validates IP address string "XXX.XXX.XXX.XXX" and extracts 32-bit value
 * @param  str    IP address string to check
 * @param  pAddr  Pointer to store the 32-bit IP address
 * @return true on success, false on error
 */
bit smrtTypeCheckIP(const char *str, uint32 *pAddr);

/************************************************************
 * MEMORY BUFFER MANAGEMENT FUNCTION PROTOTYPES             *
 ************************************************************/

/**
 * @brief  Fills a memory region using a 16-bit pattern
 * @param  buffer  Memory buffer to fill
 * @param  value   16-bit pattern to use
 * @param  count   Number of words (2-byte units) to fill
 * @return void
 */
void smrtTypeMemFill16(uint16 *buffer, uint16 value, uint32 count);

/**
 * @brief  Fills a memory region using a 32-bit pattern
 * @param  buffer  Memory buffer to fill
 * @param  value   32-bit pattern to use
 * @param  count   Number of double-words (4-byte units) to fill
 * @return void
 */
void smrtTypeMemFill32(uint32 *buffer, uint32 value, uint32 count);

/************************************************************
 * STRING COPY FUNCTION PROTOTYPES                          *
 ************************************************************/

/**
 * @brief  Copies a string until null terminator is found
 * @param  dst  Destination string buffer
 * @param  src  Source string buffer
 * @return Number of characters copied (excluding null terminator)
 */
uint32 smrtTypeStrCpy(char *dst, const char *src);

/**
 * @brief  Copies a string with maximum length limit
 * @param  dst      Destination string buffer
 * @param  src      Source string buffer
 * @param  max_len  Maximum number of characters to copy
 * @return Number of characters copied (excluding null terminator if present)
 */
uint32 smrtTypeStrnCpy(char *dst, const char *src, uint32 max_len);

#ifdef __cplusplus
}
#endif

#endif // SMRT_MC_FORMAT_H
