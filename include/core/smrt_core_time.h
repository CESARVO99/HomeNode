/**
 * @file    smrt_core_time.h
 * @brief   NTP time synchronization service
 * @project HOMENODE
 * @version 0.8.0
 *
 * Uses ESP32 built-in configTime() and getLocalTime() (lwIP SNTP).
 * Provides time accessors for the scheduling engine.
 * Timezone is configurable via WebSocket and persisted in NVS.
 */

#ifndef SMRT_CORE_TIME_H
#define SMRT_CORE_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define SMRT_NTP_SERVER_1           "pool.ntp.org"
#define SMRT_NTP_SERVER_2           "time.nist.gov"
#define SMRT_NTP_DEFAULT_GMT_OFF    0       /**< Default GMT offset (seconds) */
#define SMRT_NTP_DEFAULT_DST_OFF    0       /**< Default DST offset (seconds) */
#define SMRT_NTP_NVS_NAMESPACE      "time"
#define SMRT_NTP_SYNC_TIMEOUT_MS    10000   /**< Max wait for initial sync */

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a GMT offset value
 * @param  offset  Offset in seconds (-43200 to +50400)
 * @return 1 if valid, 0 otherwise
 */
int smrt_time_validate_gmt_offset(long offset);

/**
 * @brief  Validates a DST offset value
 * @param  offset  Offset in seconds (0 or 3600)
 * @return 1 if valid, 0 otherwise
 */
int smrt_time_validate_dst_offset(long offset);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/**
 * @brief  Initializes NTP time synchronization
 * @return void
 */
void smrt_time_init(void);

/**
 * @brief  Checks if NTP time is synchronized
 * @return 1 if synced, 0 otherwise
 */
int smrt_time_is_synced(void);

/**
 * @brief  Returns current hour (0-23)
 * @return Hour, or -1 if not synced
 */
int smrt_time_get_hour(void);

/**
 * @brief  Returns current minute (0-59)
 * @return Minute, or -1 if not synced
 */
int smrt_time_get_minute(void);

/**
 * @brief  Returns current day of week (0=Sunday, 6=Saturday)
 * @return Day of week, or -1 if not synced
 */
int smrt_time_get_dow(void);

/**
 * @brief  Returns current second (0-59)
 * @return Second, or -1 if not synced
 */
int smrt_time_get_second(void);

/**
 * @brief  Sets timezone offsets and re-initializes NTP
 * @param  gmt_offset  GMT offset in seconds
 * @param  dst_offset  DST offset in seconds
 * @return void
 */
void smrt_time_set_timezone(long gmt_offset, long dst_offset);

/**
 * @brief  Saves timezone to NVS
 * @param  gmt_offset  GMT offset in seconds
 * @param  dst_offset  DST offset in seconds
 * @return void
 */
void smrt_time_save_timezone(long gmt_offset, long dst_offset);

/**
 * @brief  Returns formatted time string (HH:MM:SS)
 * @param  buf      Output buffer
 * @param  buf_len  Buffer size (min 9)
 * @return void
 */
void smrt_time_get_formatted(char *buf, size_t buf_len);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_TIME_H
