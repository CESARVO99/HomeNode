/**
 * @file    smrt_core_http.cpp
 * @brief   HTTP server setup — global AsyncWebServer/AsyncWebSocket and route registration
 * @project HOMENODE
 * @version 0.2.0
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

    // Start
    smrt_server.begin();
    Serial.println("HTTP server started on port " + String(SMRT_WEB_SERVER_PORT));
    Serial.println("OTA ready: ArduinoOTA + HTTP /update");
}

#endif // UNIT_TEST
