/**
 * @file    smrt_mc_gpio.h
 * @brief   Prototipos de funciones de control GPIO generico
 * @project HOMENODE
 * @version 1.0.0
 *
 * Pin definitions are centralized in smrt_core_config.h.
 * This module provides generic GPIO control functions only.
 */

#ifndef SMRT_MC_GPIO_H
#define SMRT_MC_GPIO_H

//-----------------------------------------------------------------------------
// Function prototypes
//-----------------------------------------------------------------------------

#ifndef UNIT_TEST

/**
 * @brief  Initializes GPIO pins for the platform (built-in LED only).
 *         Module-specific pins are initialized by each module's init().
 * @param  none
 * @return void
 */
void smrt_gpio_init(void);

/**
 * @brief  Sets a digital output pin to HIGH state
 * @param  pin  GPIO pin number to set HIGH
 * @return void
 */
void smrt_gpio_set_state(int pin);

/**
 * @brief  Sets a digital output pin to LOW state
 * @param  pin  GPIO pin number to set LOW
 * @return void
 */
void smrt_gpio_clear_state(int pin);

/**
 * @brief  Toggles the current state of a digital output pin
 * @param  pin  GPIO pin number to toggle
 * @return void
 */
void smrt_gpio_toggle_state(int pin);

#endif // UNIT_TEST

#endif // SMRT_MC_GPIO_H
