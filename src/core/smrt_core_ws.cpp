/**
 * @file    smrt_core_ws.cpp
 * @brief   WebSocket server with module dispatch and telemetry aggregation
 * @project HOMENODE
 * @version 0.2.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// External global objects (defined in smrt_core_http.cpp)
//-----------------------------------------------------------------------------
extern AsyncWebServer smrt_server;
extern AsyncWebSocket smrt_ws;

//-----------------------------------------------------------------------------
// WiFi config response helper
//-----------------------------------------------------------------------------

/**
 * @brief  Sends a WiFi configuration response to all WebSocket clients.
 * @param  success  true if operation succeeded, false on error
 * @param  message  Human-readable result message
 * @return void
 */
static void smrt_ws_send_wifi_response(bool success, const char *message) {
    JsonDocument resp;
    resp["wifi_result"] = success;
    resp["wifi_msg"]    = message;
    String respStr;
    serializeJson(resp, respStr);
    smrt_ws.textAll(respStr);
}

//-----------------------------------------------------------------------------
// Core command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Handles the "wifi" command — validates PIN, saves credentials, restarts.
 * @param  doc  Parsed JSON document containing wifi command fields
 * @return void
 */
static void smrt_ws_handle_wifi_cmd(JsonDocument &doc) {
    const char *pin  = doc["pin"];
    const char *ssid = doc["ssid"];
    const char *pass = doc["pass"];

    // Validate PIN
    if (!pin || strcmp(pin, smrt_wifi_get_pin()) != 0) {
        smrt_ws_send_wifi_response(false, "PIN incorrecto");
        Serial.println("WiFi config: Invalid PIN attempt");
        return;
    }

    // Validate SSID
    if (!ssid || strlen(ssid) == 0) {
        smrt_ws_send_wifi_response(false, "SSID no puede estar vacio");
        return;
    }

    // Save credentials
    smrt_wifi_save_credentials(ssid, pass ? pass : "");
    Serial.println("WiFi config: Credentials saved for: " + String(ssid));

    // Check if a new PIN was provided
    const char *new_pin = doc["new_pin"];
    if (new_pin && strlen(new_pin) >= 4) {
        smrt_wifi_save_pin(new_pin);
        smrt_wifi_set_pin(new_pin);
        Serial.println("WiFi config: PIN updated");
    }

    smrt_ws_send_wifi_response(true, "Credenciales guardadas. Reiniciando en 3s...");

    delay(SMRT_RELAY_PULSE_MS);
    ESP.restart();
}

/**
 * @brief  Dispatches a parsed JSON command to core handlers or modules.
 *
 * Core commands: "status", "wifi"
 * Module commands: prefix_subcommand (e.g. "env_read") -> module dispatch
 *
 * @param  doc  Parsed JSON document with a "cmd" field
 * @return void
 */
static void smrt_ws_dispatch_command(JsonDocument &doc) {
    const char *cmd = doc["cmd"];
    if (!cmd) {
        return;
    }

    // Core commands
    if (strcmp(cmd, "status") == 0) {
        smrt_ws_send_status();
        return;
    }
    if (strcmp(cmd, "wifi") == 0) {
        smrt_ws_handle_wifi_cmd(doc);
        return;
    }

    // Module dispatch (prefix stripping)
    smrt_module_dispatch(cmd, (void *)&doc, (void *)0);
}

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Broadcasts system status JSON to all connected WebSocket clients.
 *         Core fields: rssi, uptime, ip, clients, ssid, modules.
 *         Module telemetry is aggregated under a "modules" object.
 * @return void
 */
void smrt_ws_send_status(void) {
    JsonDocument doc;

    // Core telemetry
    doc["rssi"]    = WiFi.RSSI();
    doc["uptime"]  = millis();
    doc["ip"]      = WiFi.localIP().toString();
    doc["clients"] = smrt_ws.count();
    doc["ssid"]    = String(smrt_wifi_get_ssid());

    // Module telemetry
    JsonObject modules = doc["modules"].to<JsonObject>();
    smrt_module_get_telemetry_all((void *)&modules);

    String output;
    serializeJson(doc, output);
    smrt_ws.textAll(output);
}

/**
 * @brief  Handles incoming WebSocket messages.
 *         Validates frame completeness, parses JSON and dispatches commands.
 * @param  arg   Frame info pointer (AwsFrameInfo)
 * @param  data  Pointer to the received data buffer
 * @param  len   Length of the received data
 * @return void
 */
void smrt_ws_handle_message(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;

    if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) {
        return;
    }

    data[len] = 0;
    String msg = String((char *)data);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error) {
        return;
    }

    smrt_ws_dispatch_command(doc);
}

/**
 * @brief  WebSocket event handler for connection/disconnection/data events.
 *         Sends initial status to newly connected clients.
 * @param  server  Pointer to the AsyncWebSocket server instance
 * @param  client  Pointer to the client that triggered the event
 * @param  type    Type of event
 * @param  arg     Event-specific argument
 * @param  data    Pointer to event data
 * @param  len     Length of event data
 * @return void
 */
void smrt_ws_on_event(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n",
                          client->id(), client->remoteIP().toString().c_str());
            smrt_ws_send_status();
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            smrt_ws_handle_message(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

/**
 * @brief  Initializes the WebSocket server and registers the event handler
 * @return void
 */
void smrt_ws_init(void) {
    smrt_ws.onEvent(smrt_ws_on_event);
    smrt_server.addHandler(&smrt_ws);
}

/**
 * @brief  Processes HTML template variables (placeholder for future use)
 * @param  var  Template variable name to process
 * @return Empty string (all data sent via WebSocket JSON)
 */
String smrt_ws_processor(const String& var) {
    return String();
}

#endif // UNIT_TEST
