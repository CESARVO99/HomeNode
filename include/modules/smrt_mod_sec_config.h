/**
 * @file    smrt_mod_sec_config.h
 * @brief   Configuration defines for the Security module
 * @project HOMENODE
 * @version 0.4.0
 *
 * Pin assignment, alarm states, timing and event log configuration.
 * WARNING: GPIO25 (buzzer) shared with RELAY module relay 3.
 */

#ifndef SMRT_MOD_SEC_CONFIG_H
#define SMRT_MOD_SEC_CONFIG_H

//-----------------------------------------------------------------------------
// Sensor pin assignments
//-----------------------------------------------------------------------------
#define SMRT_SEC_PIR_PIN            12      /**< GPIO12 — PIR motion sensor */
#define SMRT_SEC_REED_PIN           13      /**< GPIO13 — Reed switch (door/window) */
#define SMRT_SEC_VIBR_PIN           14      /**< GPIO14 — Vibration sensor */
#define SMRT_SEC_BUZZER_PIN         25      /**< GPIO25 — Buzzer output */

//-----------------------------------------------------------------------------
// Alarm states
//-----------------------------------------------------------------------------
#define SMRT_SEC_STATE_DISARMED     0       /**< System disarmed */
#define SMRT_SEC_STATE_ARMED        1       /**< System armed */
#define SMRT_SEC_STATE_TRIGGERED    2       /**< Alarm triggered */
#define SMRT_SEC_STATE_ENTRY_DELAY  3       /**< Entry delay countdown */
#define SMRT_SEC_STATE_EXIT_DELAY   4       /**< Exit delay countdown */

//-----------------------------------------------------------------------------
// State machine events
//-----------------------------------------------------------------------------
#define SMRT_SEC_EVT_ARM            1       /**< Arm command */
#define SMRT_SEC_EVT_DISARM         2       /**< Disarm command */
#define SMRT_SEC_EVT_MOTION         3       /**< PIR motion detected */
#define SMRT_SEC_EVT_DOOR_OPEN      4       /**< Reed switch opened */
#define SMRT_SEC_EVT_VIBRATION      5       /**< Vibration detected */
#define SMRT_SEC_EVT_TIMEOUT        6       /**< Delay timer expired */

//-----------------------------------------------------------------------------
// Timing
//-----------------------------------------------------------------------------
#define SMRT_SEC_ENTRY_DELAY_MS     30000   /**< Default entry delay (30s) */
#define SMRT_SEC_EXIT_DELAY_MS      30000   /**< Default exit delay (30s) */
#define SMRT_SEC_DELAY_MIN_MS       5000    /**< Minimum delay (5s) */
#define SMRT_SEC_DELAY_MAX_MS       120000  /**< Maximum delay (2 min) */
#define SMRT_SEC_DEBOUNCE_MS        200     /**< Sensor debounce time (ms) */

//-----------------------------------------------------------------------------
// Event log
//-----------------------------------------------------------------------------
#define SMRT_SEC_EVENT_LOG_SIZE     16      /**< Max events in circular buffer */
#define SMRT_SEC_EVENT_MSG_LEN      32      /**< Max chars per event message */

//-----------------------------------------------------------------------------
// NVS keys (namespace: "sec")
//-----------------------------------------------------------------------------
#define SMRT_SEC_NVS_NAMESPACE      "sec"           /**< NVS namespace */
#define SMRT_SEC_NVS_KEY_ARMED      "armed"         /**< Armed state persistence */
#define SMRT_SEC_NVS_KEY_ENTRY_DLY  "entry_dly"     /**< Entry delay ms */
#define SMRT_SEC_NVS_KEY_EXIT_DLY   "exit_dly"      /**< Exit delay ms */

#endif // SMRT_MOD_SEC_CONFIG_H
