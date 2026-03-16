/**
 * @file    smrt_mod_rly.cpp
 * @brief   Relay control module — 1 to 4 relay outputs with pulse mode
 * @project HOMENODE
 * @version 0.4.0
 *
 * Implements the smrt_module_t interface for multi-relay control.
 * Hardware: Up to 4 GPIO outputs (configurable via smrt_mod_rly_config.h).
 *
 * Architecture:
 *   - Static arrays hold relay states, names, and pulse tracking
 *   - rly_loop() manages non-blocking pulse deactivation via millis()
 *   - rly_ws_handler() dispatches toggle, set, pulse, and config commands
 *   - rly_get_telemetry() fills JSON with relay states and configuration
 *
 * Testability:
 *   - Utility functions (validation, getters/setters) are always compiled
 *   - Hardware-dependent code (GPIO, WS, NVS) guarded by #ifndef UNIT_TEST
 */

//=============================================================================
// Includes
//=============================================================================
#ifndef UNIT_TEST
#include "smrt_core.h"
#include "smrt_mod_rly.h"
#else
#include "smrt_mod_rly.h"
#include <string.h>
#endif

//=============================================================================
// Static state
//=============================================================================
static int           rly_states[SMRT_RLY_MAX_RELAYS]  = {0, 0, 0, 0};
static int           rly_count                         = SMRT_RLY_DEFAULT_COUNT;
static unsigned long rly_pulse_ms                      = SMRT_RLY_PULSE_DEFAULT_MS;

#ifndef UNIT_TEST
static const int     rly_pins[SMRT_RLY_MAX_RELAYS] = {
    SMRT_RLY_PIN_1, SMRT_RLY_PIN_2, SMRT_RLY_PIN_3, SMRT_RLY_PIN_4
};
static char          rly_names[SMRT_RLY_MAX_RELAYS][SMRT_RLY_NAME_MAX_LEN];
static unsigned long rly_pulse_start[SMRT_RLY_MAX_RELAYS] = {0, 0, 0, 0};
static int           rly_pulse_active[SMRT_RLY_MAX_RELAYS] = {0, 0, 0, 0};
static bool          rly_state_dirty = false;                /**< Dirty flag for debounced NVS writes */
static unsigned long rly_state_last_nvs_ms = 0;              /**< Last state NVS write timestamp */
#endif

//=============================================================================
// Testable utility functions (always compiled)
//=============================================================================

/**
 * @brief  Validates pulse duration is within allowed bounds.
 * @param  ms  Proposed pulse duration
 * @return 1 if valid, 0 otherwise
 */
