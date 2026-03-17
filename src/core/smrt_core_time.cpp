/**
 * @file    smrt_core_time.cpp
 * @brief   NTP time synchronization implementation
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_time.h"
#else
    #include "smrt_core.h"
    #include <time.h>
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a GMT offset value (UTC-12 to UTC+14)
 * @param  offset  Offset in seconds
 * @return 1 if valid, 0 otherwise
 */
int smrt_time_validate_gmt_offset(long offset) {
    return (offset >= -43200 && offset <= 50400) ? 1 : 0;
}

/**
 * @brief  Validates a DST offset value (0 or 3600)
 * @param  offset  Offset in seconds
 * @return 1 if valid, 0 otherwise
 */
int smrt_time_validate_dst_offset(long offset) {
    return (offset == 0 || offset == 3600) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

//-----------------------------------------------------------------------------
// Static state
//-----------------------------------------------------------------------------
static long smrt_time_gmt_offset = SMRT_NTP_DEFAULT_GMT_OFF;
static long smrt_time_dst_offset = SMRT_NTP_DEFAULT_DST_OFF;
static bool smrt_time_synced = false;

/**
 * @brief  Initializes NTP time synchronization
 * @return void
 */
void smrt_time_init(void) {
    /* Load timezone from NVS */
    int32_t gmt_off, dst_off;
    smrt_nvs_get_int(SMRT_NTP_NVS_NAMESPACE, "gmt_off", &gmt_off, SMRT_NTP_DEFAULT_GMT_OFF);
    smrt_nvs_get_int(SMRT_NTP_NVS_NAMESPACE, "dst_off", &dst_off, SMRT_NTP_DEFAULT_DST_OFF);
    smrt_time_gmt_offset = gmt_off;
    smrt_time_dst_offset = dst_off;

    /* Configure NTP */
    configTime(smrt_time_gmt_offset, smrt_time_dst_offset,
               SMRT_NTP_SERVER_1, SMRT_NTP_SERVER_2);

    /* Wait for initial sync (non-blocking, best-effort) */
    struct tm timeinfo;
    unsigned long start = millis();
    while (!getLocalTime(&timeinfo, 100) && (millis() - start < SMRT_NTP_SYNC_TIMEOUT_MS)) {
        delay(100);
    }

    if (getLocalTime(&timeinfo, 100)) {
        smrt_time_synced = true;
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.println("NTP synced: " + String(buf));
    } else {
        Serial.println("NTP sync pending (will retry in background)");
    }
}

/**
 * @brief  Checks if NTP time is synchronized
 * @return 1 if synced, 0 otherwise
 */
int smrt_time_is_synced(void) {
    if (smrt_time_synced) return 1;
    /* Check again in case SNTP synced in background */
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        smrt_time_synced = true;
        return 1;
    }
    return 0;
}

/**
 * @brief  Returns current hour (0-23)
 */
int smrt_time_get_hour(void) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) return -1;
    smrt_time_synced = true;
    return timeinfo.tm_hour;
}

/**
 * @brief  Returns current minute (0-59)
 */
int smrt_time_get_minute(void) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) return -1;
    return timeinfo.tm_min;
}

/**
 * @brief  Returns current day of week (0=Sunday, 6=Saturday)
 */
int smrt_time_get_dow(void) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) return -1;
    return timeinfo.tm_wday;
}

/**
 * @brief  Returns current second (0-59)
 */
int smrt_time_get_second(void) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) return -1;
    return timeinfo.tm_sec;
}

/**
 * @brief  Sets timezone and reinitializes NTP
 */
void smrt_time_set_timezone(long gmt_offset, long dst_offset) {
    smrt_time_gmt_offset = gmt_offset;
    smrt_time_dst_offset = dst_offset;
    configTime(smrt_time_gmt_offset, smrt_time_dst_offset,
               SMRT_NTP_SERVER_1, SMRT_NTP_SERVER_2);
    SMRT_DEBUG_PRINTF("NTP: Timezone set to GMT%+ld DST%ld\n",
                      gmt_offset / 3600, dst_offset / 3600);
}

/**
 * @brief  Saves timezone to NVS
 */
void smrt_time_save_timezone(long gmt_offset, long dst_offset) {
    smrt_nvs_set_int(SMRT_NTP_NVS_NAMESPACE, "gmt_off", (int32_t)gmt_offset);
    smrt_nvs_set_int(SMRT_NTP_NVS_NAMESPACE, "dst_off", (int32_t)dst_offset);
}

/**
 * @brief  Returns formatted time string (HH:MM:SS)
 */
void smrt_time_get_formatted(char *buf, size_t buf_len) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        strftime(buf, buf_len, "%H:%M:%S", &timeinfo);
    } else {
        strncpy(buf, "--:--:--", buf_len - 1);
        buf[buf_len - 1] = '\0';
    }
}

#endif // UNIT_TEST
