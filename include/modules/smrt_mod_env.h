/**
 * @file    smrt_mod_env.h
 * @brief   Environmental monitoring module — DHT22 temperature & humidity
 * @project HOMENODE
 * @version 0.3.0
 *
 * This is the first HomeNode application module. It reads temperature and
 * humidity from a DHT22 sensor and exposes the data via WebSocket telemetry
 * and on-demand commands.
 *
 * WebSocket sub-commands (prefix "env_" stripped by dispatch):
 *   "read"          — Request immediate sensor reading response
 *   "set_interval"  — Change read interval (field: "value" in ms)
 *
 * Telemetry fields (under "modules.env"):
 *   "temperature"   — Last temperature reading (°C, float)
 *   "humidity"      — Last humidity reading (%, float)
 *   "ok"            — true if last read succeeded, false on error
 */

#ifndef SMRT_MOD_ENV_H
#define SMRT_MOD_ENV_H

#include "core/smrt_core_module.h"
#include "modules/smrt_mod_env_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE DESCRIPTOR                                         *
 *************************************************************/

/**
 * @brief  Module descriptor for registration with smrt_module_register().
 *         Defined in smrt_mod_env.cpp.
 */
extern const smrt_module_t smrt_mod_env;

/*************************************************************
 * TESTABLE UTILITY FUNCTIONS                                *
 *************************************************************/

/**
 * @brief  Validates that a read interval is within allowed bounds.
 * @param  ms  Proposed interval in milliseconds
 * @return 1 if valid [SMRT_ENV_READ_MIN_MS..SMRT_ENV_READ_MAX_MS], 0 otherwise
 */
int smrt_env_validate_interval(unsigned long ms);

/**
 * @brief  Returns the current sensor read interval in milliseconds.
 * @return Current interval value
 */
unsigned long smrt_env_get_interval(void);

/**
 * @brief  Sets the sensor read interval (must pass validation first).
 * @param  ms  New interval in milliseconds
 * @return void
 */
void smrt_env_set_interval(unsigned long ms);

/**
 * @brief  Returns the last temperature reading.
 * @return Temperature in degrees Celsius (with offset applied)
 */
float smrt_env_get_temperature(void);

/**
 * @brief  Returns the last humidity reading.
 * @return Relative humidity in percent (with offset applied)
 */
float smrt_env_get_humidity(void);

/**
 * @brief  Returns whether the last sensor read was successful.
 * @return 1 if last read OK, 0 if sensor error (NaN)
 */
int smrt_env_get_status(void);

/**
 * @brief  Checks sensor values against alert thresholds.
 * @param  temp    Temperature reading
 * @param  hum     Humidity reading
 * @param  t_hi    High temp threshold
 * @param  t_lo    Low temp threshold
 * @param  h_hi    High humidity threshold
 * @param  h_lo    Low humidity threshold
 * @return Bitmask of triggered alerts (0 = no alert)
 */
int smrt_env_check_alert(float temp, float hum, float t_hi, float t_lo,
                          float h_hi, float h_lo);

/**
 * @brief  Validates an alert threshold value.
 * @param  value  Threshold value
 * @param  min    Minimum allowed
 * @param  max    Maximum allowed
 * @return 1 if valid, 0 otherwise
 */
int smrt_env_validate_threshold(float value, float min, float max);

#ifndef UNIT_TEST
/**
 * @brief  Returns pointer to the temperature ring buffer
 */
smrt_ringbuf_t *smrt_env_get_ringbuf_temp(void);

/**
 * @brief  Returns pointer to the humidity ring buffer
 */
smrt_ringbuf_t *smrt_env_get_ringbuf_hum(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // SMRT_MOD_ENV_H
