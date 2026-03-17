/**
 * @file    smrt_mod_env.cpp
 * @brief   Environmental monitoring module — DHT22 temperature & humidity
 * @project HOMENODE
 * @version 0.7.0
 *
 * Implements the smrt_module_t interface for environmental sensor monitoring.
 * Hardware: DHT22 on GPIO4 (configurable via smrt_mod_env_config.h).
 *
 * Architecture:
 *   - Static variables hold last-known sensor values
 *   - env_loop() reads the sensor at configurable intervals
 *   - env_ws_handler() responds to "read" and "set_interval" sub-commands
 *   - env_get_telemetry() fills the JSON object for periodic broadcasts
 *
 * Testability:
 *   - Utility functions (validate_interval, getters/setters) are always compiled
 *   - Hardware-dependent code (DHT library, WS responses) guarded by #ifndef UNIT_TEST
 */

//=============================================================================
// Includes
//=============================================================================
#ifndef UNIT_TEST
#include "smrt_core.h"
#include "smrt_mod_env.h"
#include <DHT.h>
#else
#include "smrt_mod_env.h"
#include <string.h>
#endif

//=============================================================================
// Static state (shared between all callbacks)
//=============================================================================
static float        env_last_temp       = 0.0f;     /**< Last temperature (C) */
static float        env_last_hum        = 0.0f;     /**< Last humidity (%)    */
static int          env_last_ok         = 0;         /**< 1=read OK, 0=error  */
static unsigned long env_read_interval  = SMRT_ENV_READ_INTERVAL_MS;

#ifndef UNIT_TEST
static unsigned long env_last_read_ms   = 0;         /**< Timestamp of last read */
static DHT          env_dht(SMRT_ENV_DHT_PIN, SMRT_ENV_DHT_TYPE);
static float env_alert_temp_hi = SMRT_ENV_TEMP_ALERT_HI;
static float env_alert_temp_lo = SMRT_ENV_TEMP_ALERT_LO;
static float env_alert_hum_hi  = SMRT_ENV_HUM_ALERT_HI;
static float env_alert_hum_lo  = SMRT_ENV_HUM_ALERT_LO;
static bool  env_alerts_enabled = false;
static int   env_last_alert = 0;
#endif

//=============================================================================
// Testable utility functions (always compiled, no hardware dependency)
//=============================================================================

/**
 * @brief  Validates that a read interval is within allowed bounds.
 * @param  ms  Proposed interval in milliseconds
 * @return 1 if valid, 0 otherwise
 */
