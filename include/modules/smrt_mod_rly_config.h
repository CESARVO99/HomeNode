/**
 * @file    smrt_mod_rly_config.h
 * @brief   Configuration defines for the Relay control module
 * @project HOMENODE
 * @version 0.4.0
 *
 * Pin assignment, relay count limits and timing constants for smrt_mod_rly.
 * WARNING: GPIO25 is shared with SEC module buzzer — do not enable both.
 */

#ifndef SMRT_MOD_RLY_CONFIG_H
#define SMRT_MOD_RLY_CONFIG_H

//-----------------------------------------------------------------------------
// Relay pin assignments
//-----------------------------------------------------------------------------
#define SMRT_RLY_PIN_1              16      /**< GPIO16 — Relay 1 */
#define SMRT_RLY_PIN_2              17      /**< GPIO17 — Relay 2 */
#define SMRT_RLY_PIN_3              25      /**< GPIO25 — Relay 3 (shared w/ SEC buzzer) */
#define SMRT_RLY_PIN_4              26      /**< GPIO26 — Relay 4 */

//-----------------------------------------------------------------------------
// Relay limits
//-----------------------------------------------------------------------------
#define SMRT_RLY_MAX_RELAYS         4       /**< Maximum supported relay count */
#define SMRT_RLY_DEFAULT_COUNT      1       /**< Default active relay count */

//-----------------------------------------------------------------------------
// Pulse mode timing
//-----------------------------------------------------------------------------
#define SMRT_RLY_PULSE_MIN_MS       100     /**< Minimum pulse duration (ms) */
#define SMRT_RLY_PULSE_MAX_MS       30000   /**< Maximum pulse duration (30s) */
#define SMRT_RLY_PULSE_DEFAULT_MS   3000    /**< Default pulse duration (ms) */

//-----------------------------------------------------------------------------
// Relay names
//-----------------------------------------------------------------------------
#define SMRT_RLY_NAME_MAX_LEN       16      /**< Max characters per relay name */

//-----------------------------------------------------------------------------
// NVS keys (namespace: "rly")
//-----------------------------------------------------------------------------
#define SMRT_RLY_NVS_NAMESPACE      "rly"           /**< NVS namespace */
#define SMRT_RLY_NVS_KEY_COUNT      "count"         /**< Active relay count */
#define SMRT_RLY_NVS_KEY_STATES     "states"        /**< Bitmask of relay states */
#define SMRT_RLY_NVS_KEY_PULSE      "pulse_ms"      /**< Pulse duration */
#define SMRT_RLY_NVS_KEY_NAME_PFX   "name_"         /**< Name prefix: name_0, name_1 */

#endif // SMRT_MOD_RLY_CONFIG_H
