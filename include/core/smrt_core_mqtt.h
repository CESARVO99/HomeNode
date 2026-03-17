/**
 * @file    smrt_core_mqtt.h
 * @brief   MQTT client for telemetry publishing and remote commands
 * @project HOMENODE
 * @version 0.8.0
 */

#ifndef SMRT_CORE_MQTT_H
#define SMRT_CORE_MQTT_H

#include "smrt_core_mqtt_config.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates an MQTT server hostname/IP
 * @param  server  Hostname string
 * @return 1 if valid, 0 otherwise
 */
int smrt_mqtt_validate_server(const char *server);

/**
 * @brief  Validates an MQTT port
 * @param  port  Port number
 * @return 1 if valid (1-65535), 0 otherwise
 */
int smrt_mqtt_validate_port(int port);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/** @brief Initializes MQTT client — loads config from NVS */
void smrt_mqtt_init(void);

/** @brief MQTT loop — handles reconnect and message processing */
void smrt_mqtt_loop(void);

/** @brief Publishes an event to the appropriate MQTT topic */
void smrt_mqtt_publish_event(uint8_t event_type, const char *payload);

/** @brief Publishes telemetry data */
void smrt_mqtt_publish_telemetry(const char *json_payload);

/** @brief Handles MQTT WebSocket commands */
void smrt_mqtt_ws_handler(const char *cmd, void *json_doc, uint32_t client_id);

/** @brief Adds MQTT telemetry to JSON object */
void smrt_mqtt_get_telemetry(void *json_obj);

/** @brief Returns 1 if MQTT is connected */
int smrt_mqtt_is_connected(void);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_MQTT_H
