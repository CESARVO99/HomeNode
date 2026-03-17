/**
 * @file    smrt_core_http.cpp
 * @brief   HTTP server setup — global AsyncWebServer/AsyncWebSocket and route registration
 * @project HOMENODE
 * @version 0.8.0
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

    // OTA services
    smrt_ota_init();
    smrt_ota_web_init();

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