int smrt_rly_validate_pulse(unsigned long ms) {
    if (ms >= SMRT_RLY_PULSE_MIN_MS && ms <= SMRT_RLY_PULSE_MAX_MS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Validates relay count is within 1..SMRT_RLY_MAX_RELAYS.
 * @param  count  Proposed relay count
 * @return 1 if valid, 0 otherwise
 */
int smrt_rly_validate_count(int count) {
    if (count >= 1 && count <= SMRT_RLY_MAX_RELAYS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Returns relay state at index.
 * @param  index  Relay index (0-based)
 * @return 1=ON, 0=OFF, -1=invalid index
 */
int smrt_rly_get_state(int index) {
    if (index < 0 || index >= SMRT_RLY_MAX_RELAYS) {
        return -1;
    }
    return rly_states[index];
}

/**
 * @brief  Sets relay state at index (RAM only).
 * @param  index  Relay index
 * @param  state  0=OFF, 1=ON
 * @return void
 */
void smrt_rly_set_state(int index, int state) {
    if (index >= 0 && index < SMRT_RLY_MAX_RELAYS) {
        rly_states[index] = (state != 0) ? 1 : 0;
    }
}

/**
 * @brief  Returns active relay count.
 * @return Count (1..4)
 */
int smrt_rly_get_count(void) {
    return rly_count;
}

/**
 * @brief  Sets active relay count.
 * @param  count  New count
 * @return void
 */
void smrt_rly_set_count(int count) {
    rly_count = count;
}

/**
 * @brief  Returns pulse duration.
 * @return Duration in ms
 */
unsigned long smrt_rly_get_pulse(void) {
    return rly_pulse_ms;
}

/**
 * @brief  Sets pulse duration.
 * @param  ms  New duration
 * @return void
 */
void smrt_rly_set_pulse(unsigned long ms) {
    rly_pulse_ms = ms;
}

//=============================================================================
// Hardware-dependent code (ESP32 only)
//=============================================================================
#ifndef UNIT_TEST

/**
 * @brief  Applies the RAM state of a relay to the GPIO pin.
 * @param  index  Relay index
 * @return void
 */
static void rly_apply_gpio(int index) {
    if (index >= 0 && index < rly_count) {
        digitalWrite(rly_pins[index], rly_states[index] ? HIGH : LOW);
    }
}

/**
 * @brief  Marks relay states as dirty for deferred NVS persistence.
 *         Actual write happens in rly_loop() with debounce.
 * @return void
 */
static void rly_save_states(void) {
    rly_state_dirty = true;
}

/**
 * @brief  Writes relay states bitmask to NVS (called from loop with debounce).
 * @return void
 */
static void rly_flush_states_nvs(void) {
    int32_t bitmask = 0;
    int i;
    for (i = 0; i < SMRT_RLY_MAX_RELAYS; i++) {
        if (rly_states[i]) {
            bitmask |= (1 << i);
        }
    }
    smrt_nvs_set_int(SMRT_RLY_NVS_NAMESPACE, SMRT_RLY_NVS_KEY_STATES, bitmask);
    rly_state_dirty = false;
    rly_state_last_nvs_ms = millis();
}

//-----------------------------------------------------------------------------
// Module callbacks
//-----------------------------------------------------------------------------

extern AsyncWebSocket smrt_ws;

/**
 * @brief  Module init — configures GPIO, loads NVS config.
 * @return void
 */
static void rly_init(void) {
    int i;

    /* Configure pins and set LOW */
    for (i = 0; i < SMRT_RLY_MAX_RELAYS; i++) {
        pinMode(rly_pins[i], OUTPUT);
        digitalWrite(rly_pins[i], LOW);
        memset(rly_names[i], 0, SMRT_RLY_NAME_MAX_LEN);
    }

    /* Load relay count from NVS */
    int32_t saved_count = 0;
    smrt_nvs_get_int(SMRT_RLY_NVS_NAMESPACE, SMRT_RLY_NVS_KEY_COUNT,
                     &saved_count, (int32_t)SMRT_RLY_DEFAULT_COUNT);
    if (smrt_rly_validate_count((int)saved_count)) {
        rly_count = (int)saved_count;
    }

    /* Load pulse duration from NVS */
    int32_t saved_pulse = 0;
    smrt_nvs_get_int(SMRT_RLY_NVS_NAMESPACE, SMRT_RLY_NVS_KEY_PULSE,
                     &saved_pulse, (int32_t)SMRT_RLY_PULSE_DEFAULT_MS);
    if (smrt_rly_validate_pulse((unsigned long)saved_pulse)) {
        rly_pulse_ms = (unsigned long)saved_pulse;
    }

    /* Load relay states bitmask from NVS and restore GPIOs */
    int32_t saved_states = 0;
    smrt_nvs_get_int(SMRT_RLY_NVS_NAMESPACE, SMRT_RLY_NVS_KEY_STATES,
                     &saved_states, 0);
    for (i = 0; i < rly_count; i++) {
        rly_states[i] = (saved_states & (1 << i)) ? 1 : 0;
        rly_apply_gpio(i);
    }

    /* Load relay names from NVS */
    for (i = 0; i < SMRT_RLY_MAX_RELAYS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "%s%d", SMRT_RLY_NVS_KEY_NAME_PFX, i);
        char buf[SMRT_RLY_NAME_MAX_LEN];
        if (smrt_nvs_get_string(SMRT_RLY_NVS_NAMESPACE, key, buf, sizeof(buf))) {
            strncpy(rly_names[i], buf, SMRT_RLY_NAME_MAX_LEN - 1);
        } else {
            snprintf(rly_names[i], SMRT_RLY_NAME_MAX_LEN, "Rele %d", i + 1);
        }
    }

    Serial.println("[RLY] Module initialized ("
                   + String(rly_count) + " relays, pulse="
                   + String(rly_pulse_ms) + "ms)");
}

/**
 * @brief  Module loop — manages non-blocking pulse deactivation.
 * @return void
 */
static void rly_loop(void) {
    unsigned long now = millis();
    int i;
    for (i = 0; i < rly_count; i++) {
        if (rly_pulse_active[i] && (now - rly_pulse_start[i] >= rly_pulse_ms)) {
            rly_states[i] = 0;
            rly_apply_gpio(i);
            rly_pulse_active[i] = 0;
            rly_save_states();
        }
    }

    /* Debounced NVS write for state changes */
    if (rly_state_dirty && (now - rly_state_last_nvs_ms >= SMRT_NVS_STATE_DEBOUNCE_MS)) {
        rly_flush_states_nvs();
    }
}

//-----------------------------------------------------------------------------
// WebSocket sub-command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Broadcasts the full relay status to all WS clients.
 * @return void
 */
static void rly_send_status(void) {
    JsonDocument resp;
    resp["type"]     = "rly_status";
    resp["count"]    = rly_count;
    resp["pulse_ms"] = rly_pulse_ms;

    JsonArray st = resp["states"].to<JsonArray>();
    JsonArray nm = resp["names"].to<JsonArray>();
    int i;
    for (i = 0; i < rly_count; i++) {
        st.add(rly_states[i]);
        nm.add(rly_names[i]);
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Handles toggle sub-command.
 * @param  doc  JSON document with "index" field
 * @return void
 */
static void rly_handle_toggle(JsonDocument &doc) {
    int idx = doc["index"] | 0;
    if (idx < 0 || idx >= rly_count) {
        return;
    }
    rly_states[idx] = rly_states[idx] ? 0 : 1;
    rly_apply_gpio(idx);
    rly_save_states();
    rly_send_status();
}

/**
 * @brief  Handles set sub-command.
 * @param  doc  JSON document with "index" and "state" fields
 * @return void
 */
static void rly_handle_set(JsonDocument &doc) {
    int idx = doc["index"] | 0;
    int val = doc["state"] | 0;
    if (idx < 0 || idx >= rly_count) {
        return;
    }
    rly_states[idx] = val ? 1 : 0;
    rly_apply_gpio(idx);
    rly_save_states();
    rly_send_status();
}

/**
 * @brief  Handles pulse sub-command (timed activation).
 * @param  doc  JSON document with "index" field
 * @return void
 */
static void rly_handle_pulse(JsonDocument &doc) {
    int idx = doc["index"] | 0;
    if (idx < 0 || idx >= rly_count) {
        return;
    }
    rly_states[idx] = 1;
    rly_apply_gpio(idx);
    rly_pulse_start[idx]  = millis();
    rly_pulse_active[idx] = 1;
    rly_save_states();
    rly_send_status();
}

/**
 * @brief  Handles set_pulse sub-command.
 * @param  doc  JSON document with "value" field (ms)
 * @return void
 */
static void rly_handle_set_pulse(JsonDocument &doc) {
    JsonDocument resp;
    resp["type"] = "rly_set_pulse";

    if (doc["value"].isNull()) {
        resp["ok"]  = false;
        resp["msg"] = "Campo 'value' requerido";
    } else {
        unsigned long val = doc["value"].as<unsigned long>();
        if (smrt_rly_validate_pulse(val)) {
            rly_pulse_ms = val;
            smrt_nvs_set_int(SMRT_RLY_NVS_NAMESPACE, SMRT_RLY_NVS_KEY_PULSE,
                             (int32_t)val);
            resp["ok"]       = true;
            resp["pulse_ms"] = val;
            resp["msg"]      = "Pulso actualizado";
        } else {
            resp["ok"]  = false;
            resp["msg"] = "Fuera de rango (100-30000 ms)";
        }
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Handles set_count sub-command.
 * @param  doc  JSON document with "value" field (1-4)
 * @return void
 */
static void rly_handle_set_count(JsonDocument &doc) {
    JsonDocument resp;
    resp["type"] = "rly_set_count";

    if (doc["value"].isNull()) {
        resp["ok"]  = false;
        resp["msg"] = "Campo 'value' requerido";
    } else {
        int val = doc["value"].as<int>();
        if (smrt_rly_validate_count(val)) {
            rly_count = val;
            smrt_nvs_set_int(SMRT_RLY_NVS_NAMESPACE, SMRT_RLY_NVS_KEY_COUNT,
                             (int32_t)val);
            resp["ok"]    = true;
            resp["count"] = val;
            resp["msg"]   = "Cantidad actualizada";
        } else {
            resp["ok"]  = false;
            resp["msg"] = "Fuera de rango (1-4)";
        }
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Handles set_name sub-command.
 * @param  doc  JSON with "index" and "name" fields
 * @return void
 */
static void rly_handle_set_name(JsonDocument &doc) {
    int idx = doc["index"] | -1;
    const char *name = doc["name"] | (const char *)NULL;

    if (idx < 0 || idx >= SMRT_RLY_MAX_RELAYS || !name) {
        return;
    }

    strncpy(rly_names[idx], name, SMRT_RLY_NAME_MAX_LEN - 1);
    rly_names[idx][SMRT_RLY_NAME_MAX_LEN - 1] = '\0';

    char key[12];
    snprintf(key, sizeof(key), "%s%d", SMRT_RLY_NVS_KEY_NAME_PFX, idx);
    smrt_nvs_set_string(SMRT_RLY_NVS_NAMESPACE, key, rly_names[idx]);

    rly_send_status();
}

/**
 * @brief  Module WS handler — dispatches relay sub-commands.
 * @param  cmd     Sub-command string (prefix stripped)
 * @param  doc     Pointer to parsed JSON document
 * @param  client  Pointer to WebSocket client (unused, we broadcast)
 * @return void
 */
static void rly_ws_handler(const char *cmd, void *doc, void *client) {
    if (!cmd) {
        return;
    }

    JsonDocument &json = *(JsonDocument *)doc;

    if (strcmp(cmd, "toggle") == 0)    { rly_handle_toggle(json);    return; }
    if (strcmp(cmd, "set") == 0)       { rly_handle_set(json);       return; }
    if (strcmp(cmd, "pulse") == 0)     { rly_handle_pulse(json);     return; }
    if (strcmp(cmd, "set_pulse") == 0) { rly_handle_set_pulse(json); return; }
    if (strcmp(cmd, "set_count") == 0) { rly_handle_set_count(json); return; }
    if (strcmp(cmd, "set_name") == 0)  { rly_handle_set_name(json);  return; }
    if (strcmp(cmd, "status") == 0)    { rly_send_status();          return; }

    Serial.println("[RLY] Unknown sub-command: " + String(cmd));
}

/**
 * @brief  Module telemetry — fills JSON with relay states.
 * @param  data  Pointer to JsonObject scoped to "rly"
 * @return void
 */
static void rly_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["count"]    = rly_count;
    obj["pulse_ms"] = rly_pulse_ms;

    JsonArray st = obj["states"].to<JsonArray>();
    JsonArray nm = obj["names"].to<JsonArray>();
    int i;
    for (i = 0; i < rly_count; i++) {
        st.add(rly_states[i]);
        nm.add(rly_names[i]);
    }
}

//=============================================================================
// Module descriptor
//=============================================================================

const smrt_module_t smrt_mod_rly = {
    "rly",                  /* id */
    "Relay Control",        /* name */
    "0.5.0",                /* version */
    rly_init,               /* init */
    rly_loop,               /* loop */
    rly_ws_handler,         /* ws_handler */
    rly_get_telemetry       /* get_telemetry */
};

#else // UNIT_TEST — minimal descriptor for test registration

const smrt_module_t smrt_mod_rly = {
    "rly",                  /* id */
    "Relay Control",        /* name */
    "0.5.0",                /* version */
    NULL,                   /* init */
    NULL,                   /* loop */
    NULL,                   /* ws_handler */
    NULL                    /* get_telemetry */
};

#endif // UNIT_TEST
