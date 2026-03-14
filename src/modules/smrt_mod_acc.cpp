/**
 * @file    smrt_mod_acc.cpp
 * @brief   Access Control module — NFC (MFRC522) + lock relay
 * @project HOMENODE
 * @version 0.4.0
 *
 * Implements the smrt_module_t interface for NFC-based access control.
 * Hardware: MFRC522 via SPI, lock relay on GPIO2.
 *
 * Architecture:
 *   - Authorized UIDs stored in static array (persisted via NVS)
 *   - Circular event log for access history
 *   - acc_loop() polls MFRC522, checks UID authorization, pulses relay
 *   - acc_ws_handler() handles UID management and config commands
 *
 * Testability:
 *   - UID management, validation, conversion are always compiled
 *   - Hardware-dependent code (SPI, MFRC522) guarded by #ifndef UNIT_TEST
 */

//=============================================================================
// Includes
//=============================================================================
#ifndef UNIT_TEST
#include "smrt_core.h"
#include "smrt_mod_acc.h"
#include <SPI.h>
#include <MFRC522.h>
#else
#include "smrt_mod_acc.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#endif

//=============================================================================
// Static state
//=============================================================================

/** Authorized UID list */
static char          acc_uids[SMRT_ACC_MAX_UIDS][SMRT_ACC_UID_STR_LEN];
static int           acc_uid_count    = 0;
static unsigned long acc_pulse_ms     = SMRT_ACC_PULSE_DEFAULT_MS;

/** Circular event log */
typedef struct {
    char          msg[SMRT_ACC_EVENT_MSG_LEN];
    unsigned long timestamp;
} acc_event_t;

static acc_event_t   acc_events[SMRT_ACC_EVENT_LOG_SIZE];
static int           acc_event_head  = 0;
static int           acc_event_count = 0;

#ifndef UNIT_TEST
static MFRC522       acc_rfid(SMRT_ACC_SPI_SS, SMRT_ACC_SPI_RST);
static int           acc_lock_state    = 0;   /**< 0=locked, 1=unlocked */
static unsigned long acc_pulse_start   = 0;
static int           acc_pulsing       = 0;
#endif

//=============================================================================
// Testable utility functions (always compiled)
//=============================================================================

/**
 * @brief  Validates pulse duration.
 * @param  ms  Duration
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_pulse(unsigned long ms) {
    if (ms >= SMRT_ACC_PULSE_MIN_MS && ms <= SMRT_ACC_PULSE_MAX_MS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Validates UID format: "XX:XX:XX:XX" (2-10 hex pairs, colon-separated).
 * @param  uid  String
 * @return 1 valid, 0 invalid
 */
int smrt_acc_validate_uid_format(const char *uid) {
    if (!uid || uid[0] == '\0') {
        return 0;
    }

    int bytes = 0;
    int i = 0;
    int len = (int)strlen(uid);

    while (i < len) {
        /* Expect two hex digits */
        if (i + 1 >= len) {
            return 0;
        }
        int c0 = uid[i];
        int c1 = uid[i + 1];
        int is_hex_0 = ((c0 >= '0' && c0 <= '9') ||
                        (c0 >= 'A' && c0 <= 'F') ||
                        (c0 >= 'a' && c0 <= 'f'));
        int is_hex_1 = ((c1 >= '0' && c1 <= '9') ||
                        (c1 >= 'A' && c1 <= 'F') ||
                        (c1 >= 'a' && c1 <= 'f'));
        if (!is_hex_0 || !is_hex_1) {
            return 0;
        }
        bytes++;
        i += 2;

        /* Expect colon or end */
        if (i < len) {
            if (uid[i] != ':') {
                return 0;
            }
            i++;
        }
    }

    if (bytes < 1 || bytes > SMRT_ACC_UID_MAX_BYTES) {
        return 0;
    }
    return 1;
}

/**
 * @brief  Adds UID to authorized list.
 * @param  uid  UID string
 * @return 1 success, 0 full/duplicate/invalid
 */
int smrt_acc_uid_add(const char *uid) {
    if (!uid || !smrt_acc_validate_uid_format(uid)) {
        return 0;
    }
    if (acc_uid_count >= SMRT_ACC_MAX_UIDS) {
        return 0;
    }
    if (smrt_acc_uid_find(uid) >= 0) {
        return 0;  /* Duplicate */
    }
    strncpy(acc_uids[acc_uid_count], uid, SMRT_ACC_UID_STR_LEN - 1);
    acc_uids[acc_uid_count][SMRT_ACC_UID_STR_LEN - 1] = '\0';
    acc_uid_count++;
    return 1;
}

