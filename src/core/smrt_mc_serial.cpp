/**
 * @file    smrt_mc_serial.cpp
 * @brief   Configuracion y lectura del puerto serial (UART)
 * @project HOMENODE
 * @version 1.0.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes the serial port with configured baudrate.
 *         Sets a read timeout to avoid blocking reads.
 * @param  none
 * @return void
 */
void smrt_serial_init(void) {
    Serial.begin(SMRT_SERIAL_BAUDRATE);
    Serial.setTimeout(SMRT_SERIAL_TIMEOUT);
}

/**
 * @brief  Reads a string from the serial port buffer.
 *         Checks if data is available and reads the complete string.
 * @param  none
 * @return String containing the received data, empty string if no data available
 */
String smrt_serial_read(void) {
    if (Serial.available() > 0) {
        return Serial.readString();
    }
    return "";
}

#endif // UNIT_TEST
