/**
 * @file    smrt_mod_plg.cpp
 * @brief   Smart Plug module — relay + current/voltage/power monitoring
 * @project HOMENODE
 * @version 0.4.1
 *
 * Implements the smrt_module_t interface for a smart plug with energy metering.
 * Hardware: Relay (GPIO2), ACS712-30A (GPIO34), ZMPT101B (GPIO35).
 *
 * Architecture:
 *   - Static variables hold relay state and last measurements
 *   - plg_loop() reads ADC at configurable intervals, calculates RMS, power, energy
 *   - plg_ws_handler() responds to toggle/set/status/set_interval/set_overload/reset_energy
 *   - plg_get_telemetry() fills JSON for periodic broadcasts
 *
 * Testability:
 *   - Utility functions (validate, calc, getters/setters) always compiled
 *   - Hardware-dependent code guarded by #ifndef UNIT_TEST
 */

//=============================================================================
// Includes
//=============================================================================
#ifndef UNIT_TEST
#include "smrt_core.h"
#include "smrt_mod_plg.h"
#else
#include "smrt_mod_plg.h"
#include <string.h>
#include <math.h>
#endif

//=============================================================================
// Static state
//=============================================================================
static int           plg_relay_state    = 0;
static float         plg_voltage        = 0.0f;
static float         plg_current        = 0.0f;
static float         plg_power          = 0.0f;
static float         plg_energy_wh      = 0.0f;
static float         plg_overload       = SMRT_PLG_OVERLOAD_DEFAULT_A;
static unsigned long plg_interval       = SMRT_PLG_INTERVAL_DEFAULT_MS;

#ifndef UNIT_TEST
static unsigned long plg_last_read_ms      = 0;
static unsigned long plg_last_energy_ms    = 0;
static unsigned long plg_last_nvs_write_ms = 0;    /**< Throttled NVS writes */
#endif

//=============================================================================
// Testable utility functions (always compiled)
//=============================================================================

/**
 * @brief  Validates read interval within bounds.
 * @param  ms  Proposed interval
 * @return 1 valid, 0 invalid
 */
