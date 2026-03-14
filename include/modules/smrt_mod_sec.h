/**
 * @file    smrt_mod_sec.h
 * @brief   Security module — PIR, reed switch, alarm state machine
 * @project HOMENODE
 * @version 0.4.0
 *
 * Intrusion detection with PIR motion, door/window reed switch, vibration.
 * State machine: DISARMED → EXIT_DELAY → ARMED → ENTRY_DELAY → TRIGGERED.
 * Circular event log with timestamps.
 *
 * WebSocket sub-commands (prefix "sec_" stripped):
 *   "arm"              — Arm the system (enter exit delay)
 *   "disarm"           — Disarm the system
 *   "status"           — Request full state
 *   "set_entry_delay"  — Set entry delay ms
 *   "set_exit_delay"   — Set exit delay ms
 *   "get_events"       — Get event log
 *   "clear_events"     — Clear event log
 *
 * Telemetry (under "modules.sec"):
 *   "state", "pir", "reed", "vibration", "entry_delay", "exit_delay", "events"
 */

#ifndef SMRT_MOD_SEC_H
#define SMRT_MOD_SEC_H

#include "core/smrt_core_module.h"
#include "modules/smrt_mod_sec_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE DESCRIPTOR                                         *
 *************************************************************/

extern const smrt_module_t smrt_mod_sec;

/*************************************************************
 * TESTABLE UTILITY FUNCTIONS                                *
 *************************************************************/

/**
 * @brief  Validates a delay value is within bounds.
 * @param  ms  Proposed delay in milliseconds
 * @return 1 if valid, 0 otherwise
 */
int smrt_sec_validate_delay(unsigned long ms);

/**
 * @brief  Returns the current alarm state.
 * @return One of SMRT_SEC_STATE_* constants
 */
int smrt_sec_get_alarm_state(void);

/**
 * @brief  Sets the alarm state (for testing).
 * @param  state  New state value
 * @return void
 */
void smrt_sec_set_alarm_state(int state);

/**
 * @brief  Pure state machine transition logic.
 * @param  current  Current alarm state
 * @param  event    Event (SMRT_SEC_EVT_*)
 * @return New alarm state after transition
 */
int smrt_sec_transition(int current, int event);

/**
 * @brief  Returns the number of logged events.
 * @return Event count (0..SMRT_SEC_EVENT_LOG_SIZE)
 */
int smrt_sec_get_event_count(void);

/**
 * @brief  Adds an event to the circular log.
 * @param  msg        Event message string
 * @param  timestamp  Event timestamp (millis)
 * @return void
 */
void smrt_sec_add_event(const char *msg, unsigned long timestamp);

/**
 * @brief  Retrieves an event from the log.
 * @param  index      Event index (0 = oldest)
 * @param  timestamp  Output: event timestamp (NULL to skip)
 * @return Event message string, or NULL if index out of range
 */
const char *smrt_sec_get_event(int index, unsigned long *timestamp);

/**
 * @brief  Clears all events from the log.
 * @return void
 */
void smrt_sec_clear_events(void);

/**
 * @brief  Returns the entry delay.
 * @return Entry delay in milliseconds
 */
unsigned long smrt_sec_get_entry_delay(void);

/**
 * @brief  Sets the entry delay.
 * @param  ms  New entry delay
 * @return void
 */
void smrt_sec_set_entry_delay(unsigned long ms);

/**
 * @brief  Returns the exit delay.
 * @return Exit delay in milliseconds
 */
unsigned long smrt_sec_get_exit_delay(void);

/**
 * @brief  Sets the exit delay.
 * @param  ms  New exit delay
 * @return void
 */
void smrt_sec_set_exit_delay(unsigned long ms);

#ifdef __cplusplus
}
#endif

#endif // SMRT_MOD_SEC_H
