/**
 * @file    smrt_mod_nrg.cpp
 * @brief   Energy monitoring module — multi-channel voltage, current, power
 * @project HOMENODE
 * @version 0.4.1
 *
 * Implements the smrt_module_t interface for multi-channel energy monitoring.
 * Hardware: Up to 4 channels with ZMPT101B (voltage) and ACS712 (current).
 *
 * Architecture:
 *   - Per-channel data struct holds V, I, W, VA, PF, kWh
 *   - Moving average smoothing on power readings
 *   - nrg_loop() reads all active channels at configurable interval
 *   - nrg_ws_handler() handles status/read/config sub-commands
 *
 * Testability:
 *   - Utility functions (validate, calc, moving_avg, getters/setters) always compiled
 *   - Hardware-dependent code guarded by #ifndef UNIT_TEST
 */

//=============================================================================
// Includes
//=============================================================================
#ifndef UNIT_TEST
#include "smrt_core.h"
#include "smrt_mod_nrg.h"
#else
#include "smrt_mod_nrg.h"
#include <string.h>
#include <math.h>
#endif

//=============================================================================
// Static state
//=============================================================================
static int           nrg_ch_count     = SMRT_NRG_DEFAULT_CHANNELS;
static unsigned long nrg_interval     = SMRT_NRG_INTERVAL_DEFAULT_MS;
static float         nrg_alert_w      = SMRT_NRG_ALERT_DEFAULT_W;

/** Per-channel accumulated energy */
static float         nrg_energy_wh[SMRT_NRG_MAX_CHANNELS] = {0};

#ifndef UNIT_TEST
static unsigned long nrg_last_read_ms      = 0;
static unsigned long nrg_last_energy_ms    = 0;
static unsigned long nrg_last_nvs_write_ms = 0;    /**< Throttled NVS writes */

/** Per-channel measurements */
typedef struct {
    float voltage;
    float current;
    float power;
    float apparent;
    float pf;
    float avg_buf[SMRT_NRG_AVG_WINDOW];
    int   avg_idx;
    int   avg_count;
} nrg_channel_t;

static nrg_channel_t nrg_ch[SMRT_NRG_MAX_CHANNELS];
#endif

//=============================================================================
// Testable utility functions (always compiled)
//=============================================================================

/**
 * @brief  Validates read interval.
 * @param  ms  Proposed interval
 * @return 1 valid, 0 invalid
 */
