/**
 * @file    smrt_core_http.cpp
 * @brief   HTTP server setup — global AsyncWebServer/AsyncWebSocket and route registration
 * @project HOMENODE
 * @version 1.0.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// Global server instances (extern'd by other core modules)
//-----------------------------------------------------------------------------
AsyncWebServer smrt_server(SMRT_WEB_SERVER_PORT);
AsyncWebSocket smrt_ws(SMRT_WS_PATH);

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes the HTTP server.
 *         - Registers WebSocket handler
 *         - Serves main page at "/"
 *         - Registers OTA upload endpoints
 *         - Starts the server
 * @return void
 */
void smrt_http_init(void) {
    // WebSocket
    smrt_ws_init();

    // Main page
    smrt_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", smrt_webui_html, smrt_ws_processor);
    });

    // REST API — Node identity
    smrt_server.on("/api/v1/node", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["node_id"]  = smrt_node_get_id();
        doc["name"]     = smrt_node_get_name();
        doc["room"]     = smrt_node_get_room();
        doc["version"]  = SMRT_PLATFORM_VERSION;
        doc["uptime"]   = millis();
        doc["modules"]  = smrt_node_get_modules();

        char mod_str[32];
        smrt_node_modules_to_string(smrt_node_get_modules(), mod_str, sizeof(mod_str));
        doc["modules_str"] = mod_str;
        doc["rssi"]     = WiFi.RSSI();
        doc["ip"]       = WiFi.localIP().toString();

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // REST API — Current telemetry snapshot
    smrt_server.on("/api/v1/telemetry", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["node_id"] = smrt_node_get_id();
        doc["rssi"]    = WiFi.RSSI();
        doc["uptime"]  = millis();

        JsonObject modules = doc["modules"].to<JsonObject>();
        smrt_module_get_telemetry_all((void *)&modules);

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // REST API — Active modules list
    smrt_server.on("/api/v1/modules", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonArray arr = doc["modules"].to<JsonArray>();
        uint8_t mask = smrt_node_get_modules();
        const char *names[] = {"env", "rly", "sec", "plg", "nrg", "acc"};
        for (int i = 0; i < 6; i++) {
            if (mask & (1 << i)) arr.add(names[i]);
        }
        doc["count"] = smrt_node_module_count(mask);

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // OTA services
    smrt_ota_init();
    smrt_ota_web_init();

    // Node identity (must init early — other services use node_id)
    smrt_node_init();

    // Crypto (must init before services that use encrypted NVS)
    #ifdef SMRT_CRYPTO
    smrt_crypto_init();
    #endif

    // NTP time sync (must init after WiFi)
    smrt_time_init();

    // Event bus
    smrt_event_init();

    // Scheduler
    #ifdef SMRT_SCHED
    smrt_sched_init();
    #endif

    // MQTT
    #ifdef SMRT_MQTT
    smrt_mqtt_init();
    #endif

    // Webhooks
    #ifdef SMRT_WEBHOOK
    smrt_webhook_init();
    #endif

    // Start
    smrt_server.begin();
    Serial.println("HTTP server started on port " + String(SMRT_WEB_SERVER_PORT));
    Serial.println("OTA ready: ArduinoOTA + HTTP /update");
}

#endif // UNIT_TEST
