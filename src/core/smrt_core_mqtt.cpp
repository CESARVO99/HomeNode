/**
 * @file    smrt_core_mqtt.cpp
 * @brief   MQTT client implementation using PubSubClient
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_mqtt.h"
    #include <cstring>
#else
    #include "smrt_core.h"
    #ifdef SMRT_MQTT
    #include <WiFi.h>
    #include <PubSubClient.h>
    #endif
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates an MQTT server hostname/IP
 */
int smrt_mqtt_validate_server(const char *server) {
    if (!server) return 0;
    int len = (int)strlen(server);
    return (len >= 1 && len < SMRT_MQTT_SERVER_MAX) ? 1 : 0;
}

/**
 * @brief  Validates an MQTT port
 */
int smrt_mqtt_validate_port(int port) {
    return (port >= 1 && port <= 65535) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only, SMRT_MQTT only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#ifdef SMRT_MQTT

//-----------------------------------------------------------------------------
// Static state
//-----------------------------------------------------------------------------
static WiFiClient smrt_mqtt_wifi;
static PubSubClient smrt_mqtt_client(smrt_mqtt_wifi);

static char smrt_mqtt_server[SMRT_MQTT_SERVER_MAX];
static int  smrt_mqtt_port = SMRT_MQTT_PORT_DEFAULT;
static char smrt_mqtt_user[SMRT_MQTT_USER_MAX];
static char smrt_mqtt_pass[SMRT_MQTT_PASS_MAX];
static char smrt_mqtt_topic_prefix[SMRT_MQTT_TOPIC_MAX];
static bool smrt_mqtt_enabled = false;

static unsigned long smrt_mqtt_last_reconnect = 0;
static unsigned long smrt_mqtt_reconnect_interval = SMRT_MQTT_RECONNECT_MIN_MS;
static unsigned long smrt_mqtt_last_publish = 0;

//-----------------------------------------------------------------------------
// MQTT callback — handles incoming commands
//-----------------------------------------------------------------------------
static void smrt_mqtt_callback(char *topic, uint8_t *payload, unsigned int len) {
    if (len > SMRT_MQTT_BUFFER_SIZE - 1) return;

    char msg[SMRT_MQTT_BUFFER_SIZE];
    memcpy(msg, payload, len);
    msg[len] = '\0';

    SMRT_DEBUG_PRINTF("MQTT recv [%s]: %s\n", topic, msg);

    /* Parse JSON and dispatch as WS command */
    JsonDocument doc;
    if (deserializeJson(doc, msg)) return;

    const char *cmd = doc["cmd"];
    if (cmd) {
        smrt_module_dispatch(cmd, (void *)&doc, (void *)0);
    }
}

//-----------------------------------------------------------------------------
// Topic builder helper
//-----------------------------------------------------------------------------
static void mqtt_build_topic(char *buf, size_t buf_len, const char *suffix) {
    snprintf(buf, buf_len, "%s%s/%s",
             smrt_mqtt_topic_prefix, smrt_wifi_get_hostname(), suffix);
}

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes MQTT client
 */
void smrt_mqtt_init(void) {
    /* Load config from NVS */
    smrt_nvs_get_string_enc(SMRT_MQTT_NVS_NAMESPACE, "server", smrt_mqtt_server, SMRT_MQTT_SERVER_MAX);
    int32_t port;
    smrt_nvs_get_int(SMRT_MQTT_NVS_NAMESPACE, "port", &port, SMRT_MQTT_PORT_DEFAULT);
    smrt_mqtt_port = port;
    smrt_nvs_get_string_enc(SMRT_MQTT_NVS_NAMESPACE, "user", smrt_mqtt_user, SMRT_MQTT_USER_MAX);
    smrt_nvs_get_string_enc(SMRT_MQTT_NVS_NAMESPACE, "pass", smrt_mqtt_pass, SMRT_MQTT_PASS_MAX);
    smrt_nvs_get_bool(SMRT_MQTT_NVS_NAMESPACE, "enabled", &smrt_mqtt_enabled, false);

    char prefix_buf[SMRT_MQTT_TOPIC_MAX];
    if (smrt_nvs_get_string(SMRT_MQTT_NVS_NAMESPACE, "topic_pfx", prefix_buf, sizeof(prefix_buf))) {
        strncpy(smrt_mqtt_topic_prefix, prefix_buf, SMRT_MQTT_TOPIC_MAX - 1);
    } else {
        strncpy(smrt_mqtt_topic_prefix, SMRT_MQTT_TOPIC_PREFIX, SMRT_MQTT_TOPIC_MAX - 1);
    }

    smrt_mqtt_client.setBufferSize(SMRT_MQTT_BUFFER_SIZE);
    smrt_mqtt_client.setKeepAlive(SMRT_MQTT_KEEPALIVE_S);
    smrt_mqtt_client.setCallback(smrt_mqtt_callback);

    if (smrt_mqtt_enabled && strlen(smrt_mqtt_server) > 0) {
        smrt_mqtt_client.setServer(smrt_mqtt_server, smrt_mqtt_port);
    }

    SMRT_DEBUG_PRINTF("MQTT initialized (enabled=%d, server=%s)\n",
                      smrt_mqtt_enabled, smrt_mqtt_server);
}

/**
 * @brief  MQTT loop — reconnect and process messages
 */
void smrt_mqtt_loop(void) {
    if (!smrt_mqtt_enabled || strlen(smrt_mqtt_server) == 0) return;

    if (!smrt_mqtt_client.connected()) {
        unsigned long now = millis();
        if (now - smrt_mqtt_last_reconnect < smrt_mqtt_reconnect_interval) return;
        smrt_mqtt_last_reconnect = now;

        SMRT_DEBUG_LOG("MQTT: Connecting...");
        bool connected = false;
        if (strlen(smrt_mqtt_user) > 0) {
            connected = smrt_mqtt_client.connect(smrt_wifi_get_hostname(),
                                                  smrt_mqtt_user, smrt_mqtt_pass);
        } else {
            connected = smrt_mqtt_client.connect(smrt_wifi_get_hostname());
        }

        if (connected) {
            SMRT_DEBUG_LOG("MQTT: Connected");
            smrt_mqtt_reconnect_interval = SMRT_MQTT_RECONNECT_MIN_MS;

            /* Subscribe to command topic */
            char topic[SMRT_MQTT_TOPIC_MAX];
            mqtt_build_topic(topic, sizeof(topic), "cmd");
            smrt_mqtt_client.subscribe(topic);
        } else {
            /* Exponential backoff */
            smrt_mqtt_reconnect_interval *= 2;
            if (smrt_mqtt_reconnect_interval > SMRT_MQTT_RECONNECT_MAX_MS) {
                smrt_mqtt_reconnect_interval = SMRT_MQTT_RECONNECT_MAX_MS;
            }
            SMRT_DEBUG_PRINTF("MQTT: Connect failed (rc=%d), retry in %lums\n",
                              smrt_mqtt_client.state(), smrt_mqtt_reconnect_interval);
        }
        return;
    }

    smrt_mqtt_client.loop();

    /* Periodic telemetry */
    unsigned long now = millis();
    if (now - smrt_mqtt_last_publish >= SMRT_MQTT_PUBLISH_INTERVAL) {
        smrt_mqtt_last_publish = now;
        /* Build telemetry JSON */
        JsonDocument doc;
        doc["rssi"]   = WiFi.RSSI();
        doc["uptime"] = millis();
        doc["ip"]     = WiFi.localIP().toString();

        JsonObject modules = doc["modules"].to<JsonObject>();
        smrt_module_get_telemetry_all((void *)&modules);

        String output;
        serializeJson(doc, output);
        smrt_mqtt_publish_telemetry(output.c_str());
    }
}

/**
 * @brief  Publishes an event to MQTT
 */
void smrt_mqtt_publish_event(uint8_t event_type, const char *payload) {
    if (!smrt_mqtt_client.connected() || !payload) return;

    const char *suffix = smrt_event_type_name(event_type);
    char topic[SMRT_MQTT_TOPIC_MAX];
    snprintf(topic, sizeof(topic), "%s%s/event/%s",
             smrt_mqtt_topic_prefix, smrt_wifi_get_hostname(), suffix);

    smrt_mqtt_client.publish(topic, payload);
}

/**
 * @brief  Publishes telemetry data
 */
void smrt_mqtt_publish_telemetry(const char *json_payload) {
    if (!smrt_mqtt_client.connected() || !json_payload) return;

    char topic[SMRT_MQTT_TOPIC_MAX];
    mqtt_build_topic(topic, sizeof(topic), "telemetry");
    smrt_mqtt_client.publish(topic, json_payload);
}

/**
 * @brief  Returns 1 if MQTT is connected
 */
int smrt_mqtt_is_connected(void) {
    return smrt_mqtt_client.connected() ? 1 : 0;
}

/**
 * @brief  Handles MQTT WebSocket commands
 */
void smrt_mqtt_ws_handler(const char *cmd, void *json_doc, uint32_t client_id) {
    extern AsyncWebSocket smrt_ws;
    JsonDocument &doc = *(JsonDocument *)json_doc;
    JsonDocument resp;

    if (strcmp(cmd, "mqtt_config") == 0) {
        const char *server = doc["server"];
        int port           = doc["port"] | SMRT_MQTT_PORT_DEFAULT;
        const char *user   = doc["user"];
        const char *pass   = doc["pass"];
        const char *prefix = doc["topic_pfx"];

        if (!server || !smrt_mqtt_validate_server(server)) {
            resp["mqtt_result"] = false;
            resp["mqtt_msg"]    = "Servidor invalido";
        } else {
            strncpy(smrt_mqtt_server, server, SMRT_MQTT_SERVER_MAX - 1);
            smrt_mqtt_port = port;
            if (user) strncpy(smrt_mqtt_user, user, SMRT_MQTT_USER_MAX - 1);
            if (pass) strncpy(smrt_mqtt_pass, pass, SMRT_MQTT_PASS_MAX - 1);
            if (prefix) strncpy(smrt_mqtt_topic_prefix, prefix, SMRT_MQTT_TOPIC_MAX - 1);

            smrt_nvs_set_string_enc(SMRT_MQTT_NVS_NAMESPACE, "server", smrt_mqtt_server);
            smrt_nvs_set_int(SMRT_MQTT_NVS_NAMESPACE, "port", smrt_mqtt_port);
            smrt_nvs_set_string_enc(SMRT_MQTT_NVS_NAMESPACE, "user", smrt_mqtt_user);
            smrt_nvs_set_string_enc(SMRT_MQTT_NVS_NAMESPACE, "pass", smrt_mqtt_pass);
            smrt_nvs_set_string(SMRT_MQTT_NVS_NAMESPACE, "topic_pfx", smrt_mqtt_topic_prefix);

            smrt_mqtt_client.setServer(smrt_mqtt_server, smrt_mqtt_port);
            smrt_mqtt_reconnect_interval = SMRT_MQTT_RECONNECT_MIN_MS;

            resp["mqtt_result"] = true;
            resp["mqtt_msg"]    = "Configuracion MQTT guardada";
        }
    }
    else if (strcmp(cmd, "mqtt_enable") == 0) {
        bool enabled = doc["enabled"] | false;
        smrt_mqtt_enabled = enabled;
        smrt_nvs_set_bool(SMRT_MQTT_NVS_NAMESPACE, "enabled", enabled);
        if (enabled && strlen(smrt_mqtt_server) > 0) {
            smrt_mqtt_client.setServer(smrt_mqtt_server, smrt_mqtt_port);
        }
        resp["mqtt_result"] = true;
        resp["mqtt_msg"]    = enabled ? "MQTT habilitado" : "MQTT deshabilitado";
    }
    else if (strcmp(cmd, "mqtt_status") == 0) {
        resp["type"]      = "mqtt_status";
        resp["enabled"]   = smrt_mqtt_enabled;
        resp["connected"] = smrt_mqtt_client.connected();
        resp["server"]    = smrt_mqtt_server;
        resp["port"]      = smrt_mqtt_port;
    }
    else {
        return;
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.text(client_id, output);
}

/**
 * @brief  Adds MQTT telemetry to JSON object
 */
void smrt_mqtt_get_telemetry(void *json_obj) {
    JsonObject &obj = *(JsonObject *)json_obj;
    obj["enabled"]   = smrt_mqtt_enabled;
    obj["connected"] = smrt_mqtt_client.connected();
}

#endif // SMRT_MQTT
#endif // UNIT_TEST
