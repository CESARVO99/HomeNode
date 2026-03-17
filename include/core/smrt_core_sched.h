/**
 * @file    smrt_core_sched.h
 * @brief   Time-based task scheduler with NTP synchronization
 * @project HOMENODE
 * @version 0.8.0
 *
 * Provides cron-like scheduling for module actions. Tasks are stored
 * in NVS and executed by dispatching through the existing WebSocket
 * command infrastructure (smrt_module_dispatch).
 *
 * Action format: "module_cmd" with optional ":arg1:arg2" suffix
 * Examples: "rly_set:0:1", "rly_toggle:1", "plg_set:1"
 */

#ifndef SMRT_CORE_SCHED_H
#define SMRT_CORE_SCHED_H

#include "smrt_core_sched_config.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Task data structure (testable — always compiled)
//-----------------------------------------------------------------------------
typedef struct {
    uint8_t enabled;                        /**< 0=disabled, 1=enabled */
    uint8_t hour;                           /**< 0-23 */
    uint8_t minute;                         /**< 0-59 */
    uint8_t days;                           /**< Day bitmask (bit0=Sun..bit6=Sat) */
    char    action[SMRT_SCHED_ACTION_LEN];  /**< Command to dispatch */
    char    name[SMRT_SCHED_NAME_LEN];      /**< Human-readable name */
} smrt_sched_task_t;

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a task structure
 * @param  task  Pointer to task
 * @return 1 if valid, 0 if invalid
 */
int smrt_sched_validate_task(const smrt_sched_task_t *task);

/**
 * @brief  Checks if a task should fire at the given time
 * @param  task    Pointer to task
 * @param  hour    Current hour (0-23)
 * @param  minute  Current minute (0-59)
 * @param  dow     Current day of week (0=Sun, 6=Sat)
 * @return 1 if should fire, 0 otherwise
 */
int smrt_sched_match_time(const smrt_sched_task_t *task, int hour, int minute, int dow);

/**
 * @brief  Parses an action string into module command components
 * @param  action     Action string (e.g., "rly_set:0:1")
 * @param  cmd_out    Output: full command (e.g., "rly_set") — max 32 chars
 * @param  args_out   Output: arguments string (e.g., "0:1") — max 32 chars
 * @return 1 on success, 0 on parse error
 */
int smrt_sched_parse_action(const char *action, char *cmd_out, char *args_out);

/**
 * @brief  Validates a task index
 * @param  index  Task index (0 to SMRT_SCHED_MAX_TASKS-1)
 * @return 1 if valid, 0 otherwise
 */
int smrt_sched_validate_index(int index);

/**
 * @brief  Converts a day bitmask to a human-readable string
 * @param  days    Day bitmask
 * @param  buf     Output buffer
 * @param  buf_len Buffer size (min 28)
 * @return void
 */
void smrt_sched_days_to_string(uint8_t days, char *buf, int buf_len);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/**
 * @brief  Initializes the scheduler — loads tasks from NVS
 * @return void
 */
void smrt_sched_init(void);

/**
 * @brief  Scheduler loop — checks time and fires matching tasks
 *         Call from main loop at any frequency; internally throttled.
 * @return void
 */
void smrt_sched_loop(void);

/**
 * @brief  Sets/updates a scheduled task
 * @param  index  Task index (0 to SMRT_SCHED_MAX_TASKS-1)
 * @param  task   Task data
 * @return 1 on success, 0 on error
 */
int smrt_sched_set_task(int index, const smrt_sched_task_t *task);

/**
 * @brief  Deletes a scheduled task
 * @param  index  Task index
 * @return 1 on success, 0 on error
 */
int smrt_sched_delete_task(int index);

/**
 * @brief  Returns a pointer to a task (read-only)
 * @param  index  Task index
 * @return Pointer to task, or NULL if invalid
 */
const smrt_sched_task_t *smrt_sched_get_task(int index);

/**
 * @brief  Returns the number of active (enabled) tasks
 * @return Count of enabled tasks
 */
int smrt_sched_active_count(void);

/**
 * @brief  Handles scheduler WebSocket commands
 * @param  cmd        Command string (after prefix stripping)
 * @param  json_doc   Pointer to ArduinoJson document
 * @param  client_id  WebSocket client ID
 * @return void
 */
void smrt_sched_ws_handler(const char *cmd, void *json_doc, uint32_t client_id);

/**
 * @brief  Adds scheduler telemetry to a JSON object
 * @param  json_obj  Pointer to ArduinoJson object
 * @return void
 */
void smrt_sched_get_telemetry(void *json_obj);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_SCHED_H
