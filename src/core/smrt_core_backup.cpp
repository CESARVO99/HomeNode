/**
 * @file    smrt_core_backup.cpp
 * @brief   Configuration backup/export and restore/import
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_backup.h"
    #include <cstring>
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a namespace string for backup
 */
int smrt_backup_validate_namespace(const char *ns) {
    if (!ns) return 0;
    int len = (int)strlen(ns);
    return (len >= 1 && len <= 15) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

//-----------------------------------------------------------------------------
// Known configuration keys per namespace
//-----------------------------------------------------------------------------
typedef struct {
    const char *ns;
    const char *key;
    bool sensitive;     /**< true = mask in export */
} smrt_backup_key_t;

static const smrt_backup_key_t backup_keys[] = {
    /* Core WiFi */
    {"smrt_cfg", "wifi_ssid", false},
    {"smrt_cfg", "wifi_pass", true},
    {"smrt_cfg", "cfg_pin",   true},
    {"smrt_cfg", "mdns_host", false},
    /* Time */
    {"time", "gmt_off", false},
    {"time", "dst_off", false},
    /* ENV */
    {"env", "interval",  false},
    {"env", "temp_hi",   false},
    {"env", "temp_lo",   false},
    {"env", "hum_hi",    false},
    {"env", "hum_lo",    false},
    {"env", "alert_en",  false},
    /* RLY */
    {"rly", "count",    false},
    {"rly", "pulse_ms", false},
    {"rly", "name_0",   false},
    {"rly", "name_1",   false},
    {"rly", "name_2",   false},
    {"rly", "name_3",   false},
    /* SEC */
    {"sec", "entry_ms", false},
    {"sec", "exit_ms",  false},
    /* PLG */
    {"plg", "interval", false},
    {"plg", "overload", false},
    /* NRG */
    {"nrg", "ch_count",   false},
    {"nrg", "interval",   false},
    {"nrg", "alert_watts", false},
    /* ACC */
    {"acc", "uid_count", false},
    {"acc", "pulse_ms",  false},
};

static const int backup_keys_count = sizeof(backup_keys) / sizeof(backup_keys[0]);

//-----------------------------------------------------------------------------
// Import confirmation state
//-----------------------------------------------------------------------------
static unsigned long smrt_backup_import_pending_ms = 0;
static bool smrt_backup_import_pending = false;

/**
 * @brief  Handles backup WebSocket commands
 */
void smrt_backup_ws_handler(const char *cmd, void *json_doc, uint32_t client_id) {
    extern AsyncWebSocket smrt_ws;
    JsonDocument &doc = *(JsonDocument *)json_doc;
    JsonDocument resp;

    if (strcmp(cmd, "cfg_export") == 0) {
        resp["type"] = "cfg_export";
        JsonObject config = resp["config"].to<JsonObject>();

        for (int i = 0; i < backup_keys_count; i++) {
            const smrt_backup_key_t *bk = &backup_keys[i];

            /* Create namespace object if not exists */
            if (!config[bk->ns].is<JsonObject>()) {
                config[bk->ns].to<JsonObject>();
            }
            JsonObject ns_obj = config[bk->ns].as<JsonObject>();

            /* Try to read as string first */
            char buf[128];
            if (smrt_nvs_get_string(bk->ns, bk->key, buf, sizeof(buf))) {
                if (bk->sensitive) {
                    ns_obj[bk->key] = "***";
                } else {
                    ns_obj[bk->key] = String(buf);
                }
            } else {
                /* Try integer */
                int32_t ival;
                if (smrt_nvs_get_int(bk->ns, bk->key, &ival, 0)) {
                    ns_obj[bk->key] = ival;
                }
            }
        }

        /* Include scheduler tasks */
        #ifdef SMRT_SCHED
        JsonArray sched = config["sched_tasks"].to<JsonArray>();
        for (int i = 0; i < SMRT_SCHED_MAX_TASKS; i++) {
            const smrt_sched_task_t *task = smrt_sched_get_task(i);
            if (task && task->enabled) {
                JsonObject t = sched.add<JsonObject>();
                t["index"]   = i;
                t["hour"]    = task->hour;
                t["minute"]  = task->minute;
                t["days"]    = task->days;
                t["action"]  = task->action;
                t["name"]    = task->name;
            }
        }
        #endif
    }
    else if (strcmp(cmd, "cfg_import") == 0) {
        /* Double confirmation: first call sets pending, second executes */
        unsigned long now = millis();
        if (!smrt_backup_import_pending ||
            (now - smrt_backup_import_pending_ms > 10000)) {
            smrt_backup_import_pending = true;
            smrt_backup_import_pending_ms = now;
            resp["backup_result"] = false;
            resp["backup_msg"]    = "Confirmacion requerida: envia cfg_import de nuevo en 10s";
        } else {
            smrt_backup_import_pending = false;

            /* Parse and apply non-sensitive keys */
            JsonObject config = doc["config"];
            if (!config) {
                resp["backup_result"] = false;
                resp["backup_msg"]    = "Error: campo 'config' no encontrado";
            } else {
                int applied = 0;
                for (int i = 0; i < backup_keys_count; i++) {
                    const smrt_backup_key_t *bk = &backup_keys[i];
                    if (bk->sensitive) continue; /* Don't import sensitive data */

                    JsonObject ns_obj = config[bk->ns];
                    if (!ns_obj) continue;

                    if (ns_obj[bk->key].is<const char *>()) {
                        smrt_nvs_set_string(bk->ns, bk->key, ns_obj[bk->key].as<const char *>());
                        applied++;
                    } else if (ns_obj[bk->key].is<int>()) {
                        smrt_nvs_set_int(bk->ns, bk->key, ns_obj[bk->key].as<int>());
                        applied++;
                    }
                }

                char msg[64];
                snprintf(msg, sizeof(msg), "Importadas %d claves. Reiniciando...", applied);
                resp["backup_result"] = true;
                resp["backup_msg"]    = msg;

                /* Schedule restart */
                String output;
                serializeJson(resp, output);
                smrt_ws.text(client_id, output);

                delay(2000);
                ESP.restart();
                return;
            }
        }
    }
    else {
        return;
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.text(client_id, output);
}

#endif // UNIT_TEST
