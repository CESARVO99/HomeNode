/**
 * @file    smrt_mod_plg_config.h
 * @brief   Configuration defines for the Smart Plug module
 * @project HOMENODE
 * @version 0.4.0
 *
 * Pin assignment, ADC parameters, calibration constants, and NVS keys.
 * Uses ACS712-30A for current sensing and ZMPT101B for voltage sensing.
 * WARNING: GPIO2 shared with ACCESS module lock relay.
 * WARNING: GPIO34/35 shared with ENERGY module channels.
 */

#ifndef SMRT_MOD_PLG_CONFIG_H
#define SMRT_MOD_PLG_CONFIG_H

//-----------------------------------------------------------------------------
// Pin assignments
//-----------------------------------------------------------------------------
#define SMRT_PLG_RELAY_PIN          2       /**< GPIO2  — Plug relay output */
#define SMRT_PLG_CURRENT_PIN        34      /**< GPIO34 — ACS712 current ADC */
#define SMRT_PLG_VOLTAGE_PIN        35      /**< GPIO35 — ZMPT101B voltage ADC */

//-----------------------------------------------------------------------------
// ADC configuration
//-----------------------------------------------------------------------------
#define SMRT_PLG_ADC_SAMPLES        100     /**< Samples per RMS calculation */
#define SMRT_PLG_ADC_RESOLUTION     4096    /**< 12-bit ADC (0-4095) */
#define SMRT_PLG_ADC_VREF           3.3f    /**< ADC reference voltage */
#define SMRT_PLG_ADC_MIDPOINT       2048    /**< Zero-cross offset (half range) */

//-----------------------------------------------------------------------------
// Calibration — ACS712-30A
//-----------------------------------------------------------------------------
#define SMRT_PLG_ACS712_SENS        0.066f  /**< ACS712-30A sensitivity (V/A) */

//-----------------------------------------------------------------------------
// Calibration — ZMPT101B
//-----------------------------------------------------------------------------
#define SMRT_PLG_ZMPT_RATIO         234.26f /**< ZMPT101B voltage ratio */

//-----------------------------------------------------------------------------
// Overload protection
//-----------------------------------------------------------------------------
#define SMRT_PLG_OVERLOAD_MIN_A     1.0f    /**< Minimum overload threshold (A) */
#define SMRT_PLG_OVERLOAD_MAX_A     30.0f   /**< Maximum overload threshold (A) */
#define SMRT_PLG_OVERLOAD_DEFAULT_A 15.0f   /**< Default overload threshold (A) */

//-----------------------------------------------------------------------------
// Read interval
//-----------------------------------------------------------------------------
#define SMRT_PLG_INTERVAL_MIN_MS    1000    /**< Minimum read interval (1s) */
#define SMRT_PLG_INTERVAL_MAX_MS    60000   /**< Maximum read interval (60s) */
#define SMRT_PLG_INTERVAL_DEFAULT_MS 2000   /**< Default read interval (2s) */

//-----------------------------------------------------------------------------
// NVS keys (namespace: "plg")
//-----------------------------------------------------------------------------
#define SMRT_PLG_NVS_NAMESPACE      "plg"           /**< NVS namespace */
#define SMRT_PLG_NVS_KEY_STATE      "state"         /**< Relay state persistence */
#define SMRT_PLG_NVS_KEY_INTERVAL   "intv"          /**< Read interval */
#define SMRT_PLG_NVS_KEY_OVERLOAD   "overload"      /**< Overload threshold */
#define SMRT_PLG_NVS_KEY_KWH        "kwh"           /**< Accumulated energy (Wh) */

#endif // SMRT_MOD_PLG_CONFIG_H
