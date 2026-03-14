/**
 * @file    smrt_mod_plg.h
 * @brief   Smart Plug module — relay + current/voltage/power monitoring
 * @project HOMENODE
 * @version 0.4.0
 *
 * Controls a relay output with real-time current, voltage, power measurement.
 * Uses ACS712-30A (current) and ZMPT101B (voltage) analog sensors.
 * Includes overload protection and cumulative energy tracking.
 *
 * WebSocket sub-commands (prefix "plg_" stripped):
 *   "toggle"         — Toggle relay state
 *   "set"            — Set relay state (field: "state")
 *   "status"         — Request full status
 *   "set_interval"   — Set read interval (field: "value" in ms)
 *   "set_overload"   — Set overload threshold (field: "value" in A)
 *   "reset_energy"   — Reset kWh accumulator
 *
 * Telemetry (under "modules.plg"):
 *   "state", "voltage", "current", "power", "energy", "overload", "interval"
 */

#ifndef SMRT_MOD_PLG_H
#define SMRT_MOD_PLG_H

#include "core/smrt_core_module.h"
#include "modules/smrt_mod_plg_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE DESCRIPTOR                                         *
 *************************************************************/

extern const smrt_module_t smrt_mod_plg;

/*************************************************************
 * TESTABLE UTILITY FUNCTIONS                                *
 *************************************************************/

/**
 * @brief  Validates read interval within bounds.
 * @param  ms  Proposed interval in milliseconds
 * @return 1 if valid, 0 otherwise
 */
int smrt_plg_validate_interval(unsigned long ms);

/**
 * @brief  Validates overload threshold within bounds.
 * @param  amps  Proposed threshold in amperes
 * @return 1 if valid, 0 otherwise
 */
int smrt_plg_validate_overload(float amps);

/**
 * @brief  Calculates real power from voltage and current.
 * @param  voltage  RMS voltage in volts
 * @param  current  RMS current in amperes
 * @return Power in watts
 */
float smrt_plg_calc_power(float voltage, float current);

/**
 * @brief  Calculates energy increment from power and elapsed time.
 * @param  watts      Power in watts
 * @param  elapsed_ms Elapsed time in milliseconds
 * @return Energy in watt-hours
 */
float smrt_plg_calc_energy(float watts, unsigned long elapsed_ms);

/**
 * @brief  Calculates RMS value from an array of ADC samples.
 * @param  samples  Array of raw ADC values
 * @param  count    Number of samples
 * @param  offset   DC offset (midpoint)
 * @param  scale    Scale factor (volts per ADC unit * sensor ratio)
 * @return RMS value in physical units
 */
float smrt_plg_calc_rms(const int *samples, int count, int offset, float scale);

/**
 * @brief  Returns current relay state.
 * @return 1=ON, 0=OFF
 */
int smrt_plg_get_state(void);

/**
 * @brief  Sets relay state (RAM only).
 * @param  state  0=OFF, 1=ON
 * @return void
 */
void smrt_plg_set_state(int state);

/**
 * @brief  Returns last measured RMS voltage.
 * @return Voltage in volts
 */
float smrt_plg_get_voltage(void);

/**
 * @brief  Returns last measured RMS current.
 * @return Current in amperes
 */
float smrt_plg_get_current(void);

/**
 * @brief  Returns last calculated power.
 * @return Power in watts
 */
float smrt_plg_get_power(void);

/**
 * @brief  Returns accumulated energy.
 * @return Energy in watt-hours
 */
float smrt_plg_get_energy(void);

/**
 * @brief  Sets accumulated energy (for reset or restore).
 * @param  wh  Energy in watt-hours
 * @return void
 */
void smrt_plg_set_energy(float wh);

/**
 * @brief  Returns current overload threshold.
 * @return Threshold in amperes
 */
float smrt_plg_get_overload(void);

/**
 * @brief  Sets overload threshold.
 * @param  amps  Threshold in amperes
 * @return void
 */
void smrt_plg_set_overload(float amps);

/**
 * @brief  Returns current read interval.
 * @return Interval in milliseconds
 */
unsigned long smrt_plg_get_interval(void);

/**
 * @brief  Sets read interval.
 * @param  ms  Interval in milliseconds
 * @return void
 */
void smrt_plg_set_interval(unsigned long ms);

#ifdef __cplusplus
}
#endif

#endif // SMRT_MOD_PLG_H
