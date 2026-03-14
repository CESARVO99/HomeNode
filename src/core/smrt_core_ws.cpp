/**
 * @file    smrt_core_ws.cpp
 * @brief   WebSocket server with authentication, rate limiting, module dispatch
 * @project HOMENODE
 * @version 0.4.1
 *
 * Security features (v0.4.1):
 *   - WebSocket authentication via "auth" command (PIN-based)
 *   - Read-only commands ("status") allowed without auth
 *   - Write commands require authenticated session
 *   - PIN rate limiting with lockout after N failed attempts
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
// Active client tracking (for per-client message routing)
//-----------------------------------------------------------------------------
static uint32_t smrt_ws_active_client_id = 0;  /**< Client ID for current message */

//-----------------------------------------------------------------------------
// Auth response helpers
//-----------------------------------------------------------------------------

/**
 * @brief  Sends an auth response JSON to a specific client.
 * @param  client_id  Target client ID
 * @param  success    true if authenticated, false on error
 * @param  message    Human-readable message
 * @return void
 */
static void smrt_ws_send_auth_response(uint32_t client_id, bool success, const char *message) {
    JsonDocument resp;
    resp["auth_result"] = success;
    resp["auth_msg"]    = message;
    String respStr;
    serializeJson(resp, respStr);
    smrt_ws.text(client_id, respStr);
}

/**
 * @brief  Sends an error JSON to a specific client.
 * @param  client_id  Target client ID
 * @param  message    Error message
 * @return void
 */
static void smrt_ws_send_error(uint32_t client_id, const char *message) {
    JsonDocument resp;
    resp["error"] = message;
    String respStr;
    serializeJson(resp, respStr);
    smrt_ws.text(client_id, respStr);
}

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
// Auth command handler
//-----------------------------------------------------------------------------

/**
 * @brief  Handles the "auth" command — validates PIN with rate limiting.
 * @param  doc        Parsed JSON document with "pin" field
 * @param  client_id  WebSocket client ID requesting auth
 * @return void
 */
static void smrt_ws_handle_auth_cmd(JsonDocument &doc, uint32_t client_id) {
    /* Check lockout first */
    if (smrt_auth_pin_is_locked()) {
        unsigned long remaining = smrt_auth_pin_lockout_remaining();
        char msg[64];
        snprintf(msg, sizeof(msg), "Bloqueado. Reintenta en %lus", remaining);
        smrt_ws_send_auth_response(client_id, false, msg);
        return;
    }

    const char *pin = doc["pin"];
    if (!pin || strcmp(pin, smrt_wifi_get_pin()) != 0) {
        smrt_auth_pin_fail();
        if (smrt_auth_pin_is_locked()) {
            smrt_ws_send_auth_response(client_id, false,
                "PIN incorrecto. Demasiados intentos, bloqueado 60s");
        } else {
            smrt_ws_send_auth_response(client_id, false, "PIN incorrecto");
        }
        Serial.printf("AUTH: Failed PIN attempt from client #%u\n", client_id);
        return;
    }

    /* PIN correct — authenticate client */
    smrt_auth_pin_reset();
    smrt_auth_ws_login(client_id);
    smrt_ws_send_auth_response(client_id, true, "Autenticado correctamente");
    Serial.printf("AUTH: Client #%u authenticated\n", client_id);
}

//-----------------------------------------------------------------------------
// Core command handlers
//-----------------------------------------------------------------------------

/**
 * @brief  Handles the "wifi" command — validates PIN, saves credentials, restarts.
 *         Now includes rate limiting via smrt_auth_pin_* functions.
 * @param  doc  Parsed JSON document containing wifi command fields
 * @return void
 */
static void smrt_ws_handle_wifi_cmd(JsonDocument &doc) {
    /* Check lockout */
    if (smrt_auth_pin_is_locked()) {
        unsigned long remaining = smrt_auth_pin_lockout_remaining();
        char msg[64];
        snprintf(msg, sizeof(msg), "Bloqueado. Reintenta en %lus", remaining);
        smrt_ws_send_wifi_response(false, msg);
        return;
    }

    const char *pin  = doc["pin"];
    const char *ssid = doc["ssid"];
    const char *pass = doc["pass"];

    // Validate PIN with rate limiting
    if (!pin || strcmp(pin, smrt_wifi_get_pin()) != 0) {
        smrt_auth_pin_fail();
        if (smrt_auth_pin_is_locked()) {
            smrt_ws_send_wifi_response(false,
                "PIN incorrecto. Demasiados intentos, bloqueado 60s");
        } else {
            smrt_ws_send_wifi_response(false, "PIN incorrecto");
        }
        Serial.println("WiFi config: Invalid PIN attempt");
        return;
    }

    smrt_auth_pin_reset();

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
 * Security policy:
 *   - "status" is read-only and allowed without authentication
 *   - "auth" always allowed (it IS the authentication mechanism)
 *   - "wifi" has its own PIN validation (rate-limited)
 *   - All other commands (module commands) require authenticated session
 *
 * @param  doc        Parsed JSON document with a "cmd" field
 * @param  client_id  WebSocket client ID that sent the message
 * @return void
 */
static void smrt_ws_dispatch_command(JsonDocument &doc, uint32_t client_id) {
    const char *cmd = doc["cmd"];
    if (!cmd) {
        return;
    }

    /* Always allowed: status (read-only telemetry) */
    if (strcmp(cmd, "status") == 0) {
        smrt_ws_send_status();
        return;
    }

    /* Always allowed: auth (this IS the login mechanism) */
    if (strcmp(cmd, "auth") == 0) {
        smrt_ws_handle_auth_cmd(doc, client_id);
        return;
    }

    /* WiFi has its own PIN check with rate limiting */
    if (strcmp(cmd, "wifi") == 0) {
        smrt_ws_handle_wifi_cmd(doc);
        return;
    }

    /* All other commands require authentication */
    if (!smrt_auth_ws_is_authenticated(client_id)) {
        smrt_ws_send_error(client_id,
            "No autenticado. Envia {\"cmd\":\"auth\",\"pin\":\"XXXX\"} primero");
        return;
    }

    /* Module dispatch (prefix stripping) */
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
 * @param  arg        Frame info pointer (AwsFrameInfo)
 * @param  data       Pointer to the received data buffer
 * @param  len        Length of the received data
 * @param  client_id  WebSocket client ID that sent the message
 * @return void
 */
static void smrt_ws_handle_message_from(void *arg, uint8_t *data, size_t len,
                                         uint32_t client_id) {
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

    smrt_ws_dispatch_command(doc, client_id);
}

/**
 * @brief  Legacy wrapper for handle_message (maintains header compatibility).
 * @param  arg   Frame info pointer
 * @param  data  Data buffer
 * @param  len   Data length
 * @return void
 */
void smrt_ws_handle_message(void *arg, uint8_t *data, size_t len) {
    smrt_ws_handle_message_from(arg, data, len, smrt_ws_active_client_id);
}

/**
 * @brief  WebSocket event handler for connection/disconnection/data events.
 *         Sends initial status to newly connected clients.
 *         Tracks/unracks authenticated clients on connect/disconnect.
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
            smrt_auth_ws_logout(client->id());
            break;
        case WS_EVT_DATA:
            smrt_ws_active_client_id = client->id();
            smrt_ws_handle_message_from(arg, data, len, client->id());
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
    smrt_auth_init();
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
