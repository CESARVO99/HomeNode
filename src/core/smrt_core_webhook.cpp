/**
 * @file    smrt_core_webhook.cpp
 * @brief   HTTP webhook notifications implementation
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_webhook.h"
    #include <cstring>
#else
    #include "smrt_core.h"
    #ifdef SMRT_WEBHOOK
    #include <HTTPClient.h>
    #endif
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a webhook URL
 */
int smrt_webhook_validate_url(const char *url) {
    if (!url) return 0;
    int len = (int)strlen(url);
    if (len < 10 || len >= SMRT_WEBHOOK_URL_MAX) return 0;
    if (strncmp(url, "http://", 7) == 0) return 1;
    if (strncmp(url, "https://", 8) == 0) return 1;
    return 0;
}

/**
 * @brief  Validates a webhook index
 */
int smrt_webhook_validate_index(int index) {
    return (index >= 0 && index < SMRT_WEBHOOK_MAX_HOOKS) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only, SMRT_WEBHOOK only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#ifdef SMRT_WEBHOOK

//-----------------------------------------------------------------------------
// Static state
//-----------------------------------------------------------------------------
typedef struct {
    char    url[SMRT_WEBHOOK_URL_MAX];
    uint8_t events;     /**< Event filter bitmask */
    bool    active;
} smrt_webhook_t;

static smrt_webhook_t smrt_webhooks[SMRT_WEBHOOK_MAX_HOOKS];

//-----------------------------------------------------------------------------
// NVS helpers
//-----------------------------------------------------------------------------
static void webhook_load(int index) {
    char url_key[8], evt_key[8];
    snprintf(url_key, sizeof(url_key), "url_%d", index);
    snprintf(evt_key, sizeof(evt_key), "evt_%d", index);

    if (smrt_nvs_get_string(SMRT_WEBHOOK_NVS_NAMESPACE, url_key,
                             smrt_webhooks[index].url, SMRT_WEBHOOK_URL_MAX)) {
        int32_t events;
        smrt_nvs_get_int(SMRT_WEBHOOK_NVS_NAMESPACE, evt_key, &events, SMRT_EVT_ALL);
        smrt_webhooks[index].events = (uint8_t)events;
        smrt_webhooks[index].active = true;
    } else {
        memset(&smrt_webhooks[index], 0, sizeof(smrt_webhook_t));
    }
}

static void webhook_save(int index) {
    char url_key[8], evt_key[8];
    snprintf(url_key, sizeof(url_key), "url_%d", index);
    snprintf(evt_key, sizeof(evt_key), "evt_%d", index);

    if (smrt_webhooks[index].active) {
        smrt_nvs_set_string(SMRT_WEBHOOK_NVS_NAMESPACE, url_key, smrt_webhooks[index].url);
        smrt_nvs_set_int(SMRT_WEBHOOK_NVS_NAMESPACE, evt_key, smrt_webhooks[index].events);
    } else {
        smrt_nvs_remove(SMRT_WEBHOOK_NVS_NAMESPACE, url_key);
        smrt_nvs_remove(SMRT_WEBHOOK_NVS_NAMESPACE, evt_key);
    }
}

//-----------------------------------------------------------------------------
// HTTP POST helper
//-----------------------------------------------------------------------------
static bool webhook_send_http(const char *url, const char *json_payload) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(SMRT_WEBHOOK_TIMEOUT_MS);

    int code = http.POST(json_payload);
    http.end();

    return (code >= 200 && code < 300);
}

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes webhooks — loads config from NVS
 */
void smrt_webhook_init(void) {
    for (int i = 0; i < SMRT_WEBHOOK_MAX_HOOKS; i++) {
        webhook_load(i);
    }
    SMRT_DEBUG_LOG("Webhook subsystem initialized");
}

/**
 * @brief  Sends an event to matching webhooks
 */
