/**
 * @file    smrt_mc_gpio.cpp
 * @brief   Control generico de pines GPIO
 * @project HOMENODE
 * @version 1.0.0
 *
 * Platform-level GPIO init only sets up the built-in LED.
 * Module-specific pins are initialized by each module's init() callback.
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
 * @brief  Initializes GPIO pins for the platform.
 *         Only configures the built-in LED as OUTPUT (LOW).
 *         Module-specific GPIOs are initialized by modules.
 * @param  none
 * @return void
 */
void smrt_gpio_init(void) {
    pinMode(SMRT_LED_BUILTIN, OUTPUT);
    digitalWrite(SMRT_LED_BUILTIN, LOW);
}

/**
 * @brief  Sets a digital output pin to HIGH state
 * @param  pin  GPIO pin number to set HIGH
 * @return void
 */
void smrt_gpio_set_state(int pin) {
    digitalWrite(pin, HIGH);
}

/**
 * @brief  Sets a digital output pin to LOW state
 * @param  pin  GPIO pin number to set LOW
 * @return void
 */
void smrt_gpio_clear_state(int pin) {
    digitalWrite(pin, LOW);
}

/**
 * @brief  Toggles the current state of a digital output pin.
 *         Reads the current pin state and inverts it.
 * @param  pin  GPIO pin number to toggle
 * @return void
 */
void smrt_gpio_toggle_state(int pin) {
    digitalWrite(pin, !digitalRead(pin));
}

#endif // UNIT_TEST
