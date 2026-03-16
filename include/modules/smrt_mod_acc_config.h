/**
 * @file    smrt_mod_acc_config.h
 * @brief   Configuration defines for the Access Control module
 * @project HOMENODE
 * @version 0.7.0
 *
 * SPI pin assignments for MFRC522, lock relay, UID storage, event log.
 * WARNING: GPIO2 (lock relay) shared with PLUG module relay.
 */

#ifndef SMRT_MOD_ACC_CONFIG_H
#define SMRT_MOD_ACC_CONFIG_H

//-----------------------------------------------------------------------------
// SPI pin assignments (MFRC522 NFC reader)
//-----------------------------------------------------------------------------
#define SMRT_ACC_SPI_SCK            18      /**< GPIO18 — SPI clock */
#define SMRT_ACC_SPI_MISO           19      /**< GPIO19 — SPI MISO */
#define SMRT_ACC_SPI_MOSI           23      /**< GPIO23 — SPI MOSI */
#define SMRT_ACC_SPI_SS             5       /**< GPIO5  — SPI slave select */
#define SMRT_ACC_SPI_RST            27      /**< GPIO27 — MFRC522 reset */

//-----------------------------------------------------------------------------
// Lock relay
//-----------------------------------------------------------------------------
#define SMRT_ACC_LOCK_PIN           2       /**< GPIO2  — Lock relay output */
#define SMRT_ACC_PULSE_MIN_MS       500     /**< Minimum pulse duration (ms) */
#define SMRT_ACC_PULSE_MAX_MS       15000   /**< Maximum pulse duration (ms) */
#define SMRT_ACC_PULSE_DEFAULT_MS   3000    /**< Default pulse duration (ms) */

//-----------------------------------------------------------------------------
// UID storage
//-----------------------------------------------------------------------------
#define SMRT_ACC_MAX_UIDS           20      /**< Maximum authorized UIDs */
#define SMRT_ACC_UID_MAX_BYTES      10      /**< Maximum bytes per UID */
#define SMRT_ACC_UID_STR_LEN        30      /**< Max UID string "XX:XX:...:XX" */

//-----------------------------------------------------------------------------
// NFC lockout (anti-brute-force)
//-----------------------------------------------------------------------------
#define SMRT_ACC_MAX_FAILED_ATTEMPTS 5      /**< Failed NFC reads before lockout */
#define SMRT_ACC_LOCKOUT_MS         300000  /**< Lockout duration (5 minutes) */

//-----------------------------------------------------------------------------
// Event log
//-----------------------------------------------------------------------------
#define SMRT_ACC_EVENT_LOG_SIZE     16      /**< Max events in circular buffer */
#define SMRT_ACC_EVENT_MSG_LEN      48      /**< Max chars per event message */

//-----------------------------------------------------------------------------
// NVS keys (namespace: "acc")
//-----------------------------------------------------------------------------
#define SMRT_ACC_NVS_NAMESPACE      "acc"           /**< NVS namespace */
#define SMRT_ACC_NVS_KEY_UID_CNT    "uid_cnt"       /**< Authorized UID count */
#define SMRT_ACC_NVS_KEY_UID_PFX    "uid_"          /**< UID prefix: uid_0..uid_19 */
#define SMRT_ACC_NVS_KEY_PULSE      "pulse_ms"      /**< Pulse duration */
#define SMRT_ACC_NVS_KEY_EVENTS     "events"        /**< Event log JSON blob */

#endif // SMRT_MOD_ACC_CONFIG_H
