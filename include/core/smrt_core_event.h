/**
 * @file    smrt_core_event.h
 * @brief   Lightweight event bus for module-to-notification dispatch
 * @project HOMENODE
 * @version 0.8.0
 *
 * Modules publish events without knowing about MQTT or webhooks.
 * The event bus forwards events to all registered notification
 * channels (MQTT, webhooks) as configured.
 */

#ifndef SMRT_CORE_EVENT_H
#define SMRT_CORE_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Event type bitmask
//-----------------------------------------------------------------------------
#define SMRT_EVT_SEC_TRIGGERED      0x01    /**< Security alarm triggered */
#define SMRT_EVT_SEC_ARMED          0x02    /**< Security system armed */
#define SMRT_EVT_SEC_DISARMED       0x04    /**< Security system disarmed */
#define SMRT_EVT_ACC_AUTHORIZED     0x08    /**< NFC access authorized */
#define SMRT_EVT_ACC_DENIED         0x10    /**< NFC access denied */
#define SMRT_EVT_NRG_OVERLOAD      0x20    /**< Energy overload alert */
#define SMRT_EVT_ENV_ALERT          0x40    /**< Environmental threshold alert */
#define SMRT_EVT_RLY_CHANGED        0x80    /**< Relay state changed */
#define SMRT_EVT_ALL                0xFF    /**< All events mask */

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Checks if an event type matches a filter bitmask
 * @param  event_type  Event type (single bit)
 * @param  filter      Filter bitmask
 * @return 1 if matches, 0 otherwise
 */
int smrt_event_matches_filter(uint8_t event_type, uint8_t filter);

/**
 * @brief  Returns a human-readable name for an event type
 * @param  event_type  Event type (single bit)
 * @return Static string with event name, or "unknown"
 */
const char *smrt_event_type_name(uint8_t event_type);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/**
 * @brief  Initializes the event bus
 * @return void
 */
void smrt_event_init(void);

/**
 * @brief  Publishes an event to all registered notification channels
 * @param  event_type   Event type bitmask (single bit)
 * @param  json_payload JSON string with event details
 * @return void
 */
void smrt_event_publish(uint8_t event_type, const char *json_payload);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_EVENT_H
