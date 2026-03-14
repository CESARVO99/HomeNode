/**
 * @file    smrt_mod_nrg.h
 * @brief   Energy monitoring module — multi-channel voltage, current, power
 * @project HOMENODE
 * @version 0.4.0
 *
 * Multi-channel energy monitor with moving average smoothing.
 * Up to 4 channels of voltage/current measurement.
 *
 * WebSocket sub-commands (prefix "nrg_" stripped):
 *   "status"         — Request full status (all channels)
 *   "read"           — Read single channel (field: "channel")
 *   "set_interval"   — Set read interval (field: "value" in ms)
 *   "set_channels"   — Set active channel count (field: "value")
 *   "set_alert"      — Set power alert threshold (field: "value" in W)
 *   "reset_energy"   — Reset kWh for channel (field: "channel")
 *
 * Telemetry (under "modules.nrg"):
 *   "channels", "interval", "alert", "ch": [{v,i,w,va,pf,kwh}, ...]
 */

#ifndef SMRT_MOD_NRG_H
#define SMRT_MOD_NRG_H

#include "core/smrt_core_module.h"
#include "modules/smrt_mod_nrg_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE DESCRIPTOR                                         *
 *************************************************************/

extern const smrt_module_t smrt_mod_nrg;

/*************************************************************
 * TESTABLE UTILITY FUNCTIONS                                *
 *************************************************************/

/**
 * @brief  Validates read interval within bounds.
 * @param  ms  Proposed interval
 * @return 1 if valid, 0 otherwise
 */
int smrt_nrg_validate_interval(unsigned long ms);

/**
 * @brief  Validates power alert threshold within bounds.
 * @param  watts  Proposed threshold
 * @return 1 if valid, 0 otherwise
 */
int smrt_nrg_validate_alert(float watts);

/**
 * @brief  Validates channel count within 1..MAX_CHANNELS.
 * @param  count  Proposed channel count
 * @return 1 if valid, 0 otherwise
 */
int smrt_nrg_validate_channels(int count);

/**
 * @brief  Calculates real power (W = V * I).
 * @param  voltage  RMS voltage
 * @param  current  RMS current
 * @return Power in watts
 */
float smrt_nrg_calc_power(float voltage, float current);

/**
 * @brief  Calculates apparent power (VA = V * I).
 * @param  voltage  RMS voltage
 * @param  current  RMS current
 * @return Apparent power in VA
 */
float smrt_nrg_calc_apparent_power(float voltage, float current);

/**
 * @brief  Calculates power factor (PF = W / VA).
 * @param  real_power     Real power in watts
 * @param  apparent_power Apparent power in VA
 * @return Power factor (0.0-1.0), 0.0 if apparent_power is 0
 */
float smrt_nrg_calc_power_factor(float real_power, float apparent_power);

/**
 * @brief  Calculates energy increment.
 * @param  watts      Power in watts
 * @param  elapsed_ms Elapsed time in milliseconds
 * @return Energy in watt-hours
 */
float smrt_nrg_calc_energy(float watts, unsigned long elapsed_ms);

/**
 * @brief  Computes simple moving average.
 * @param  buffer  Circular buffer of values
 * @param  size    Buffer size
 * @param  count   Number of valid entries
 * @return Average value
 */
float smrt_nrg_moving_avg(const float *buffer, int size, int count);

/**
 * @brief  Returns active channel count.
 * @return Channel count (1..4)
 */
int smrt_nrg_get_channels(void);

/**
 * @brief  Sets active channel count.
 * @param  count  New count
 * @return void
 */
void smrt_nrg_set_channels(int count);

/**
 * @brief  Returns read interval.
 * @return Interval in milliseconds
 */
unsigned long smrt_nrg_get_interval(void);

/**
 * @brief  Sets read interval.
 * @param  ms  Interval in milliseconds
 * @return void
 */
void smrt_nrg_set_interval(unsigned long ms);

/**
 * @brief  Returns alert threshold.
 * @return Threshold in watts
 */
float smrt_nrg_get_alert(void);

/**
 * @brief  Sets alert threshold.
 * @param  watts  Threshold in watts
 * @return void
 */
void smrt_nrg_set_alert(float watts);

/**
 * @brief  Returns accumulated energy for a channel.
 * @param  ch  Channel index (0-based)
 * @return Energy in watt-hours, 0.0 if out of range
 */
float smrt_nrg_get_energy(int ch);

/**
 * @brief  Sets accumulated energy for a channel.
 * @param  ch  Channel index (0-based)
 * @param  wh  Energy in watt-hours
 * @return void
 */
void smrt_nrg_set_energy(int ch, float wh);

#ifdef __cplusplus
}
#endif

#endif // SMRT_MOD_NRG_H
