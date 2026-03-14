/**
 * @file    smrt_mod_sec.cpp
 * @brief   Security module — PIR, reed switch, alarm state machine
 * @project HOMENODE
 * @version 0.5.0
 *
 * Implements the smrt_module_t interface for home security monitoring.
 * Hardware: PIR (GPIO12), Reed switch (GPIO13), Vibration (GPIO14), Buzzer (GPIO25).
 *
 * Architecture:
 *   - Pure state machine logic is testable (smrt_sec_transition)
 *   - Circular event log stores last 16 events with timestamps
 *   - sec_loop() polls sensors with debounce and feeds state machine
 *   - sec_ws_handler() handles arm/disarm and config commands
 */

//=============================================================================
// Includes
//=============================================================================
#ifndef UNIT_TEST
#include "smrt_core.h"
#include "smrt_mod_sec.h"
#else
#include "smrt_mod_sec.h"
#include <string.h>
#endif

//=============================================================================
// Static state
//=============================================================================
static int           sec_alarm_state   = SMRT_SEC_STATE_DISARMED;
static unsigned long sec_entry_delay   = SMRT_SEC_ENTRY_DELAY_MS;
static unsigned long sec_exit_delay    = SMRT_SEC_EXIT_DELAY_MS;

/* Circular event log */
typedef struct {
    char          msg[SMRT_SEC_EVENT_MSG_LEN];
    unsigned long timestamp;
} sec_event_t;

static sec_event_t  sec_events[SMRT_SEC_EVENT_LOG_SIZE];
static int          sec_event_head  = 0;
static int          sec_event_count = 0;

#ifndef UNIT_TEST
static unsigned long sec_delay_start  = 0;
static int           sec_pir_last      = 0;
static int           sec_reed_last     = 0;
static int           sec_vibr_last     = 0;
static unsigned long sec_pir_debounce  = 0;
static unsigned long sec_reed_debounce = 0;
static unsigned long sec_vibr_debounce = 0;
static unsigned long sec_events_last_nvs_ms = 0;
#endif

//=============================================================================
// Testable utility functions (always compiled)
//=============================================================================

/**
 * @brief  Validates delay value within bounds.
 * @param  ms  Proposed delay
 * @return 1 valid, 0 invalid
 */
