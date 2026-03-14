/**
 * @file    smrt_mod_rly.h
 * @brief   Relay control module — 1 to 4 relay outputs with pulse mode
 * @project HOMENODE
 * @version 0.4.0
 *
 * Controls 1-4 relay outputs with toggle, set, and timed pulse modes.
 * Each relay has a configurable name and its state persists via NVS.
 *
 * WebSocket sub-commands (prefix "rly_" stripped by dispatch):
 *   "toggle"       — Toggle relay at index
 *   "set"          — Set specific relay state (field: "index", "state")
 *   "set_all"      — Set all relay states (field: "states" array)
 *   "pulse"        — Timed pulse relay at index
 *   "set_pulse"    — Change pulse duration (field: "value" in ms)
 *   "set_count"    — Change active relay count (field: "value")
 *   "set_name"     — Change relay name (fields: "index", "name")
 *   "status"       — Request full relay status
 *
 * Telemetry fields (under "modules.rly"):
 *   "count"        — Active relay count
 *   "states"       — Array of 0/1 per relay
 *   "names"        — Array of relay name strings
 *   "pulse_ms"     — Current pulse duration
 */

#ifndef SMRT_MOD_RLY_H
#define SMRT_MOD_RLY_H

#include "core/smrt_core_module.h"
#include "modules/smrt_mod_rly_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE DESCRIPTOR                                         *
 *************************************************************/

extern const smrt_module_t smrt_mod_rly;

/*************************************************************
 * TESTABLE UTILITY FUNCTIONS                                *
 *************************************************************/

/**
 * @brief  Validates pulse duration is within allowed bounds.
 * @param  ms  Proposed pulse duration in milliseconds
 * @return 1 if valid, 0 otherwise
 */
int smrt_rly_validate_pulse(unsigned long ms);

/**
 * @brief  Validates relay count is within 1..SMRT_RLY_MAX_RELAYS.
 * @param  count  Proposed relay count
 * @return 1 if valid, 0 otherwise
 */
int smrt_rly_validate_count(int count);

/**
 * @brief  Returns the current state of a relay by index.
 * @param  index  Relay index (0-based)
 * @return 1 if ON, 0 if OFF, -1 if index out of range
 */
int smrt_rly_get_state(int index);

/**
 * @brief  Sets the state of a relay by index (in RAM only).
 * @param  index  Relay index (0-based)
 * @param  state  0=OFF, 1=ON
 * @return void
 */
void smrt_rly_set_state(int index, int state);

/**
 * @brief  Returns the active relay count.
 * @return Number of active relays (1..4)
 */
int smrt_rly_get_count(void);

/**
 * @brief  Sets the active relay count.
 * @param  count  New relay count (must pass validation)
 * @return void
 */
void smrt_rly_set_count(int count);

/**
 * @brief  Returns the current pulse duration.
 * @return Pulse duration in milliseconds
 */
unsigned long smrt_rly_get_pulse(void);

/**
 * @brief  Sets the pulse duration.
 * @param  ms  New pulse duration (must pass validation)
 * @return void
 */
void smrt_rly_set_pulse(unsigned long ms);

#ifdef __cplusplus
}
#endif

#endif // SMRT_MOD_RLY_H
