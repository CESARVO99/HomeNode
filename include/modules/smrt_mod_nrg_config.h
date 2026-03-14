/**
 * @file    smrt_mod_nrg_config.h
 * @brief   Configuration defines for the Energy monitoring module
 * @project HOMENODE
 * @version 0.4.0
 *
 * Multi-channel energy monitoring with ADC.
 * Channel 0: GPIO34 (V) + GPIO35 (I)
 * Channel 1: GPIO36 (V) + GPIO39 (I)
 * Channel 2/3: Share V from ch0, GPIO32 (I ch2), GPIO33 (I ch3)
 * WARNING: GPIO34-39 shared with PLUG module.
 */

#ifndef SMRT_MOD_NRG_CONFIG_H
#define SMRT_MOD_NRG_CONFIG_H

//-----------------------------------------------------------------------------
// Pin assignments per channel
//-----------------------------------------------------------------------------
#define SMRT_NRG_CH0_V_PIN          34      /**< GPIO34 — Channel 0 voltage */
#define SMRT_NRG_CH0_I_PIN          35      /**< GPIO35 — Channel 0 current */
#define SMRT_NRG_CH1_V_PIN          36      /**< GPIO36 — Channel 1 voltage */
#define SMRT_NRG_CH1_I_PIN          39      /**< GPIO39 — Channel 1 current */
#define SMRT_NRG_CH2_I_PIN          32      /**< GPIO32 — Channel 2 current (V from ch0) */
#define SMRT_NRG_CH3_I_PIN          33      /**< GPIO33 — Channel 3 current (V from ch0) */

//-----------------------------------------------------------------------------
// Channel limits
//-----------------------------------------------------------------------------
#define SMRT_NRG_MAX_CHANNELS       4       /**< Maximum supported channels */
#define SMRT_NRG_DEFAULT_CHANNELS   1       /**< Default active channel count */

//-----------------------------------------------------------------------------
// ADC configuration
//-----------------------------------------------------------------------------
#define SMRT_NRG_ADC_SAMPLES        200     /**< Samples per RMS calculation */
#define SMRT_NRG_ADC_RESOLUTION     4096    /**< 12-bit ADC */
#define SMRT_NRG_ADC_VREF           3.3f    /**< ADC reference voltage */
#define SMRT_NRG_ADC_MIDPOINT       2048    /**< DC offset */

//-----------------------------------------------------------------------------
// Calibration
//-----------------------------------------------------------------------------
#define SMRT_NRG_ACS712_SENS        0.066f  /**< ACS712-30A sensitivity (V/A) */
#define SMRT_NRG_ZMPT_RATIO         234.26f /**< ZMPT101B voltage ratio */

//-----------------------------------------------------------------------------
// Moving average
//-----------------------------------------------------------------------------
#define SMRT_NRG_AVG_WINDOW         5       /**< Moving average window size */

//-----------------------------------------------------------------------------
// Power alert
//-----------------------------------------------------------------------------
#define SMRT_NRG_ALERT_MIN_W        100.0f  /**< Minimum alert threshold (W) */
#define SMRT_NRG_ALERT_MAX_W        10000.0f /**< Maximum alert threshold (W) */
#define SMRT_NRG_ALERT_DEFAULT_W    3000.0f /**< Default alert threshold (W) */

//-----------------------------------------------------------------------------
// Read interval
//-----------------------------------------------------------------------------
#define SMRT_NRG_INTERVAL_MIN_MS    1000    /**< Minimum read interval (1s) */
#define SMRT_NRG_INTERVAL_MAX_MS    60000   /**< Maximum read interval (60s) */
#define SMRT_NRG_INTERVAL_DEFAULT_MS 5000   /**< Default read interval (5s) */

//-----------------------------------------------------------------------------
// NVS keys (namespace: "nrg")
//-----------------------------------------------------------------------------
#define SMRT_NRG_NVS_NAMESPACE      "nrg"           /**< NVS namespace */
#define SMRT_NRG_NVS_KEY_CH_COUNT   "ch_count"      /**< Active channel count */
#define SMRT_NRG_NVS_KEY_INTERVAL   "intv"          /**< Read interval */
#define SMRT_NRG_NVS_KEY_ALERT      "alert_w"       /**< Alert threshold */
#define SMRT_NRG_NVS_KEY_KWH_PFX    "kwh_"          /**< Energy prefix: kwh_0, kwh_1 */

#endif // SMRT_MOD_NRG_CONFIG_H