int smrt_nrg_validate_interval(unsigned long ms) {
    if (ms >= SMRT_NRG_INTERVAL_MIN_MS && ms <= SMRT_NRG_INTERVAL_MAX_MS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Validates power alert threshold.
 * @param  watts  Proposed threshold
 * @return 1 valid, 0 invalid
 */
int smrt_nrg_validate_alert(float watts) {
    if (watts >= SMRT_NRG_ALERT_MIN_W && watts <= SMRT_NRG_ALERT_MAX_W) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Validates channel count.
 * @param  count  Proposed count
 * @return 1 valid, 0 invalid
 */
int smrt_nrg_validate_channels(int count) {
    if (count >= 1 && count <= SMRT_NRG_MAX_CHANNELS) {
        return 1;
    }
    return 0;
}

/**
 * @brief  Calculates real power.
 * @param  voltage  V
 * @param  current  A
 * @return Power (W)
 */
float smrt_nrg_calc_power(float voltage, float current) {
    return voltage * current;
}

/**
 * @brief  Calculates apparent power.
 * @param  voltage  V
 * @param  current  A
 * @return Apparent power (VA)
 */
float smrt_nrg_calc_apparent_power(float voltage, float current) {
    return voltage * current;
}

/**
 * @brief  Calculates power factor.
 * @param  real_power      W
 * @param  apparent_power  VA
 * @return PF (0.0-1.0)
 */
float smrt_nrg_calc_power_factor(float real_power, float apparent_power) {
    if (apparent_power <= 0.0f) {
        return 0.0f;
    }
    float pf = real_power / apparent_power;
    if (pf > 1.0f) {
        pf = 1.0f;
    }
    if (pf < 0.0f) {
        pf = 0.0f;
    }
    return pf;
}

/**
 * @brief  Calculates energy increment.
 * @param  watts      W
 * @param  elapsed_ms ms
 * @return Wh
 */
float smrt_nrg_calc_energy(float watts, unsigned long elapsed_ms) {
    if (elapsed_ms == 0) {
        return 0.0f;
    }
    return watts * ((float)elapsed_ms / 3600000.0f);
}

/**
 * @brief  Computes simple moving average.
 * @param  buffer  Values
 * @param  size    Buffer capacity
 * @param  count   Valid entries
 * @return Average
 */
float smrt_nrg_moving_avg(const float *buffer, int size, int count) {
    if (!buffer || count <= 0 || size <= 0) {
        return 0.0f;
    }
    int n = (count < size) ? count : size;
    float sum = 0.0f;
    int i;
    for (i = 0; i < n; i++) {
        sum += buffer[i];
    }
    return sum / (float)n;
}

/**
 * @brief  Returns active channel count.
 * @return Count
 */
int smrt_nrg_get_channels(void) {
    return nrg_ch_count;
}

/**
 * @brief  Sets active channel count.
 * @param  count  New count
 * @return void
 */
void smrt_nrg_set_channels(int count) {
    nrg_ch_count = count;
}

/**
 * @brief  Returns read interval.
 * @return ms
 */
unsigned long smrt_nrg_get_interval(void) {
    return nrg_interval;
}

/**
 * @brief  Sets read interval.
 * @param  ms  Interval
 * @return void
 */
void smrt_nrg_set_interval(unsigned long ms) {
    nrg_interval = ms;
}

/**
 * @brief  Returns alert threshold.
 * @return Watts
 */
float smrt_nrg_get_alert(void) {
    return nrg_alert_w;
}

/**
 * @brief  Sets alert threshold.
 * @param  watts  Threshold
 * @return void
 */
void smrt_nrg_set_alert(float watts) {
    nrg_alert_w = watts;
}

/**
 * @brief  Returns accumulated energy for channel.
 * @param  ch  Channel index
 * @return Wh, 0 if out of range
 */
float smrt_nrg_get_energy(int ch) {
    if (ch < 0 || ch >= SMRT_NRG_MAX_CHANNELS) {
        return 0.0f;
    }
    return nrg_energy_wh[ch];
}

/**
 * @brief  Sets accumulated energy for channel.
 * @param  ch  Channel index
 * @param  wh  Energy (Wh)
 * @return void
 */
void smrt_nrg_set_energy(int ch, float wh) {
    if (ch >= 0 && ch < SMRT_NRG_MAX_CHANNELS) {
        nrg_energy_wh[ch] = wh;
    }
}

//=============================================================================
// Hardware-dependent code (ESP32 only)
//=============================================================================
#ifndef UNIT_TEST

extern AsyncWebSocket smrt_ws;

/** Pin lookup tables */
static const int nrg_v_pins[SMRT_NRG_MAX_CHANNELS] = {
    SMRT_NRG_CH0_V_PIN, SMRT_NRG_CH1_V_PIN,
    SMRT_NRG_CH0_V_PIN, SMRT_NRG_CH0_V_PIN  /* ch2,3 share ch0 voltage */
};

static const int nrg_i_pins[SMRT_NRG_MAX_CHANNELS] = {
    SMRT_NRG_CH0_I_PIN, SMRT_NRG_CH1_I_PIN,
    SMRT_NRG_CH2_I_PIN, SMRT_NRG_CH3_I_PIN
};

/**
 * @brief  Reads RMS value from ADC pin.
 * @param  pin     ADC pin
 * @param  scale   Scale factor
 * @return RMS value
 */
static float nrg_read_rms(int pin, float scale) {
    float sum_sq = 0.0f;
    int idx;
    float v_per_adc = SMRT_NRG_ADC_VREF / (float)SMRT_NRG_ADC_RESOLUTION;

    for (idx = 0; idx < SMRT_NRG_ADC_SAMPLES; idx++) {
        int raw = analogRead(pin);
        float val = (float)(raw - SMRT_NRG_ADC_MIDPOINT) * v_per_adc * scale;
        sum_sq += val * val;
    }
    return sqrtf(sum_sq / (float)SMRT_NRG_ADC_SAMPLES);
}

/**
 * @brief  Reads all active channels.
 * @return void
 */
static void nrg_read_channels(void) {
    int ch;
    for (ch = 0; ch < nrg_ch_count; ch++) {
        nrg_ch[ch].voltage = nrg_read_rms(nrg_v_pins[ch], SMRT_NRG_ZMPT_RATIO);
        nrg_ch[ch].current = nrg_read_rms(nrg_i_pins[ch],
                                            1.0f / SMRT_NRG_ACS712_SENS);
        nrg_ch[ch].power = smrt_nrg_calc_power(nrg_ch[ch].voltage,
                                                 nrg_ch[ch].current);
        nrg_ch[ch].apparent = smrt_nrg_calc_apparent_power(nrg_ch[ch].voltage,
                                                            nrg_ch[ch].current);
        nrg_ch[ch].pf = smrt_nrg_calc_power_factor(nrg_ch[ch].power,
                                                     nrg_ch[ch].apparent);

        /* Update moving average */
        nrg_ch[ch].avg_buf[nrg_ch[ch].avg_idx] = nrg_ch[ch].power;
        nrg_ch[ch].avg_idx = (nrg_ch[ch].avg_idx + 1) % SMRT_NRG_AVG_WINDOW;
        if (nrg_ch[ch].avg_count < SMRT_NRG_AVG_WINDOW) {
            nrg_ch[ch].avg_count++;
        }
    }
}

//-----------------------------------------------------------------------------
// Module callbacks
//-----------------------------------------------------------------------------

/**
 * @brief  Module init — loads NVS config.
 * @return void
 */
static void nrg_init(void) {
    memset(nrg_ch, 0, sizeof(nrg_ch));

    /* Load channel count */
    int32_t saved_ch = 0;
    smrt_nvs_get_int(SMRT_NRG_NVS_NAMESPACE, SMRT_NRG_NVS_KEY_CH_COUNT,
                     &saved_ch, SMRT_NRG_DEFAULT_CHANNELS);
    if (smrt_nrg_validate_channels((int)saved_ch)) {
        nrg_ch_count = (int)saved_ch;
    }

    /* Load interval */
    int32_t saved_intv = 0;
    smrt_nvs_get_int(SMRT_NRG_NVS_NAMESPACE, SMRT_NRG_NVS_KEY_INTERVAL,
                     &saved_intv, (int32_t)SMRT_NRG_INTERVAL_DEFAULT_MS);
    if (smrt_nrg_validate_interval((unsigned long)saved_intv)) {
        nrg_interval = (unsigned long)saved_intv;
    }

    /* Load alert threshold (stored as int: W * 100) */
    int32_t saved_alert = 0;
    smrt_nvs_get_int(SMRT_NRG_NVS_NAMESPACE, SMRT_NRG_NVS_KEY_ALERT,
                     &saved_alert, (int32_t)(SMRT_NRG_ALERT_DEFAULT_W * 100));
    float alert = (float)saved_alert / 100.0f;
    if (smrt_nrg_validate_alert(alert)) {
        nrg_alert_w = alert;
    }

    /* Load per-channel energy */
    int ch;
    for (ch = 0; ch < SMRT_NRG_MAX_CHANNELS; ch++) {
        char key[12];
        snprintf(key, sizeof(key), "%s%d", SMRT_NRG_NVS_KEY_KWH_PFX, ch);
        int32_t saved_kwh = 0;
        smrt_nvs_get_int(SMRT_NRG_NVS_NAMESPACE, key, &saved_kwh, 0);
        nrg_energy_wh[ch] = (float)saved_kwh / 100.0f;
    }

    nrg_last_read_ms   = millis();
    nrg_last_energy_ms = millis();

    Serial.println("[NRG] Module initialized (channels="
                   + String(nrg_ch_count) + ", interval="
                   + String(nrg_interval) + "ms, alert="
                   + String(nrg_alert_w, 0) + "W)");
}

/**
 * @brief  Module loop — reads channels, accumulates energy, checks alerts.
 * @return void
 */
static void nrg_loop(void) {
    unsigned long now = millis();

    if (now - nrg_last_read_ms < nrg_interval) {
        return;
    }
    nrg_last_read_ms = now;

    nrg_read_channels();

    /* Accumulate energy per channel */
    unsigned long dt = now - nrg_last_energy_ms;
    nrg_last_energy_ms = now;
    int ch;
    bool need_nvs_write = (now - nrg_last_nvs_write_ms >= SMRT_NVS_WRITE_INTERVAL_MS);
    for (ch = 0; ch < nrg_ch_count; ch++) {
        if (nrg_ch[ch].power > 0.0f) {
            nrg_energy_wh[ch] += smrt_nrg_calc_energy(nrg_ch[ch].power, dt);
        }
    }
    /* Persist at throttled interval to protect flash (every 5 min) */
    if (need_nvs_write) {
        nrg_last_nvs_write_ms = now;
        for (ch = 0; ch < nrg_ch_count; ch++) {
            char key[12];
            snprintf(key, sizeof(key), "%s%d", SMRT_NRG_NVS_KEY_KWH_PFX, ch);
            smrt_nvs_set_int(SMRT_NRG_NVS_NAMESPACE, key,
                             (int32_t)(nrg_energy_wh[ch] * 100));
        }
    }

    /* Check power alert */
    for (ch = 0; ch < nrg_ch_count; ch++) {
        if (nrg_ch[ch].power > nrg_alert_w) {
            JsonDocument resp;
            resp["type"]    = "nrg_alert";
            resp["channel"] = ch;
            resp["power"]   = nrg_ch[ch].power;
            resp["limit"]   = nrg_alert_w;
            String output;
            serializeJson(resp, output);
            smrt_ws.textAll(output);
            Serial.println("[NRG] Alert ch" + String(ch) + " P="
                           + String(nrg_ch[ch].power, 1) + "W > "
                           + String(nrg_alert_w, 0) + "W");
        }
    }
}

//-----------------------------------------------------------------------------
// WebSocket sub-command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Sends full status for all channels.
 * @return void
 */
static void nrg_send_status(void) {
    JsonDocument resp;
    resp["type"]     = "nrg_status";
    resp["channels"] = nrg_ch_count;
    resp["interval"] = nrg_interval;
    resp["alert"]    = nrg_alert_w;

    JsonArray arr = resp["ch"].to<JsonArray>();
    int ch;
    for (ch = 0; ch < nrg_ch_count; ch++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["v"]   = nrg_ch[ch].voltage;
        obj["i"]   = nrg_ch[ch].current;
        obj["w"]   = nrg_ch[ch].power;
        obj["va"]  = nrg_ch[ch].apparent;
        obj["pf"]  = nrg_ch[ch].pf;
        obj["kwh"] = nrg_energy_wh[ch];
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Module WS handler.
 * @param  cmd     Sub-command
 * @param  doc     JSON document
 * @param  client  WS client
 * @return void
 */
static void nrg_ws_handler(const char *cmd, void *doc, void *client) {
    if (!cmd) {
        return;
    }

    JsonDocument &json = *(JsonDocument *)doc;

    if (strcmp(cmd, "status") == 0) {
        nrg_send_status();
        return;
    }

    if (strcmp(cmd, "read") == 0) {
        int ch = json["channel"].as<int>();
        if (ch >= 0 && ch < nrg_ch_count) {
            JsonDocument resp;
            resp["type"]    = "nrg_read";
            resp["channel"] = ch;
            resp["v"]       = nrg_ch[ch].voltage;
            resp["i"]       = nrg_ch[ch].current;
            resp["w"]       = nrg_ch[ch].power;
            resp["va"]      = nrg_ch[ch].apparent;
            resp["pf"]      = nrg_ch[ch].pf;
            resp["kwh"]     = nrg_energy_wh[ch];
            String output;
            serializeJson(resp, output);
            smrt_ws.textAll(output);
        }
        return;
    }

    if (strcmp(cmd, "set_interval") == 0) {
        JsonDocument resp;
        resp["type"] = "nrg_set_interval";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            unsigned long val = json["value"].as<unsigned long>();
            if (smrt_nrg_validate_interval(val)) {
                nrg_interval = val;
                smrt_nvs_set_int(SMRT_NRG_NVS_NAMESPACE,
                                 SMRT_NRG_NVS_KEY_INTERVAL, (int32_t)val);
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

    if (strcmp(cmd, "set_channels") == 0) {
        JsonDocument resp;
        resp["type"] = "nrg_set_channels";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            int val = json["value"].as<int>();
            if (smrt_nrg_validate_channels(val)) {
                nrg_ch_count = val;
                smrt_nvs_set_int(SMRT_NRG_NVS_NAMESPACE,
                                 SMRT_NRG_NVS_KEY_CH_COUNT, val);
                resp["ok"]       = true;
                resp["channels"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (1-4)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "set_alert") == 0) {
        JsonDocument resp;
        resp["type"] = "nrg_set_alert";
        if (json["value"].isNull()) {
            resp["ok"]  = false;
            resp["msg"] = "Campo 'value' requerido";
        } else {
            float val = json["value"].as<float>();
            if (smrt_nrg_validate_alert(val)) {
                nrg_alert_w = val;
                smrt_nvs_set_int(SMRT_NRG_NVS_NAMESPACE,
                                 SMRT_NRG_NVS_KEY_ALERT,
                                 (int32_t)(val * 100));
                resp["ok"]    = true;
                resp["alert"] = val;
            } else {
                resp["ok"]  = false;
                resp["msg"] = "Fuera de rango (100-10000 W)";
            }
        }
        String output;
        serializeJson(resp, output);
        smrt_ws.textAll(output);
        return;
    }

    if (strcmp(cmd, "reset_energy") == 0) {
        int ch = json["channel"].as<int>();
        if (ch >= 0 && ch < SMRT_NRG_MAX_CHANNELS) {
            nrg_energy_wh[ch] = 0.0f;
            char key[12];
            snprintf(key, sizeof(key), "%s%d", SMRT_NRG_NVS_KEY_KWH_PFX, ch);
            smrt_nvs_set_int(SMRT_NRG_NVS_NAMESPACE, key, 0);
        }
        nrg_send_status();
        return;
    }

    Serial.println("[NRG] Unknown sub-command: " + String(cmd));
}

/**
 * @brief  Module telemetry.
 * @param  data  JsonObject pointer
 * @return void
 */
static void nrg_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["channels"] = nrg_ch_count;
    obj["interval"] = nrg_interval;
    obj["alert"]    = nrg_alert_w;

    JsonArray arr = obj["ch"].to<JsonArray>();
    int ch;
    for (ch = 0; ch < nrg_ch_count; ch++) {
        JsonObject cobj = arr.add<JsonObject>();
        cobj["v"]   = nrg_ch[ch].voltage;
        cobj["i"]   = nrg_ch[ch].current;
        cobj["w"]   = nrg_ch[ch].power;
        cobj["va"]  = nrg_ch[ch].apparent;
        cobj["pf"]  = nrg_ch[ch].pf;
        cobj["kwh"] = nrg_energy_wh[ch];
    }
}

//=============================================================================
// Module descriptor
//=============================================================================

const smrt_module_t smrt_mod_nrg = {
    "nrg",                  /* id */
    "Energy Monitor",       /* name */
    "0.5.0",                /* version */
    nrg_init,               /* init */
    nrg_loop,               /* loop */
    nrg_ws_handler,         /* ws_handler */
    nrg_get_telemetry       /* get_telemetry */
};

#else // UNIT_TEST

const smrt_module_t smrt_mod_nrg = {
    "nrg",                  /* id */
    "Energy Monitor",       /* name */
    "0.5.0",                /* version */
    NULL,                   /* init */
    NULL,                   /* loop */
    NULL,                   /* ws_handler */
    NULL                    /* get_telemetry */
};

#endif // UNIT_TEST