int smrt_sec_validate_delay(unsigned long ms) {
    if (ms >= SMRT_SEC_DELAY_MIN_MS && ms <= SMRT_SEC_DELAY_MAX_MS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Returns current alarm state.
 * @return State constant
 */
int smrt_sec_get_alarm_state(void) {
    return sec_alarm_state;
}

/**
 * @brief  Sets alarm state (for testing).
 * @param  state  New state
 * @return void
 */
void smrt_sec_set_alarm_state(int state) {
    sec_alarm_state = state;
}

/**
 * @brief  Pure state machine transition.
 * @param  current  Current state
 * @param  event    Event to process
 * @return New state
 */
int smrt_sec_transition(int current, int event) {
    if (event == SMRT_SEC_EVT_DISARM) {
        return SMRT_SEC_STATE_DISARMED;
    }

    switch (current) {
    case SMRT_SEC_STATE_DISARMED:
        if (event == SMRT_SEC_EVT_ARM) {
            return SMRT_SEC_STATE_EXIT_DELAY;
        }
        break;

    case SMRT_SEC_STATE_EXIT_DELAY:
        if (event == SMRT_SEC_EVT_TIMEOUT) {
            return SMRT_SEC_STATE_ARMED;
        }
        break;

    case SMRT_SEC_STATE_ARMED:
        if (event == SMRT_SEC_EVT_MOTION ||
            event == SMRT_SEC_EVT_DOOR_OPEN ||
            event == SMRT_SEC_EVT_VIBRATION) {
            return SMRT_SEC_STATE_ENTRY_DELAY;
        }
        break;

    case SMRT_SEC_STATE_ENTRY_DELAY:
        if (event == SMRT_SEC_EVT_TIMEOUT) {
            return SMRT_SEC_STATE_TRIGGERED;
        }
        break;

    case SMRT_SEC_STATE_TRIGGERED:
        /* Only DISARM can exit triggered (handled above) */
        break;

    default:
        break;
    }

    return current;
}

/**
 * @brief  Returns event count.
 * @return Number of events in log
 */
int smrt_sec_get_event_count(void) {
    return sec_event_count;
}

/**
 * @brief  Adds an event to the circular log.
 * @param  msg        Event message
 * @param  timestamp  Timestamp (millis)
 * @return void
 */
void smrt_sec_add_event(const char *msg, unsigned long timestamp) {
    if (!msg) {
        return;
    }
    strncpy(sec_events[sec_event_head].msg, msg, SMRT_SEC_EVENT_MSG_LEN - 1);
    sec_events[sec_event_head].msg[SMRT_SEC_EVENT_MSG_LEN - 1] = '\0';
    sec_events[sec_event_head].timestamp = timestamp;

    sec_event_head = (sec_event_head + 1) % SMRT_SEC_EVENT_LOG_SIZE;
    if (sec_event_count < SMRT_SEC_EVENT_LOG_SIZE) {
        sec_event_count++;
    }
}

/**
 * @brief  Retrieves an event by index (0=oldest).
 * @param  index      Event index
 * @param  timestamp  Output timestamp (nullable)
 * @return Message string or NULL
 */
const char *smrt_sec_get_event(int index, unsigned long *timestamp) {
    if (index < 0 || index >= sec_event_count) {
        return NULL;
    }
    int pos;
    if (sec_event_count < SMRT_SEC_EVENT_LOG_SIZE) {
        pos = index;
    } else {
        pos = (sec_event_head + index) % SMRT_SEC_EVENT_LOG_SIZE;
    }
    if (timestamp) {
        *timestamp = sec_events[pos].timestamp;
    }
    return sec_events[pos].msg;
}

/**
 * @brief  Clears the event log.
 * @return void
 */
void smrt_sec_clear_events(void) {
    sec_event_head  = 0;
    sec_event_count = 0;
}

/**
 * @brief  Returns entry delay.
 * @return Delay ms
 */
unsigned long smrt_sec_get_entry_delay(void) {
    return sec_entry_delay;
}

/**
 * @brief  Sets entry delay.
 * @param  ms  New delay
 * @return void
 */
void smrt_sec_set_entry_delay(unsigned long ms) {
    sec_entry_delay = ms;
}

/**
 * @brief  Returns exit delay.
 * @return Delay ms
 */
unsigned long smrt_sec_get_exit_delay(void) {
    return sec_exit_delay;
}

/**
 * @brief  Sets exit delay.
 * @param  ms  New delay
 * @return void
 */
void smrt_sec_set_exit_delay(unsigned long ms) {
    sec_exit_delay = ms;
}

//=============================================================================
// Hardware-dependent code (ESP32 only)
//=============================================================================
#ifndef UNIT_TEST

extern AsyncWebSocket smrt_ws;

//-----------------------------------------------------------------------------
// Event log NVS persistence
//-----------------------------------------------------------------------------

/**
 * @brief  Serializes event log to NVS as JSON blob.
 * @return void
 */
static void sec_save_events(void) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    int i;
    for (i = 0; i < sec_event_count; i++) {
        unsigned long ts = 0;
        const char *msg = smrt_sec_get_event(i, &ts);
        if (msg) {
            JsonObject ev = arr.add<JsonObject>();
            ev["m"] = msg;
            ev["t"] = ts;
        }
    }
    String output;
    serializeJson(doc, output);
    smrt_nvs_set_string(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_EVENTS,
                         output.c_str());
}

/**
 * @brief  Restores event log from NVS JSON blob.
 * @return void
 */
static void sec_load_events(void) {
    char buf[1024];
    if (!smrt_nvs_get_string(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_EVENTS,
                              buf, sizeof(buf))) {
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, buf) != DeserializationError::Ok) {
        return;
    }
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject ev : arr) {
        const char *m = ev["m"];
        unsigned long t = ev["t"];
        if (m) {
            smrt_sec_add_event(m, t);
        }
    }
}

