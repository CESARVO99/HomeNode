/**
 * @file    smrt_core_ws.h
 * @brief   WebSocket server — init, event handling, dispatch and telemetry
 * @project HOMENODE
 * @version 0.2.0
 */

#ifndef SMRT_CORE_WS_H
#define SMRT_CORE_WS_H

#ifndef UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes the WebSocket server and registers the event handler
 * @return void
 */
void smrt_ws_init(void);

/**
 * @brief  WebSocket event handler for connection/disconnection/data events
 * @param  server  Pointer to the AsyncWebSocket server instance
 * @param  client  Pointer to the client that triggered the event
 * @param  type    Type of event
 * @param  arg     Event-specific argument
 * @param  data    Pointer to event data
 * @param  len     Length of event data
 * @return void
 */
void smrt_ws_on_event(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len);

/**
 * @brief  Handles incoming WebSocket messages (JSON commands).
 *         Routes core commands (status, wifi) and delegates module
 *         commands via smrt_module_dispatch().
 * @param  arg   Frame info pointer (AwsFrameInfo)
 * @param  data  Pointer to the received data buffer
 * @param  len   Length of the received data
 * @return void
 */
void smrt_ws_handle_message(void *arg, uint8_t *data, size_t len);

/**
 * @brief  Broadcasts system status JSON to all connected WebSocket clients.
 *         Includes core telemetry + aggregated module telemetry.
 * @return void
 */
void smrt_ws_send_status(void);

/**
 * @brief  Processes HTML template variables (replaces placeholders)
 * @param  var  Template variable name to process
 * @return String containing the replacement value
 */
String smrt_ws_processor(const String& var);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST

#endif // SMRT_CORE_WS_H
