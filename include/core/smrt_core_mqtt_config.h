/**
 * @file    smrt_core_mqtt_config.h
 * @brief   MQTT client configuration defines
 * @project HOMENODE
 * @version 1.0.0
 */

#ifndef SMRT_CORE_MQTT_CONFIG_H
#define SMRT_CORE_MQTT_CONFIG_H

#define SMRT_MQTT_SERVER_MAX        64      /**< Max broker hostname length */
#define SMRT_MQTT_PORT_DEFAULT      1883    /**< Default MQTT port */
#define SMRT_MQTT_USER_MAX          33      /**< Max username length */
#define SMRT_MQTT_PASS_MAX          65      /**< Max password length */
#define SMRT_MQTT_TOPIC_MAX         96      /**< Max topic string length */
#define SMRT_MQTT_TOPIC_PREFIX      "homenode/" /**< Default topic prefix */
#define SMRT_MQTT_RECONNECT_MIN_MS  1000    /**< Min reconnect interval */
#define SMRT_MQTT_RECONNECT_MAX_MS  60000   /**< Max reconnect interval */
#define SMRT_MQTT_KEEPALIVE_S       60      /**< Keep-alive interval */
#define SMRT_MQTT_PUBLISH_INTERVAL  30000   /**< Telemetry publish interval (ms) */
#define SMRT_MQTT_BUFFER_SIZE       512     /**< PubSubClient buffer size */
#define SMRT_MQTT_NVS_NAMESPACE     "mqtt"  /**< NVS namespace */

/** Discovery interval — nodes announce periodically */
#define SMRT_MQTT_DISCOVERY_INTERVAL_MS  60000  /**< Discovery announce interval (60s) */
#define SMRT_MQTT_DISCOVERY_TOPIC   "homenode/discovery/announce" /**< Discovery announce topic */
#define SMRT_MQTT_DISCOVERY_REQ     "homenode/discovery/request"  /**< Discovery request topic */

#endif // SMRT_CORE_MQTT_CONFIG_H
