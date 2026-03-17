/**
 * @file    smrt_core_webhook.h
 * @brief   HTTP webhook notifications for module events
 * @project HOMENODE
 * @version 0.8.0
 */

#ifndef SMRT_CORE_WEBHOOK_H
#define SMRT_CORE_WEBHOOK_H

#include "smrt_core_webhook_config.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a webhook URL (must start with http:// or https://)
 * @param  url  URL string
 * @return 1 if valid, 0 otherwise
 */
int smrt_webhook_validate_url(const char *url);

/**
 * @brief  Validates a webhook index
 * @param  index  Webhook index (0 to SMRT_WEBHOOK_MAX_HOOKS-1)
 * @return 1 if valid, 0 otherwise
 */
int smrt_webhook_validate_index(int index);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/** @brief Initializes webhooks — loads config from NVS */
void smrt_webhook_init(void);

/** @brief Sends an event to matching webhooks */
void smrt_webhook_send_event(uint8_t event_type, const char *payload);

/** @brief Handles webhook WebSocket commands */
void smrt_webhook_ws_handler(const char *cmd, void *json_doc, uint32_t client_id);

/** @brief Adds webhook telemetry to JSON object */
void smrt_webhook_get_telemetry(void *json_obj);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_WEBHOOK_H