void smrt_webhook_send_event(uint8_t event_type, const char *payload) {
    if (!payload) return;

    /* Build full event JSON with type info */
    JsonDocument doc;
    doc["event"]    = smrt_event_type_name(event_type);
    doc["hostname"] = smrt_wifi_get_hostname();
    doc["uptime"]   = millis();

    /* Try to parse payload and merge */
    JsonDocument payload_doc;
    if (!deserializeJson(payload_doc, payload)) {
        JsonObject data = doc["data"].to<JsonObject>();
        for (JsonPair kv : payload_doc.as<JsonObject>()) {
            data[kv.key()] = kv.value();
        }
    }

    String output;
    serializeJson(doc, output);

    for (int i = 0; i < SMRT_WEBHOOK_MAX_HOOKS; i++) {
        if (!smrt_webhooks[i].active) continue;
        if (!smrt_event_matches_filter(event_type, smrt_webhooks[i].events)) continue;

        bool sent = false;
        for (int retry = 0; retry <= SMRT_WEBHOOK_RETRY_COUNT && !sent; retry++) {
            sent = webhook_send_http(smrt_webhooks[i].url, output.c_str());
            if (!sent && retry < SMRT_WEBHOOK_RETRY_COUNT) {
                delay(500);
            }
        }

        if (!sent) {
            SMRT_DEBUG_PRINTF("WEBHOOK: Failed to send to %s\n", smrt_webhooks[i].url);
        }
    }
}

/**
 * @brief  Handles webhook WebSocket commands
 */
void smrt_webhook_ws_handler(const char *cmd, void *json_doc, uint32_t client_id) {
    extern AsyncWebSocket smrt_ws;
    JsonDocument &doc = *(JsonDocument *)json_doc;
    JsonDocument resp;

    if (strcmp(cmd, "webhook_set") == 0) {
        int index       = doc["index"] | -1;
        const char *url = doc["url"];
        int events      = doc["events"] | SMRT_EVT_ALL;

        if (!smrt_webhook_validate_index(index) || !url || !smrt_webhook_validate_url(url)) {
            resp["webhook_result"] = false;
            resp["webhook_msg"]    = "Datos invalidos";
        } else {
            strncpy(smrt_webhooks[index].url, url, SMRT_WEBHOOK_URL_MAX - 1);
            smrt_webhooks[index].events = (uint8_t)events;
            smrt_webhooks[index].active = true;
            webhook_save(index);
            resp["webhook_result"] = true;
            resp["webhook_msg"]    = "Webhook guardado";
        }
    }
    else if (strcmp(cmd, "webhook_delete") == 0) {
        int index = doc["index"] | -1;
        if (smrt_webhook_validate_index(index)) {
            memset(&smrt_webhooks[index], 0, sizeof(smrt_webhook_t));
            webhook_save(index);
            resp["webhook_result"] = true;
            resp["webhook_msg"]    = "Webhook eliminado";
        } else {
            resp["webhook_result"] = false;
            resp["webhook_msg"]    = "Indice invalido";
        }
    }
    else if (strcmp(cmd, "webhook_list") == 0) {
        resp["type"] = "webhook_list";
        JsonArray hooks = resp["webhooks"].to<JsonArray>();
        for (int i = 0; i < SMRT_WEBHOOK_MAX_HOOKS; i++) {
            JsonObject h = hooks.add<JsonObject>();
            h["index"]  = i;
            h["active"] = smrt_webhooks[i].active;
            h["url"]    = smrt_webhooks[i].url;
            h["events"] = smrt_webhooks[i].events;
        }
    }
    else if (strcmp(cmd, "webhook_test") == 0) {
        int index = doc["index"] | -1;
        if (smrt_webhook_validate_index(index) && smrt_webhooks[index].active) {
            bool ok = webhook_send_http(smrt_webhooks[index].url,
                                         "{\"event\":\"test\",\"msg\":\"HomeNode webhook test\"}");
            resp["webhook_result"] = ok;
            resp["webhook_msg"]    = ok ? "Test enviado OK" : "Error al enviar test";
        } else {
            resp["webhook_result"] = false;
            resp["webhook_msg"]    = "Webhook no activo";
        }
    }
    else {
        return;
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.text(client_id, output);
}

/**
 * @brief  Adds webhook telemetry to JSON object
 */
void smrt_webhook_get_telemetry(void *json_obj) {
    JsonObject &obj = *(JsonObject *)json_obj;
    int count = 0;
    for (int i = 0; i < SMRT_WEBHOOK_MAX_HOOKS; i++) {
        if (smrt_webhooks[i].active) count++;
    }
    obj["active_hooks"] = count;
}

#endif // SMRT_WEBHOOK
#endif // UNIT_TEST