/**
 * @brief  Returns state name for JSON and serial.
 * @param  state  Alarm state constant
 * @return Human-readable string
 */
static const char *sec_state_name(int state) {
    switch (state) {
    case SMRT_SEC_STATE_DISARMED:    return "disarmed";
    case SMRT_SEC_STATE_ARMED:       return "armed";
    case SMRT_SEC_STATE_TRIGGERED:   return "triggered";
    case SMRT_SEC_STATE_ENTRY_DELAY: return "entry_delay";
    case SMRT_SEC_STATE_EXIT_DELAY:  return "exit_delay";
    default:                         return "unknown";
    }
}

/**
 * @brief  Broadcasts an immediate alert to all WS clients.
 * @param  trigger  Trigger type string ("motion", "door", "vibration")
 * @return void
 */
static void sec_send_alert(const char *trigger) {
    JsonDocument resp;
    resp["type"]      = "sec_alert";
    resp["trigger"]   = trigger;
    resp["state"]     = sec_state_name(sec_alarm_state);
    resp["timestamp"] = millis();

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Broadcasts full security status.
 * @return void
 */
static void sec_send_status(void) {
    JsonDocument resp;
    resp["type"]        = "sec_status";
    resp["state"]       = sec_state_name(sec_alarm_state);
    resp["pir"]         = sec_pir_last;
    resp["reed"]        = sec_reed_last;
    resp["vibration"]   = sec_vibr_last;
    resp["entry_delay"] = sec_entry_delay;
    resp["exit_delay"]  = sec_exit_delay;
    resp["events"]      = sec_event_count;

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Processes a state machine event with side effects (GPIO, WS, NVS).
 * @param  event  Event constant
 * @return void
 */
static void sec_process_event(int event) {
    int prev = sec_alarm_state;
    sec_alarm_state = smrt_sec_transition(prev, event);

    if (sec_alarm_state == prev) {
        return;
    }

    /* Handle state entry actions */
    switch (sec_alarm_state) {
    case SMRT_SEC_STATE_EXIT_DELAY:
        sec_delay_start = millis();
        smrt_sec_add_event("Armando (exit delay)", millis());
        Serial.println("[SEC] Exit delay started");
        break;

    case SMRT_SEC_STATE_ARMED:
        smrt_sec_add_event("Sistema armado", millis());
        smrt_nvs_set_bool(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_ARMED, true);
        Serial.println("[SEC] System ARMED");
        break;

    case SMRT_SEC_STATE_ENTRY_DELAY:
        sec_delay_start = millis();
        smrt_sec_add_event("Intrusion detectada", millis());
        Serial.println("[SEC] Entry delay — intrusion detected");
        break;

    case SMRT_SEC_STATE_TRIGGERED:
        digitalWrite(SMRT_SEC_BUZZER_PIN, HIGH);
        smrt_sec_add_event("ALARMA ACTIVADA", millis());
        sec_send_alert("alarm");
        Serial.println("[SEC] *** ALARM TRIGGERED ***");
        break;

    case SMRT_SEC_STATE_DISARMED:
        digitalWrite(SMRT_SEC_BUZZER_PIN, LOW);
        smrt_sec_add_event("Sistema desarmado", millis());
        smrt_nvs_set_bool(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_ARMED, false);
        Serial.println("[SEC] System DISARMED");
        break;

    default:
        break;
    }

    /* Throttled NVS persistence of event log */
    unsigned long now_save = millis();
    if (now_save - sec_events_last_nvs_ms >= SMRT_NVS_WRITE_INTERVAL_MS) {
        sec_events_last_nvs_ms = now_save;
        sec_save_events();
    }

    sec_send_status();
}

//-----------------------------------------------------------------------------
// Module callbacks
//-----------------------------------------------------------------------------

/**
 * @brief  Module init — configures GPIO, loads NVS.
 * @return void
 */
static void sec_init(void) {
    pinMode(SMRT_SEC_PIR_PIN, INPUT);
    pinMode(SMRT_SEC_REED_PIN, INPUT_PULLUP);
    pinMode(SMRT_SEC_VIBR_PIN, INPUT_PULLUP);
    pinMode(SMRT_SEC_BUZZER_PIN, OUTPUT);
    digitalWrite(SMRT_SEC_BUZZER_PIN, LOW);

    /* Load delays from NVS */
    int32_t saved_entry = 0;
    smrt_nvs_get_int(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_ENTRY_DLY,
                     &saved_entry, (int32_t)SMRT_SEC_ENTRY_DELAY_MS);
    if (smrt_sec_validate_delay((unsigned long)saved_entry)) {
        sec_entry_delay = (unsigned long)saved_entry;
    }

    int32_t saved_exit = 0;
    smrt_nvs_get_int(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_EXIT_DLY,
                     &saved_exit, (int32_t)SMRT_SEC_EXIT_DELAY_MS);
    if (smrt_sec_validate_delay((unsigned long)saved_exit)) {
        sec_exit_delay = (unsigned long)saved_exit;
    }

    /* Check if was armed before reboot */
    bool was_armed = false;
    smrt_nvs_get_bool(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_ARMED,
                      &was_armed, false);
    /* Restore event log from NVS */
    sec_load_events();

    if (was_armed) {
        sec_alarm_state = SMRT_SEC_STATE_ARMED;
        smrt_sec_add_event("Rearmed after reboot", millis());
    }

    Serial.println("[SEC] Module initialized (entry="
                   + String(sec_entry_delay) + "ms, exit="
                   + String(sec_exit_delay) + "ms)");
}

/**
 * @brief  Module loop — polls sensors, manages delay timers.
 * @return void
 */
static void sec_loop(void) {
    unsigned long now = millis();

    /* Check delay timeouts */
    if (sec_alarm_state == SMRT_SEC_STATE_EXIT_DELAY) {
        if (now - sec_delay_start >= sec_exit_delay) {
            sec_process_event(SMRT_SEC_EVT_TIMEOUT);
        }
        return;
    }

    if (sec_alarm_state == SMRT_SEC_STATE_ENTRY_DELAY) {
        if (now - sec_delay_start >= sec_entry_delay) {
            sec_process_event(SMRT_SEC_EVT_TIMEOUT);
        }
        return;
    }

    /* Only poll sensors when ARMED */
    if (sec_alarm_state != SMRT_SEC_STATE_ARMED) {
        return;
    }

    /* PIR — active HIGH, debounced */
    int pir = digitalRead(SMRT_SEC_PIR_PIN);
    if (pir && !sec_pir_last && (now - sec_pir_debounce >= SMRT_SEC_DEBOUNCE_MS)) {
        sec_pir_debounce = now;
        smrt_sec_add_event("Movimiento detectado", now);
        sec_send_alert("motion");
        sec_process_event(SMRT_SEC_EVT_MOTION);
    }
    sec_pir_last = pir;

    /* Reed — active LOW (open = HIGH), debounced */
    int reed = digitalRead(SMRT_SEC_REED_PIN);
    if (reed && !sec_reed_last && (now - sec_reed_debounce >= SMRT_SEC_DEBOUNCE_MS)) {
        sec_reed_debounce = now;
        smrt_sec_add_event("Puerta/ventana abierta", now);
        sec_send_alert("door");
        sec_process_event(SMRT_SEC_EVT_DOOR_OPEN);
    }
    sec_reed_last = reed;

    /* Vibration — active HIGH (shock = HIGH), debounced */
    int vibr = digitalRead(SMRT_SEC_VIBR_PIN);
    if (vibr && !sec_vibr_last && (now - sec_vibr_debounce >= SMRT_SEC_DEBOUNCE_MS)) {
        sec_vibr_debounce = now;
        smrt_sec_add_event("Vibracion detectada", now);
        sec_send_alert("vibration");
        sec_process_event(SMRT_SEC_EVT_VIBRATION);
    }
    sec_vibr_last = vibr;
}

//-----------------------------------------------------------------------------
// WebSocket sub-command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Module WS handler — dispatches security sub-commands.
 * @param  cmd     Sub-command (prefix stripped)
 * @param  doc     Parsed JSON document
 * @param  client  WS client (unused)
 * @return void
 */
static void sec_ws_handler(const char *cmd, void *doc, void *client) {
    if (!cmd) {
        return;
    }

    JsonDocument &json = *(JsonDocument *)doc;

    if (strcmp(cmd, "arm") == 0) {
        sec_process_event(SMRT_SEC_EVT_ARM);
        return;
    }

    if (strcmp(cmd, "disarm") == 0) {
        sec_process_event(SMRT_SEC_EVT_DISARM);
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        sec_send_status();
        return;
    }

    if (strcmp(cmd, "set_entry_delay") == 0) {
        JsonDocument resp;
        resp["type"] = "sec_set_entry_delay";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            unsigned long val = json["value"].as<unsigned long>();
            if (smrt_sec_validate_delay(val)) {
                sec_entry_delay = val;
                smrt_nvs_set_int(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_ENTRY_DLY,
                                 (int32_t)val);
                resp["ok"]          = true;
                resp["entry_delay"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (5000-120000 ms)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "set_exit_delay") == 0) {
        JsonDocument resp;
        resp["type"] = "sec_set_exit_delay";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            unsigned long val = json["value"].as<unsigned long>();
            if (smrt_sec_validate_delay(val)) {
                sec_exit_delay = val;
                smrt_nvs_set_int(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_EXIT_DLY,
                                 (int32_t)val);
                resp["ok"]         = true;
                resp["exit_delay"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (5000-120000 ms)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "get_events") == 0) {
        JsonDocument resp;
        resp["type"] = "sec_events";
        JsonArray arr = resp["events"].to<JsonArray>();
        int i;
        for (i = 0; i < sec_event_count; i++) {
            unsigned long ts = 0;
            const char *msg = smrt_sec_get_event(i, &ts);
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

    if (strcmp(cmd, "clear_events") == 0) {
        smrt_sec_clear_events();
        smrt_nvs_remove(SMRT_SEC_NVS_NAMESPACE, SMRT_SEC_NVS_KEY_EVENTS);
        sec_send_status();
        return;
    }

    Serial.println("[SEC] Unknown sub-command: " + String(cmd));
}

/**
 * @brief  Module telemetry — fills JSON with security state.
 * @param  data  Pointer to JsonObject
 * @return void
 */
static void sec_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["state"]       = sec_state_name(sec_alarm_state);
    obj["pir"]         = sec_pir_last;
    obj["reed"]        = sec_reed_last;
    obj["vibration"]   = sec_vibr_last;
    obj["entry_delay"] = sec_entry_delay;
    obj["exit_delay"]  = sec_exit_delay;
    obj["events"]      = sec_event_count;
}

//=============================================================================
// Module descriptor
//=============================================================================

const smrt_module_t smrt_mod_sec = {
    "sec",                  /* id */
    "Security",             /* name */
    "0.5.0",                /* version */
    sec_init,               /* init */
    sec_loop,               /* loop */
    sec_ws_handler,         /* ws_handler */
    sec_get_telemetry       /* get_telemetry */
};

#else // UNIT_TEST

const smrt_module_t smrt_mod_sec = {
    "sec",                  /* id */
    "Security",             /* name */
    "0.5.0",                /* version */
    NULL,                   /* init */
    NULL,                   /* loop */
    NULL,                   /* ws_handler */
    NULL                    /* get_telemetry */
};

#endif // UNIT_TEST