/**
 * @brief  Removes UID from list.
 * @param  uid  UID string
 * @return 1 removed, 0 not found
 */
int smrt_acc_uid_remove(const char *uid) {
    int idx = smrt_acc_uid_find(uid);
    if (idx < 0) {
        return 0;
    }
    /* Shift remaining UIDs down */
    int i;
    for (i = idx; i < acc_uid_count - 1; i++) {
        strncpy(acc_uids[i], acc_uids[i + 1], SMRT_ACC_UID_STR_LEN);
    }
    acc_uid_count--;
    acc_uids[acc_uid_count][0] = '\0';
    return 1;
}

/**
 * @brief  Finds UID index.
 * @param  uid  UID string
 * @return Index or -1
 */
int smrt_acc_uid_find(const char *uid) {
    if (!uid) {
        return -1;
    }
    int i;
    for (i = 0; i < acc_uid_count; i++) {
        if (strcmp(acc_uids[i], uid) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief  Returns UID count.
 * @return Count
 */
int smrt_acc_uid_count(void) {
    return acc_uid_count;
}

/**
 * @brief  Returns UID by index.
 * @param  index  Index
 * @return UID string or NULL
 */
const char *smrt_acc_uid_get(int index) {
    if (index < 0 || index >= acc_uid_count) {
        return NULL;
    }
    return acc_uids[index];
}

/**
 * @brief  Clears all UIDs.
 * @return void
 */
void smrt_acc_uid_clear(void) {
    acc_uid_count = 0;
    int i;
    for (i = 0; i < SMRT_ACC_MAX_UIDS; i++) {
        acc_uids[i][0] = '\0';
    }
}

/**
 * @brief  Checks if UID is authorized.
 * @param  uid  UID string
 * @return 1 authorized, 0 not
 */
int smrt_acc_uid_is_authorized(const char *uid) {
    return (smrt_acc_uid_find(uid) >= 0) ? 1 : 0;
}

/**
 * @brief  Converts bytes to "XX:XX:XX" string.
 * @param  bytes    Byte array
 * @param  len      Byte count
 * @param  out      Output string
 * @param  out_len  Output buffer size
 * @return 1 success, 0 error
 */
int smrt_acc_uid_bytes_to_str(const unsigned char *bytes, int len,
                               char *out, int out_len) {
    if (!bytes || !out || len <= 0 || len > SMRT_ACC_UID_MAX_BYTES) {
        return 0;
    }
    /* Need at least len*3 chars (2 hex + colon) minus trailing colon, +1 for null */
    int needed = len * 3;
    if (needed > out_len) {
        return 0;
    }
    int pos = 0;
    int i;
    for (i = 0; i < len; i++) {
        if (i > 0) {
            out[pos++] = ':';
        }
        pos += snprintf(out + pos, out_len - pos, "%02X", bytes[i]);
    }
    out[pos] = '\0';
    return 1;
}

/**
 * @brief  Converts "XX:XX:XX" string to bytes.
 * @param  str      UID string
 * @param  out      Output byte array
 * @param  out_len  Output array size
 * @return Bytes parsed, 0 on error
 */
int smrt_acc_uid_str_to_bytes(const char *str, unsigned char *out, int out_len) {
    if (!str || !out || out_len <= 0) {
        return 0;
    }
    if (!smrt_acc_validate_uid_format(str)) {
        return 0;
    }

    int bytes = 0;
    int i = 0;
    int len = (int)strlen(str);

    while (i < len && bytes < out_len) {
        unsigned int val = 0;
        if (sscanf(str + i, "%2x", &val) != 1) {
            return 0;
        }
        out[bytes++] = (unsigned char)val;
        i += 2;
        if (i < len && str[i] == ':') {
            i++;
        }
    }
    return bytes;
}

/**
 * @brief  Returns pulse duration.
 * @return ms
 */
unsigned long smrt_acc_get_pulse(void) {
    return acc_pulse_ms;
}

/**
 * @brief  Sets pulse duration.
 * @param  ms  Duration
 * @return void
 */
void smrt_acc_set_pulse(unsigned long ms) {
    acc_pulse_ms = ms;
}

/**
 * @brief  Returns event count.
 * @return Count
 */
int smrt_acc_get_event_count(void) {
    return acc_event_count;
}

/**
 * @brief  Adds event to circular log.
 * @param  msg        Message
 * @param  timestamp  Timestamp
 * @return void
 */
void smrt_acc_add_event(const char *msg, unsigned long timestamp) {
    if (!msg) {
        return;
    }
    strncpy(acc_events[acc_event_head].msg, msg, SMRT_ACC_EVENT_MSG_LEN - 1);
    acc_events[acc_event_head].msg[SMRT_ACC_EVENT_MSG_LEN - 1] = '\0';
    acc_events[acc_event_head].timestamp = timestamp;

    acc_event_head = (acc_event_head + 1) % SMRT_ACC_EVENT_LOG_SIZE;
    if (acc_event_count < SMRT_ACC_EVENT_LOG_SIZE) {
        acc_event_count++;
    }
}

/**
 * @brief  Returns event by index (0=oldest).
 * @param  index      Index
 * @param  timestamp  Output timestamp (nullable)
 * @return Message or NULL
 */
const char *smrt_acc_get_event(int index, unsigned long *timestamp) {
    if (index < 0 || index >= acc_event_count) {
        return NULL;
    }
    int pos;
    if (acc_event_count < SMRT_ACC_EVENT_LOG_SIZE) {
        pos = index;
    } else {
        pos = (acc_event_head + index) % SMRT_ACC_EVENT_LOG_SIZE;
    }
    if (timestamp) {
        *timestamp = acc_events[pos].timestamp;
    }
    return acc_events[pos].msg;
}

/**
 * @brief  Clears event log.
 * @return void
 */
void smrt_acc_clear_events(void) {
    acc_event_head  = 0;
    acc_event_count = 0;
}

//=============================================================================
// Hardware-dependent code (ESP32 only)
//=============================================================================
#ifndef UNIT_TEST

extern AsyncWebSocket smrt_ws;

/**
 * @brief  Saves all UIDs to NVS.
 * @return void
 */
static void acc_save_uids(void) {
    smrt_nvs_set_int(SMRT_ACC_NVS_NAMESPACE, SMRT_ACC_NVS_KEY_UID_CNT,
                     acc_uid_count);
    int i;
    for (i = 0; i < acc_uid_count; i++) {
        char key[12];
        snprintf(key, sizeof(key), "%s%d", SMRT_ACC_NVS_KEY_UID_PFX, i);
        smrt_nvs_set_string(SMRT_ACC_NVS_NAMESPACE, key, acc_uids[i]);
    }
}

/**
 * @brief  Loads UIDs from NVS.
 * @return void
 */
static void acc_load_uids(void) {
    int32_t cnt = 0;
    smrt_nvs_get_int(SMRT_ACC_NVS_NAMESPACE, SMRT_ACC_NVS_KEY_UID_CNT,
                     &cnt, 0);
    if (cnt > SMRT_ACC_MAX_UIDS) {
        cnt = SMRT_ACC_MAX_UIDS;
    }
    acc_uid_count = 0;
    int i;
    for (i = 0; i < cnt; i++) {
        char key[12];
        snprintf(key, sizeof(key), "%s%d", SMRT_ACC_NVS_KEY_UID_PFX, i);
        char buf[SMRT_ACC_UID_STR_LEN] = {0};
        bool found = smrt_nvs_get_string(SMRT_ACC_NVS_NAMESPACE, key,
                                          buf, sizeof(buf));
        if (found && buf[0] != '\0' && smrt_acc_validate_uid_format(buf)) {
            strncpy(acc_uids[acc_uid_count], buf, SMRT_ACC_UID_STR_LEN - 1);
            acc_uids[acc_uid_count][SMRT_ACC_UID_STR_LEN - 1] = '\0';
            acc_uid_count++;
        }
    }
}

/**
 * @brief  Sends full access status via WS.
 * @return void
 */
static void acc_send_status(void) {
    JsonDocument resp;
    resp["type"]     = "acc_status";
    resp["locked"]   = !acc_lock_state;
    resp["uids"]     = acc_uid_count;
    resp["pulse_ms"] = acc_pulse_ms;
    resp["events"]   = acc_event_count;

    if (acc_event_count > 0) {
        unsigned long ts = 0;
        const char *last = smrt_acc_get_event(acc_event_count - 1, &ts);
        if (last) {
            resp["last_event"] = last;
        }
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

//-----------------------------------------------------------------------------
// Module callbacks
//-----------------------------------------------------------------------------

/**
 * @brief  Module init — starts SPI, MFRC522, loads NVS.
 * @return void
 */
static void acc_init(void) {
    /* Lock relay */
    pinMode(SMRT_ACC_LOCK_PIN, OUTPUT);
    digitalWrite(SMRT_ACC_LOCK_PIN, LOW);

    /* SPI + MFRC522 */
    SPI.begin(SMRT_ACC_SPI_SCK, SMRT_ACC_SPI_MISO,
              SMRT_ACC_SPI_MOSI, SMRT_ACC_SPI_SS);
    acc_rfid.PCD_Init();

    /* Load pulse duration */
    int32_t saved_pulse = 0;
    smrt_nvs_get_int(SMRT_ACC_NVS_NAMESPACE, SMRT_ACC_NVS_KEY_PULSE,
                     &saved_pulse, (int32_t)SMRT_ACC_PULSE_DEFAULT_MS);
    if (smrt_acc_validate_pulse((unsigned long)saved_pulse)) {
        acc_pulse_ms = (unsigned long)saved_pulse;
    }

    /* Load UIDs */
    acc_load_uids();

    Serial.println("[ACC] Module initialized (UIDs="
                   + String(acc_uid_count)
                   + ", pulse=" + String(acc_pulse_ms) + "ms)");
}

/**
 * @brief  Module loop — polls MFRC522, manages lock pulse.
 * @return void
 */
static void acc_loop(void) {
    unsigned long now = millis();

    /* Handle pulse timeout */
    if (acc_pulsing) {
        if (now - acc_pulse_start >= acc_pulse_ms) {
            acc_pulsing = 0;
            acc_lock_state = 0;
            digitalWrite(SMRT_ACC_LOCK_PIN, LOW);
            Serial.println("[ACC] Lock LOCKED (pulse done)");
            acc_send_status();
        }
        return;
    }

    /* Poll NFC reader */
    if (!acc_rfid.PICC_IsNewCardPresent()) {
        return;
    }
    if (!acc_rfid.PICC_ReadCardSerial()) {
        return;
    }

    /* Convert UID to string */
    char uid_str[SMRT_ACC_UID_STR_LEN];
    smrt_acc_uid_bytes_to_str(acc_rfid.uid.uidByte, acc_rfid.uid.size,
                               uid_str, sizeof(uid_str));

    /* Check authorization */
    if (smrt_acc_uid_is_authorized(uid_str)) {
        /* Unlock */
        acc_lock_state = 1;
        acc_pulsing = 1;
        acc_pulse_start = now;
        digitalWrite(SMRT_ACC_LOCK_PIN, HIGH);

        char evt_msg[SMRT_ACC_EVENT_MSG_LEN];
        snprintf(evt_msg, sizeof(evt_msg), "OK: %s", uid_str);
        smrt_acc_add_event(evt_msg, now);

        Serial.println("[ACC] Authorized: " + String(uid_str));

        JsonDocument resp;
        resp["type"]  = "acc_access";
        resp["uid"]   = uid_str;
        resp["ok"]    = true;
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
    } else {
        char evt_msg[SMRT_ACC_EVENT_MSG_LEN];
        snprintf(evt_msg, sizeof(evt_msg), "DENIED: %s", uid_str);
        smrt_acc_add_event(evt_msg, now);

        Serial.println("[ACC] DENIED: " + String(uid_str));

        JsonDocument resp;
        resp["type"]  = "acc_access";
        resp["uid"]   = uid_str;
        resp["ok"]    = false;
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
    }

    acc_rfid.PICC_HaltA();
    acc_send_status();
}

//-----------------------------------------------------------------------------
// WebSocket sub-command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Module WS handler.
 * @param  cmd     Sub-command
 * @param  doc     JSON document
 * @param  client  WS client
 * @return void
 */
static void acc_ws_handler(const char *cmd, void *doc, void *client) {
    if (!cmd) {
        return;
    }

    JsonDocument &json = *(JsonDocument *)doc;

    if (strcmp(cmd, "toggle") == 0) {
        if (acc_pulsing) {
            /* Cancel pulse, lock */
            acc_pulsing = 0;
            acc_lock_state = 0;
            digitalWrite(SMRT_ACC_LOCK_PIN, LOW);
        } else {
            /* Start pulse unlock */
            acc_lock_state = 1;
            acc_pulsing = 1;
            acc_pulse_start = millis();
            digitalWrite(SMRT_ACC_LOCK_PIN, HIGH);
        }
        smrt_acc_add_event("Manual toggle", millis());
        acc_send_status();
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        acc_send_status();
        return;
    }

    if (strcmp(cmd, "add_uid") == 0) {
        JsonDocument resp;
        resp["type"] = "acc_add_uid";
        const char *uid = json["uid"].as<const char *>();
        if (!uid) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'uid' requerido";
        } else if (smrt_acc_uid_add(uid)) {
            acc_save_uids();
            resp["ok"]   = true;
            resp["uid"]  = uid;
            resp["uids"] = acc_uid_count;
        } else {
            resp["ok"]  = false;
            resp["msg"] = "UID invalido, duplicado o lista llena";
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "remove_uid") == 0) {
        JsonDocument resp;
        resp["type"] = "acc_remove_uid";
        const char *uid = json["uid"].as<const char *>();
        if (!uid) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'uid' requerido";
        } else if (smrt_acc_uid_remove(uid)) {
            acc_save_uids();
            resp["ok"]   = true;
            resp["uid"]  = uid;
            resp["uids"] = acc_uid_count;
        } else {
            resp["ok"]  = false;
            resp["msg"] = "UID no encontrado";
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "list_uids") == 0) {
        JsonDocument resp;
        resp["type"] = "acc_uids";
        JsonArray arr = resp["uids"].to<JsonArray>();
        int i;
        for (i = 0; i < acc_uid_count; i++) {
            arr.add(acc_uids[i]);
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "clear_uids") == 0) {
        smrt_acc_uid_clear();
        acc_save_uids();
        acc_send_status();
        return;
    }

    if (strcmp(cmd, "set_pulse") == 0) {
        JsonDocument resp;
        resp["type"] = "acc_set_pulse";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            unsigned long val = json["value"].as<unsigned long>();
            if (smrt_acc_validate_pulse(val)) {
                acc_pulse_ms = val;
                smrt_nvs_set_int(SMRT_ACC_NVS_NAMESPACE,
                                 SMRT_ACC_NVS_KEY_PULSE, (int32_t)val);
                resp["ok"]       = true;
                resp["pulse_ms"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (500-15000 ms)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "get_events") == 0) {
        JsonDocument resp;
        resp["type"] = "acc_events";
        JsonArray arr = resp["events"].to<JsonArray>();
        int i;
        for (i = 0; i < acc_event_count; i++) {
            unsigned long ts = 0;
            const char *msg = smrt_acc_get_event(i, &ts);
            if (msg) {
                JsonObject ev = arr.add<JsonObject>();
                ev["msg"] = msg;
                ev["ts"]  = ts;
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    Serial.println("[ACC] Unknown sub-command: " + String(cmd));
}

/**
 * @brief  Module telemetry.
 * @param  data  JsonObject pointer
 * @return void
 */
static void acc_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["locked"]   = !acc_lock_state;
    obj["uids"]     = acc_uid_count;
    obj["pulse_ms"] = acc_pulse_ms;
    obj["events"]   = acc_event_count;

    if (acc_event_count > 0) {
        unsigned long ts = 0;
        const char *last = smrt_acc_get_event(acc_event_count - 1, &ts);
        if (last) {
            obj["last_event"] = last;
        }
    }
}

//=============================================================================
// Module descriptor
//=============================================================================

const smrt_module_t smrt_mod_acc = {
    "acc",                  /* id */
    "Access Control",       /* name */
    "0.4.0",                /* version */
    acc_init,               /* init */
    acc_loop,               /* loop */
    acc_ws_handler,         /* ws_handler */
    acc_get_telemetry       /* get_telemetry */
};

#else // UNIT_TEST

const smrt_module_t smrt_mod_acc = {
    "acc",                  /* id */
    "Access Control",       /* name */
    "0.4.0",                /* version */
    NULL,                   /* init */
    NULL,                   /* loop */
    NULL,                   /* ws_handler */
    NULL                    /* get_telemetry */
};

#endif // UNIT_TEST
