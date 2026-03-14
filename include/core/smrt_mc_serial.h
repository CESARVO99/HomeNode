/**
 * @file    smrt_mc_serial.h
 * @brief   Definiciones de caracteres ASCII y prototipos de comunicacion serial
 * @project HOMENODE
 * @version 1.0.0
 */

#ifndef SMRT_MC_SERIAL_H
#define SMRT_MC_SERIAL_H

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

//-----------------------------------------------------------------------------
// ASCII control character definitions
//-----------------------------------------------------------------------------
#define SMRT_ASCII_NULL         0x00    /**< NULL character */
#define SMRT_ASCII_CR           0x0D    /**< Carriage Return (CR) */
#define SMRT_ASCII_SPC          0x20    /**< Space character */

//-----------------------------------------------------------------------------
// ASCII symbol definitions
//-----------------------------------------------------------------------------
#define SMRT_ASCII_SUM          0x2B    /**< '+' symbol */
#define SMRT_ASCII_RES          0x2D    /**< '-' symbol */
#define SMRT_ASCII_POINT        0x2E    /**< '.' symbol */
#define SMRT_ASCII_COMMA        0x2C    /**< ',' symbol */
#define SMRT_ASCII_BAR          0x2F    /**< '/' symbol */
#define SMRT_ASCII_MIN          0x3C    /**< '<' symbol */
#define SMRT_ASCII_EQ           0x3D    /**< '=' symbol */
#define SMRT_ASCII_MAX          0x3E    /**< '>' symbol */
#define SMRT_ASCII_QUEST        0x3F    /**< '?' symbol */
#define SMRT_ASCII_UBAR         0x5F    /**< '_' symbol */

//-----------------------------------------------------------------------------
// ASCII uppercase letter definitions (A-Z)
//-----------------------------------------------------------------------------
#define SMRT_ASCII_FIRST_UPPER  0x41    /**< 'A' - First uppercase letter */
#define SMRT_ASCII_LAST_UPPER   0x5A    /**< 'Z' - Last uppercase letter */

#define SMRT_ASCII_A            0x41    /**< 'A' */
#define SMRT_ASCII_B            0x42    /**< 'B' */
#define SMRT_ASCII_C            0x43    /**< 'C' */
#define SMRT_ASCII_D            0x44    /**< 'D' */
#define SMRT_ASCII_E            0x45    /**< 'E' */
#define SMRT_ASCII_F            0x46    /**< 'F' */
#define SMRT_ASCII_G            0x47    /**< 'G' */
#define SMRT_ASCII_H            0x48    /**< 'H' */
#define SMRT_ASCII_I            0x49    /**< 'I' */
#define SMRT_ASCII_J            0x4A    /**< 'J' */
#define SMRT_ASCII_K            0x4B    /**< 'K' */
#define SMRT_ASCII_L            0x4C    /**< 'L' */
#define SMRT_ASCII_M            0x4D    /**< 'M' */
#define SMRT_ASCII_N            0x4E    /**< 'N' */
#define SMRT_ASCII_O            0x4F    /**< 'O' */
#define SMRT_ASCII_P            0x50    /**< 'P' */
#define SMRT_ASCII_Q            0x51    /**< 'Q' */
#define SMRT_ASCII_R            0x52    /**< 'R' */
#define SMRT_ASCII_S            0x53    /**< 'S' */
#define SMRT_ASCII_T            0x54    /**< 'T' */
#define SMRT_ASCII_U            0x55    /**< 'U' */
#define SMRT_ASCII_V            0x56    /**< 'V' */
#define SMRT_ASCII_W            0x57    /**< 'W' */
#define SMRT_ASCII_X            0x58    /**< 'X' */
#define SMRT_ASCII_Y            0x59    /**< 'Y' */
#define SMRT_ASCII_Z            0x5A    /**< 'Z' */

//-----------------------------------------------------------------------------
// ASCII lowercase letter definitions (a-z)
//-----------------------------------------------------------------------------
#define SMRT_ASCII_FIRST_LOWER  0x61    /**< 'a' - First lowercase letter */
#define SMRT_ASCII_LAST_LOWER   0x7A    /**< 'z' - Last lowercase letter */

//-----------------------------------------------------------------------------
// ASCII numeric digit definitions (0-9)
//-----------------------------------------------------------------------------
#define SMRT_ASCII_NUM_0        0x30    /**< '0' */
#define SMRT_ASCII_NUM_1        0x31    /**< '1' */
#define SMRT_ASCII_NUM_2        0x32    /**< '2' */
#define SMRT_ASCII_NUM_3        0x33    /**< '3' */
#define SMRT_ASCII_NUM_4        0x34    /**< '4' */
#define SMRT_ASCII_NUM_5        0x35    /**< '5' */
#define SMRT_ASCII_NUM_6        0x36    /**< '6' */
#define SMRT_ASCII_NUM_7        0x37    /**< '7' */
#define SMRT_ASCII_NUM_8        0x38    /**< '8' */
#define SMRT_ASCII_NUM_9        0x39    /**< '9' */

//-----------------------------------------------------------------------------
// Function prototypes
//-----------------------------------------------------------------------------

#ifndef UNIT_TEST
/**
 * @brief  Initializes the serial port with configured baudrate
 * @param  none
 * @return void
 */
void smrt_serial_init(void);

/**
 * @brief  Reads a string from the serial port buffer
 * @param  none
 * @return String containing the received data, empty string if no data available
 */
String smrt_serial_read(void);
#endif // UNIT_TEST

#endif // SMRT_MC_SERIAL_H
