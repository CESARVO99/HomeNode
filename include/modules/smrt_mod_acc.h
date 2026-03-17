/**
 * @file    smrt_mod_acc.h
 * @brief   Access Control module — NFC (MFRC522) + lock relay
 * @project HOMENODE
 * @version 0.7.0
 *
 * NFC-based access control with MFRC522 reader via SPI.
 * Stores up to 20 authorized UIDs. Lock relay pulsed on authorized read.
 * Circular event log for access history.
 *
 * WebSocket sub-commands (prefix "acc_" stripped):
 *   "toggle"        — Toggle lock relay
 *   "status"        — Request full status
 *   "add_uid"       — Add authorized UID (field: "uid" as "XX:XX:XX:XX")
 *   "remove_uid"    — Remove UID (field: "uid")
 *   "list_uids"     — List all authorized UIDs
 *   "clear_uids"    — Remove all UIDs
 *   "set_pulse"     — Set pulse duration (field: "value" in ms)
 *   "get_events"    — Get event log
 *
 * Telemetry (under "modules.acc"):
 *   "locked", "uids", "pulse_ms", "events", "last_event"
 */

#ifndef SMRT_MOD_ACC_H
#define SMRT_MOD_ACC_H

#include "core/smrt_core_module.h"
#include "modules/smrt_mod_acc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE DESCRIPTOR                                         *
 *************************************************************/

extern const smrt_module_t smrt_mod_acc;

/*************************************************************
 * TESTABLE UTILITY FUNCTIONS                                *
 *************************************************************/

/**
 * @brief  Validates pulse duration within bounds.
 * @param  ms  Proposed duration
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_pulse(unsigned long ms);

/**
 * @brief  Validates lockout attempt count (1..100).
 * @param  attempts  Max failed attempts before lockout
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_lockout_attempts(int attempts);

/**
 * @brief  Validates lockout duration (10s..30min).
 * @param  ms  Lockout duration in milliseconds
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_lockout_ms(unsigned long ms);

/**
 * @brief  Validates UID string format ("XX:XX:XX:XX" hex pairs).
 * @param  uid  UID string
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_uid_format(const char *uid);

/**
 * @brief  Adds a UID to the authorized list.
 * @param  uid  UID string ("XX:XX:XX:XX")
 * @return 1 on success, 0 if full or duplicate
 */
int smrt_acc_uid_add(const char *uid);

/**
 * @brief  Removes a UID from the authorized list.
 * @param  uid  UID string
 * @return 1 on success, 0 if not found
 */
int smrt_acc_uid_remove(const char *uid);

/**
 * @brief  Finds a UID in the authorized list.
 * @param  uid  UID string
 * @return Index (0-based) or -1 if not found
 */
int smrt_acc_uid_find(const char *uid);

/**
 * @brief  Returns the number of authorized UIDs.
 * @return UID count
 */
int smrt_acc_uid_count(void);

/**
 * @brief  Returns a UID by index.
 * @param  index  UID index (0-based)
 * @return UID string, or NULL if out of range
 */
const char *smrt_acc_uid_get(int index);

/**
 * @brief  Clears all authorized UIDs.
 * @return void
 */
void smrt_acc_uid_clear(void);

/**
 * @brief  Checks if a UID is authorized.
 * @param  uid  UID string
 * @return 1 if authorized, 0 otherwise
 */
int smrt_acc_uid_is_authorized(const char *uid);

/**
 * @brief  Converts UID bytes to colon-separated hex string.
 * @param  bytes   UID byte array
 * @param  len     Number of bytes
 * @param  out     Output string buffer (must be >= SMRT_ACC_UID_STR_LEN)
 * @param  out_len Output buffer size
 * @return 1 on success, 0 on error
 */
int smrt_acc_uid_bytes_to_str(const unsigned char *bytes, int len,
                               char *out, int out_len);

/**
 * @brief  Converts colon-separated hex string to UID bytes.
 * @param  str     UID string ("XX:XX:XX:XX")
 * @param  out     Output byte array
 * @param  out_len Output array size
 * @return Number of bytes parsed, 0 on error
 */
int smrt_acc_uid_str_to_bytes(const char *str, unsigned char *out, int out_len);

/**
 * @brief  Returns current pulse duration.
 * @return Pulse ms
 */
unsigned long smrt_acc_get_pulse(void);

/**
 * @brief  Sets pulse duration.
 * @param  ms  Duration
 * @return void
 */
void smrt_acc_set_pulse(unsigned long ms);

/**
 * @brief  Returns event count.
 * @return Count
 */
int smrt_acc_get_event_count(void);

/**
 * @brief  Adds an event to the log.
 * @param  msg        Event message
 * @param  timestamp  Timestamp (millis)
 * @return void
 */
void smrt_acc_add_event(const char *msg, unsigned long timestamp);

/**
 * @brief  Returns an event by index.
 * @param  index      Event index (0=oldest)
 * @param  timestamp  Output timestamp (nullable)
 * @return Message string or NULL
 */
const char *smrt_acc_get_event(int index, unsigned long *timestamp);

/**
 * @brief  Clears event log.
 * @return void
 */
void smrt_acc_clear_events(void);

/**
 * @brief  Validates learn mode timeout (5s..120s).
 * @param  ms  Timeout in milliseconds
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_learn_timeout(unsigned long ms);

#ifdef __cplusplus
}
#endif

#endif // SMRT_MOD_ACC_H