int smrt_plg_validate_interval(unsigned long ms) {
    if (ms >= SMRT_PLG_INTERVAL_MIN_MS && ms <= SMRT_PLG_INTERVAL_MAX_MS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Validates overload threshold within bounds.
 * @param  amps  Proposed threshold
 * @return 1 valid, 0 invalid
 */
int smrt_plg_validate_overload(float amps) {
    if (amps >= SMRT_PLG_OVERLOAD_MIN_A && amps <= SMRT_PLG_OVERLOAD_MAX_A) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Calculates power from voltage and current.
 * @param  voltage  RMS voltage (V)
 * @param  current  RMS current (A)
 * @return Power in watts
 */
float smrt_plg_calc_power(float voltage, float current) {
    return voltage * current;
}

/**
 * @brief  Calculates energy increment.
 * @param  watts      Power (W)
 * @param  elapsed_ms Elapsed time (ms)
 * @return Energy in watt-hours
 */
float smrt_plg_calc_energy(float watts, unsigned long elapsed_ms) {
    if (elapsed_ms == 0) {
        return 0.0f;
    }
    return watts * ((float)elapsed_ms / 3600000.0f);
}

/**
 * @brief  Calculates RMS from ADC samples.
 * @param  samples  Raw ADC values
 * @param  count    Number of samples
 * @param  offset   DC offset (midpoint)
 * @param  scale    Scale factor
 * @return RMS value
 */
float smrt_plg_calc_rms(const int *samples, int count, int offset, float scale) {
    if (!samples || count <= 0) {
        return 0.0f;
    }
    float sum_sq = 0.0f;
    int i;
    for (i = 0; i < count; i++) {
        float val = (float)(samples[i] - offset) * scale;
        sum_sq += val * val;
    }
    return sqrtf(sum_sq / (float)count);
}

/**
 * @brief  Returns relay state.
 * @return 1=ON, 0=OFF
 */
int smrt_plg_get_state(void) {
    return plg_relay_state;
}

/**
 * @brief  Sets relay state (RAM only).
 * @param  state  0=OFF, 1=ON
 * @return void
 */
void smrt_plg_set_state(int state) {
    plg_relay_state = (state != 0) ? 1 : 0;
}

/**
 * @brief  Returns last voltage reading.
 * @return Voltage (V)
 */
float smrt_plg_get_voltage(void) {
    return plg_voltage;
}

/**
 * @brief  Returns last current reading.
 * @return Current (A)
 */
float smrt_plg_get_current(void) {
    return plg_current;
}

/**
 * @brief  Returns last power calculation.
 * @return Power (W)
 */
float smrt_plg_get_power(void) {
    return plg_power;
}

/**
 * @brief  Returns accumulated energy.
 * @return Energy (Wh)
 */
float smrt_plg_get_energy(void) {
    return plg_energy_wh;
}

/**
 * @brief  Sets accumulated energy.
 * @param  wh  Energy (Wh)
 * @return void
 */
void smrt_plg_set_energy(float wh) {
    plg_energy_wh = wh;
}

/**
 * @brief  Returns overload threshold.
 * @return Threshold (A)
 */
float smrt_plg_get_overload(void) {
    return plg_overload;
}

/**
 * @brief  Sets overload threshold.
 * @param  amps  Threshold (A)
 * @return void
 */
void smrt_plg_set_overload(float amps) {
    plg_overload = amps;
}

/**
 * @brief  Returns read interval.
 * @return Interval (ms)
 */
unsigned long smrt_plg_get_interval(void) {
    return plg_interval;
}

/**
 * @brief  Sets read interval.
 * @param  ms  Interval (ms)
 * @return void
 */
void smrt_plg_set_interval(unsigned long ms) {
    plg_interval = ms;
}

//=============================================================================
// Hardware-dependent code (ESP32 only)
//=============================================================================
#ifndef UNIT_TEST

extern AsyncWebSocket smrt_ws;

/**
 * @brief  Reads ADC samples and calculates RMS current/voltage.
 * @return void
 */
static void plg_read_sensors(void) {
    int i_samples[SMRT_PLG_ADC_SAMPLES];
    int v_samples[SMRT_PLG_ADC_SAMPLES];
    int idx;

    for (idx = 0; idx < SMRT_PLG_ADC_SAMPLES; idx++) {
        i_samples[idx] = analogRead(SMRT_PLG_CURRENT_PIN);
        v_samples[idx] = analogRead(SMRT_PLG_VOLTAGE_PIN);
    }

    float v_per_adc = SMRT_PLG_ADC_VREF / (float)SMRT_PLG_ADC_RESOLUTION;

    plg_current = smrt_plg_calc_rms(i_samples, SMRT_PLG_ADC_SAMPLES,
                                     SMRT_PLG_ADC_MIDPOINT,
                                     v_per_adc / SMRT_PLG_ACS712_SENS);
    plg_voltage = smrt_plg_calc_rms(v_samples, SMRT_PLG_ADC_SAMPLES,
                                     SMRT_PLG_ADC_MIDPOINT,
                                     v_per_adc * SMRT_PLG_ZMPT_RATIO);
    plg_power = smrt_plg_calc_power(plg_voltage, plg_current);
}

/**
 * @brief  Checks overload and disables relay if exceeded.
 * @return void
 */
static void plg_check_overload(void) {
    if (plg_relay_state && plg_current > plg_overload) {
        plg_relay_state = 0;
        digitalWrite(SMRT_PLG_RELAY_PIN, LOW);
        smrt_nvs_set_bool(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_STATE, false);
        Serial.println("[PLG] OVERLOAD — relay disabled (I="
                       + String(plg_current, 2) + "A > "
                       + String(plg_overload, 2) + "A)");

        JsonDocument resp;
        resp["type"]    = "plg_overload";
        resp["current"] = plg_current;
        resp["limit"]   = plg_overload;
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
    }
}

//-----------------------------------------------------------------------------
// Module callbacks
//-----------------------------------------------------------------------------

/**
 * @brief  Module init — configures GPIO, loads NVS.
 * @return void
 */
static void plg_init(void) {
    pinMode(SMRT_PLG_RELAY_PIN, OUTPUT);
    digitalWrite(SMRT_PLG_RELAY_PIN, LOW);

    /* Load saved state */
    bool saved_state = false;
    smrt_nvs_get_bool(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_STATE,
                      &saved_state, false);
    plg_relay_state = saved_state ? 1 : 0;
    digitalWrite(SMRT_PLG_RELAY_PIN, plg_relay_state);

    /* Load interval */
    int32_t saved_intv = 0;
    smrt_nvs_get_int(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_INTERVAL,
                     &saved_intv, (int32_t)SMRT_PLG_INTERVAL_DEFAULT_MS);
    if (smrt_plg_validate_interval((unsigned long)saved_intv)) {
        plg_interval = (unsigned long)saved_intv;
    }

    /* Load overload threshold (stored as int: amps * 100) */
    int32_t saved_ol = 0;
    smrt_nvs_get_int(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_OVERLOAD,
                     &saved_ol, (int32_t)(SMRT_PLG_OVERLOAD_DEFAULT_A * 100));
    float ol = (float)saved_ol / 100.0f;
    if (smrt_plg_validate_overload(ol)) {
        plg_overload = ol;
    }

    /* Load accumulated energy (stored as int: Wh * 100) */
    int32_t saved_kwh = 0;
    smrt_nvs_get_int(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_KWH,
                     &saved_kwh, 0);
    plg_energy_wh = (float)saved_kwh / 100.0f;

    plg_last_read_ms   = millis();
    plg_last_energy_ms = millis();

    Serial.println("[PLG] Module initialized (relay="
                   + String(plg_relay_state) + ", interval="
                   + String(plg_interval) + "ms, overload="
                   + String(plg_overload, 1) + "A)");
}

/**
 * @brief  Module loop — reads sensors, accumulates energy, checks overload.
 * @return void
 */
static void plg_loop(void) {
    unsigned long now = millis();

    if (now - plg_last_read_ms < plg_interval) {
        return;
    }
    plg_last_read_ms = now;

    plg_read_sensors();

    /* Accumulate energy */
    unsigned long dt = now - plg_last_energy_ms;
    plg_last_energy_ms = now;
    if (plg_relay_state && plg_power > 0.0f) {
        plg_energy_wh += smrt_plg_calc_energy(plg_power, dt);
        /* Persist at throttled interval to protect flash (every 5 min) */
        if (now - plg_last_nvs_write_ms >= SMRT_NVS_WRITE_INTERVAL_MS) {
            plg_last_nvs_write_ms = now;
            smrt_nvs_set_int(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_KWH,
                             (int32_t)(plg_energy_wh * 100));
        }
    }

    plg_check_overload();
}

//-----------------------------------------------------------------------------
// WebSocket sub-command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Sends full plug status via WS.
 * @return void
 */
static void plg_send_status(void) {
    JsonDocument resp;
    resp["type"]     = "plg_status";
    resp["state"]    = plg_relay_state;
    resp["voltage"]  = plg_voltage;
    resp["current"]  = plg_current;
    resp["power"]    = plg_power;
    resp["energy"]   = plg_energy_wh;
    resp["overload"] = plg_overload;
    resp["interval"] = plg_interval;

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Toggles relay and persists state.
 * @return void
 */
static void plg_toggle(void) {
    plg_relay_state = !plg_relay_state;
    digitalWrite(SMRT_PLG_RELAY_PIN, plg_relay_state);
    smrt_nvs_set_bool(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_STATE,
                      (bool)plg_relay_state);
    Serial.println("[PLG] Relay " + String(plg_relay_state ? "ON" : "OFF"));
    plg_send_status();
}

/**
 * @brief  Module WS handler — dispatches plug sub-commands.
 * @param  cmd     Sub-command (prefix stripped)
 * @param  doc     Parsed JSON document
 * @param  client  WS client (unused)
 * @return void
 */
static void plg_ws_handler(const char *cmd, void *doc, void *client) {
    if (!cmd) {
        return;
    }

    JsonDocument &json = *(JsonDocument *)doc;

    if (strcmp(cmd, "toggle") == 0) {
        plg_toggle();
        return;
    }

    if (strcmp(cmd, "set") == 0) {
        if (!json["state"].isNull()) {
            plg_relay_state = json["state"].as<int>() ? 1 : 0;
            digitalWrite(SMRT_PLG_RELAY_PIN, plg_relay_state);
            smrt_nvs_set_bool(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_STATE,
                              (bool)plg_relay_state);
        }
        plg_send_status();
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        plg_send_status();
        return;
    }

    if (strcmp(cmd, "set_interval") == 0) {
        JsonDocument resp;
        resp["type"] = "plg_set_interval";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            unsigned long val = json["value"].as<unsigned long>();
            if (smrt_plg_validate_interval(val)) {
                plg_interval = val;
                smrt_nvs_set_int(SMRT_PLG_NVS_NAMESPACE,
                                 SMRT_PLG_NVS_KEY_INTERVAL, (int32_t)val);
                resp["ok"]       = true;
                resp["interval"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (1000-60000 ms)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "set_overload") == 0) {
        JsonDocument resp;
        resp["type"] = "plg_set_overload";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            float val = json["value"].as<float>();
            if (smrt_plg_validate_overload(val)) {
                plg_overload = val;
                smrt_nvs_set_int(SMRT_PLG_NVS_NAMESPACE,
                                 SMRT_PLG_NVS_KEY_OVERLOAD,
                                 (int32_t)(val * 100));
                resp["ok"]       = true;
                resp["overload"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (1.0-30.0 A)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "reset_energy") == 0) {
        plg_energy_wh = 0.0f;
        smrt_nvs_set_int(SMRT_PLG_NVS_NAMESPACE, SMRT_PLG_NVS_KEY_KWH, 0);
        plg_send_status();
        return;
    }

    Serial.println("[PLG] Unknown sub-command: " + String(cmd));
}

/**
 * @brief  Module telemetry — fills JSON with plug state and readings.
 * @param  data  Pointer to JsonObject
 * @return void
 */
static void plg_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["state"]    = plg_relay_state;
    obj["voltage"]  = plg_voltage;
    obj["current"]  = plg_current;
    obj["power"]    = plg_power;
    obj["energy"]   = plg_energy_wh;
    obj["overload"] = plg_overload;
    obj["interval"] = plg_interval;
}

//=============================================================================
// Module descriptor
//=============================================================================

const smrt_module_t smrt_mod_plg = {
    "plg",                  /* id */
    "Smart Plug",           /* name */
    "0.4.0",                /* version */
    plg_init,               /* init */
    plg_loop,               /* loop */
    plg_ws_handler,         /* ws_handler */
    plg_get_telemetry       /* get_telemetry */
};

#else // UNIT_TEST

const smrt_module_t smrt_mod_plg = {
    "plg",                  /* id */
    "Smart Plug",           /* name */
    "0.4.0",                /* version */
    NULL,                   /* init */
    NULL,                   /* loop */
    NULL,                   /* ws_handler */
    NULL                    /* get_telemetry */
};

#endif // UNIT_TEST
