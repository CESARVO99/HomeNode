/**
 * @file    smrt_core_event.cpp
 * @brief   Lightweight event bus — dispatches module events to notification channels
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_event.h"
    #include <cstring>
    #include <cstdint>
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Checks if an event type matches a filter bitmask
 * @param  event_type  Event type (single bit)
 * @param  filter      Filter bitmask
 * @return 1 if matches, 0 otherwise
 */
int smrt_event_matches_filter(uint8_t event_type, uint8_t filter) {
    return (event_type & filter) ? 1 : 0;
}

/**
 * @brief  Returns a human-readable name for an event type
 * @param  event_type  Event type (single bit)
 * @return Static string with event name
 */
const char *smrt_event_type_name(uint8_t event_type) {
    switch (event_type) {
        case SMRT_EVT_SEC_TRIGGERED:  return "sec_triggered";
        case SMRT_EVT_SEC_ARMED:      return "sec_armed";
        case SMRT_EVT_SEC_DISARMED:   return "sec_disarmed";
        case SMRT_EVT_ACC_AUTHORIZED: return "acc_authorized";
        case SMRT_EVT_ACC_DENIED:     return "acc_denied";
        case SMRT_EVT_NRG_OVERLOAD:   return "nrg_overload";
        case SMRT_EVT_ENV_ALERT:      return "env_alert";
        case SMRT_EVT_RLY_CHANGED:    return "rly_changed";
        default:                       return "unknown";
    }
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/**
 * @brief  Initializes the event bus
 * @return void
 */
void smrt_event_init(void) {
    SMRT_DEBUG_LOG("Event bus initialized");
}

/**
 * @brief  Publishes an event to all registered notification channels
 * @param  event_type   Event type bitmask
 * @param  json_payload JSON string with event details
 * @return void
 */
void smrt_event_publish(uint8_t event_type, const char *json_payload) {
    SMRT_DEBUG_PRINTF("EVENT: %s — %s\n",
                      smrt_event_type_name(event_type),
                      json_payload ? json_payload : "");

    /* Forward to MQTT if available */
    #ifdef SMRT_MQTT
    extern void smrt_mqtt_publish_event(uint8_t event_type, const char *payload);
    smrt_mqtt_publish_event(event_type, json_payload);
    #endif

    /* Forward to webhooks if available */
    #ifdef SMRT_WEBHOOK
    extern void smrt_webhook_send_event(uint8_t event_type, const char *payload);
    smrt_webhook_send_event(event_type, json_payload);
    #endif
}

#endif // UNIT_TEST