int smrt_env_validate_interval(unsigned long ms) {
    if (ms >= SMRT_ENV_READ_MIN_MS && ms <= SMRT_ENV_READ_MAX_MS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Returns the current sensor read interval.
 * @return Interval in milliseconds
 */
unsigned long smrt_env_get_interval(void) {
    return env_read_interval;
}

/**
 * @brief  Sets the sensor read interval (caller must validate first).
 * @param  ms  New interval in milliseconds
 * @return void
 */
void smrt_env_set_interval(unsigned long ms) {
    env_read_interval = ms;
}

/**
 * @brief  Returns the last temperature reading.
 * @return Temperature in degrees Celsius
 */
float smrt_env_get_temperature(void) {
    return env_last_temp;
}

/**
 * @brief  Returns the last humidity reading.
 * @return Relative humidity in percent
 */
float smrt_env_get_humidity(void) {
    return env_last_hum;
}

/**
 * @brief  Returns whether the last sensor read was successful.
 * @return 1 if OK, 0 if error
 */
int smrt_env_get_status(void) {
    return env_last_ok;
}

int smrt_env_check_alert(float temp, float hum, float t_hi, float t_lo,
                          float h_hi, float h_lo) {
    int result = 0;
    if (temp > t_hi) result |= SMRT_ENV_ALERT_TEMP_HI;
    if (temp < t_lo) result |= SMRT_ENV_ALERT_TEMP_LO;
    if (hum > h_hi)  result |= SMRT_ENV_ALERT_HUM_HI;
    if (hum < h_lo)  result |= SMRT_ENV_ALERT_HUM_LO;
    return result;
}

int smrt_env_validate_threshold(float value, float min, float max) {
    return (value >= min && value <= max) ? 1 : 0;
}

//=============================================================================
// Hardware-dependent code (ESP32 only)
//=============================================================================
#ifndef UNIT_TEST

/**
 * @brief  Reads the DHT22 sensor and updates static variables.
 *         Applies calibration offsets from config.
 * @return void
 */
static void env_read_sensor(void) {
    for (int attempt = 0; attempt < SMRT_ENV_READ_RETRIES; attempt++) {
        float t = env_dht.readTemperature();
        float h = env_dht.readHumidity();

        if (!isnan(t) && !isnan(h)) {
            env_last_temp = t + SMRT_ENV_TEMP_OFFSET;
            env_last_hum  = h + SMRT_ENV_HUM_OFFSET;
            env_last_ok   = 1;
            return;
        }

        if (attempt < SMRT_ENV_READ_RETRIES - 1) {
            delay(SMRT_ENV_RETRY_DELAY_MS);
        }
    }

    env_last_ok = 0;
    Serial.println("[ENV] Sensor read error after retries");
}

//-----------------------------------------------------------------------------
// Module callbacks
//-----------------------------------------------------------------------------

/**
 * @brief  Module init — starts the DHT sensor, loads interval from NVS.
 * @return void
 */
static void env_init(void) {
    env_dht.begin();

    // Load saved interval from NVS (if any)
    int32_t saved_interval = 0;
    smrt_nvs_get_int(SMRT_ENV_NVS_NAMESPACE,
                     SMRT_ENV_NVS_KEY_INTERVAL,
                     &saved_interval,
                     (int32_t)SMRT_ENV_READ_INTERVAL_MS);
    if (smrt_env_validate_interval((unsigned long)saved_interval)) {
        env_read_interval = (unsigned long)saved_interval;
    }

    int32_t temp_hi, temp_lo, hum_hi, hum_lo;
    bool alert_en;
    if (smrt_nvs_get_int(SMRT_ENV_NVS_NAMESPACE, "temp_hi", &temp_hi, (int32_t)(SMRT_ENV_TEMP_ALERT_HI * 10)))
        env_alert_temp_hi = temp_hi / 10.0f;
    if (smrt_nvs_get_int(SMRT_ENV_NVS_NAMESPACE, "temp_lo", &temp_lo, (int32_t)(SMRT_ENV_TEMP_ALERT_LO * 10)))
        env_alert_temp_lo = temp_lo / 10.0f;
    if (smrt_nvs_get_int(SMRT_ENV_NVS_NAMESPACE, "hum_hi", &hum_hi, (int32_t)(SMRT_ENV_HUM_ALERT_HI * 10)))
        env_alert_hum_hi = hum_hi / 10.0f;
    if (smrt_nvs_get_int(SMRT_ENV_NVS_NAMESPACE, "hum_lo", &hum_lo, (int32_t)(SMRT_ENV_HUM_ALERT_LO * 10)))
        env_alert_hum_lo = hum_lo / 10.0f;
    smrt_nvs_get_bool(SMRT_ENV_NVS_NAMESPACE, "alert_en", &alert_en, false);
    env_alerts_enabled = alert_en;

    // First read
    env_read_sensor();
    env_last_read_ms = millis();

    Serial.println("[ENV] Module initialized (DHT22 on GPIO"
                   + String(SMRT_ENV_DHT_PIN)
                   + ", interval=" + String(env_read_interval) + "ms)");
}

/**
 * @brief  Module loop — reads sensor at configured interval.
 * @return void
 */
static void env_loop(void) {
    unsigned long now = millis();
    if (now - env_last_read_ms >= env_read_interval) {
        env_last_read_ms = now;
        env_read_sensor();

        /* Alert check */
        if (env_alerts_enabled) {
            int alert = smrt_env_check_alert(env_last_temp, env_last_hum,
                                              env_alert_temp_hi, env_alert_temp_lo,
                                              env_alert_hum_hi, env_alert_hum_lo);
            if (alert != 0 && alert != env_last_alert) {
                env_last_alert = alert;
                JsonDocument evt;
                evt["alert"] = alert;
                evt["temp"]  = env_last_temp;
                evt["hum"]   = env_last_hum;
                String evt_str;
                serializeJson(evt, evt_str);
                smrt_event_publish(SMRT_EVT_ENV_ALERT, evt_str.c_str());
            } else if (alert == 0) {
                env_last_alert = 0;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// WebSocket sub-command handlers
//-----------------------------------------------------------------------------

extern AsyncWebSocket smrt_ws;

/**
 * @brief  Sends the current sensor reading as a JSON response.
 * @return void
 */
static void env_send_reading(void) {
    JsonDocument resp;
    resp["type"]        = "env_read";
    resp["temperature"] = env_last_temp;
    resp["humidity"]    = env_last_hum;
    resp["ok"]          = (bool)env_last_ok;
    resp["interval"]    = env_read_interval;

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Handles the "set_interval" sub-command.
 *         Validates the new interval, saves to NVS, and responds.
 * @param  doc  Parsed JSON document containing "value" field
 * @return void
 */
static void env_handle_set_interval(JsonDocument &doc) {
    JsonDocument resp;
    resp["type"] = "env_set_interval";

    if (doc["value"].isNull()) {
        resp["ok"]  = false;
        resp["msg"] = "Campo 'value' requerido";
    } else {
        unsigned long new_val = doc["value"].as<unsigned long>();
        if (smrt_env_validate_interval(new_val)) {
            env_read_interval = new_val;
            smrt_nvs_set_int(SMRT_ENV_NVS_NAMESPACE,
                             SMRT_ENV_NVS_KEY_INTERVAL,
                             (int)new_val);
            resp["ok"]       = true;
            resp["interval"] = new_val;
            resp["msg"]      = "Intervalo actualizado";
            Serial.println("[ENV] Read interval set to " + String(new_val) + "ms");
        } else {
            resp["ok"]  = false;
            resp["msg"] = "Intervalo fuera de rango (2000-60000 ms)";
        }
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Module WS handler — dispatches "read" and "set_interval" sub-commands.
 *         The "env_" prefix has already been stripped by the core dispatcher.
 * @param  cmd     Sub-command string (e.g. "read", "set_interval")
 * @param  doc     Pointer to parsed JSON document (void* for portability)
 * @param  client  Pointer to WebSocket client (void*, unused — we broadcast)
 * @return void
 */
static void env_ws_handler(const char *cmd, void *doc, void *client) {
    if (!cmd) {
        return;
    }

    if (strcmp(cmd, "read") == 0) {
        env_send_reading();
        return;
    }

    if (strcmp(cmd, "set_interval") == 0) {
        JsonDocument &json = *(JsonDocument *)doc;
        env_handle_set_interval(json);
        return;
    }

    if (strcmp(cmd, "set_alert") == 0) {
        JsonDocument &json = *(JsonDocument *)doc;
        float t_hi = json["temp_hi"] | env_alert_temp_hi;
        float t_lo = json["temp_lo"] | env_alert_temp_lo;
        float h_hi = json["hum_hi"]  | env_alert_hum_hi;
        float h_lo = json["hum_lo"]  | env_alert_hum_lo;
        bool  enabled = json["enabled"] | env_alerts_enabled;

        env_alert_temp_hi = t_hi;
        env_alert_temp_lo = t_lo;
        env_alert_hum_hi  = h_hi;
        env_alert_hum_lo  = h_lo;
        env_alerts_enabled = enabled;

        smrt_nvs_set_int(SMRT_ENV_NVS_NAMESPACE, "temp_hi", (int32_t)(t_hi * 10));
        smrt_nvs_set_int(SMRT_ENV_NVS_NAMESPACE, "temp_lo", (int32_t)(t_lo * 10));
        smrt_nvs_set_int(SMRT_ENV_NVS_NAMESPACE, "hum_hi",  (int32_t)(h_hi * 10));
        smrt_nvs_set_int(SMRT_ENV_NVS_NAMESPACE, "hum_lo",  (int32_t)(h_lo * 10));
        smrt_nvs_set_bool(SMRT_ENV_NVS_NAMESPACE, "alert_en", enabled);
        return;
    }

    if (strcmp(cmd, "get_alert") == 0) {
        JsonDocument resp;
        resp["type"]     = "env_alert_config";
        resp["temp_hi"]  = env_alert_temp_hi;
        resp["temp_lo"]  = env_alert_temp_lo;
        resp["hum_hi"]   = env_alert_hum_hi;
        resp["hum_lo"]   = env_alert_hum_lo;
        resp["enabled"]  = env_alerts_enabled;
        String output;
        serializeJson(resp, output);
        extern AsyncWebSocket smrt_ws;
        smrt_ws.textAll(output);
        return;
    }

    Serial.println("[ENV] Unknown sub-command: " + String(cmd));
}

/**
 * @brief  Module telemetry — fills JSON object with temperature, humidity, status.
 *         Called by smrt_module_get_telemetry_all() during periodic broadcasts.
 * @param  data  Pointer to JsonObject scoped to this module's key ("env")
 * @return void
 */
static void env_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["temperature"] = env_last_temp;
    obj["humidity"]    = env_last_hum;
    obj["ok"]          = (bool)env_last_ok;
    obj["alert_enabled"] = env_alerts_enabled;
    if (env_alerts_enabled) {
        obj["alert"] = env_last_alert;
    }
}

//=============================================================================
// Module descriptor
//=============================================================================

/**
 * @brief  The ENV module descriptor, registered in smrt_main.cpp.
 */
const smrt_module_t smrt_mod_env = {
    "env",                  /* id */
    "Environmental",        /* name */
    "0.3.0",                /* version */
    env_init,               /* init */
    env_loop,               /* loop */
    env_ws_handler,         /* ws_handler */
    env_get_telemetry       /* get_telemetry */
};

#else // UNIT_TEST — provide a minimal descriptor for test registration

/**
 * @brief  Test-only module descriptor (no hardware callbacks).
 */
const smrt_module_t smrt_mod_env = {
    "env",                  /* id */
    "Environmental",        /* name */
    "0.3.0",                /* version */
    NULL,                   /* init */
    NULL,                   /* loop */
    NULL,                   /* ws_handler */
    NULL                    /* get_telemetry */
};

#endif // UNIT_TEST
