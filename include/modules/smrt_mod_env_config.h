/**
 * @file    smrt_mod_env_config.h
 * @brief   Configuration defines for the Environmental module (DHT22)
 * @project HOMENODE
 * @version 0.7.0
 *
 * Pin assignment, sensor type and timing constants for smrt_mod_env.
 * Refer to docs/requirements.md section 4.4 for the pin map.
 */

#ifndef SMRT_MOD_ENV_CONFIG_H
#define SMRT_MOD_ENV_CONFIG_H

//-----------------------------------------------------------------------------
// Sensor hardware
//-----------------------------------------------------------------------------
#define SMRT_ENV_DHT_PIN            4       /**< GPIO4 — DHT22 data line */
#define SMRT_ENV_DHT_TYPE           22      /**< Sensor type: 22 = DHT22/AM2302 */

//-----------------------------------------------------------------------------
// Timing
//-----------------------------------------------------------------------------
#define SMRT_ENV_READ_INTERVAL_MS   5000    /**< Default sensor read interval (ms) */
#define SMRT_ENV_READ_MIN_MS        2000    /**< Minimum read interval (DHT22 limit) */
#define SMRT_ENV_READ_MAX_MS        60000   /**< Maximum read interval (1 minute) */
#define SMRT_ENV_READ_RETRIES       3       /**< Retry attempts on sensor read failure */
#define SMRT_ENV_RETRY_DELAY_MS     50      /**< Delay between read retries (ms) */

//-----------------------------------------------------------------------------
// Calibration offsets (additive correction)
//-----------------------------------------------------------------------------
#define SMRT_ENV_TEMP_OFFSET        0.0f    /**< Temperature offset (C) */
#define SMRT_ENV_HUM_OFFSET         0.0f    /**< Humidity offset (%) */

//-----------------------------------------------------------------------------
// NVS keys (namespace: "env")
//-----------------------------------------------------------------------------
#define SMRT_ENV_NVS_NAMESPACE      "env"           /**< NVS namespace for ENV module */
#define SMRT_ENV_NVS_KEY_INTERVAL   "read_intv"     /**< NVS key: read interval */

//-----------------------------------------------------------------------------
// Alert thresholds
//-----------------------------------------------------------------------------
#define SMRT_ENV_TEMP_ALERT_HI      40.0f   /**< Default high temp threshold (C) */
#define SMRT_ENV_TEMP_ALERT_LO      5.0f    /**< Default low temp threshold (C) */
#define SMRT_ENV_HUM_ALERT_HI       85.0f   /**< Default high humidity threshold (%) */
#define SMRT_ENV_HUM_ALERT_LO       20.0f   /**< Default low humidity threshold (%) */

//-----------------------------------------------------------------------------
// Alert bitmask
//-----------------------------------------------------------------------------
#define SMRT_ENV_ALERT_TEMP_HI      0x01
#define SMRT_ENV_ALERT_TEMP_LO      0x02
#define SMRT_ENV_ALERT_HUM_HI       0x04
#define SMRT_ENV_ALERT_HUM_LO       0x08

#endif // SMRT_MOD_ENV_CONFIG_H
