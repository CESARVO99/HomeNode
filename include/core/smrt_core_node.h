/**
 * @file    smrt_core_node.h
 * @brief   Node identity and configuration for multi-node HomeNode system
 * @project HOMENODE
 * @version 1.0.0
 *
 * Each ESP32 has a unique identity:
 *   - node_id: derived from MAC address (e.g., "A4CF1200FF01")
 *   - name: user-friendly name (e.g., "Living Room Sensors")
 *   - room: room assignment (e.g., "Living Room")
 *   - modules: bitmask of active modules
 *
 * Stored in NVS namespace "node_cfg".
 */

#ifndef SMRT_CORE_NODE_H
#define SMRT_CORE_NODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define SMRT_NODE_NVS_NAMESPACE     "node_cfg"  /**< NVS namespace */
#define SMRT_NODE_NAME_MAX          33          /**< Max name length (32+null) */
#define SMRT_NODE_ROOM_MAX          33          /**< Max room length (32+null) */
#define SMRT_NODE_ID_MAX            13          /**< Max node_id length (12 hex + null) */

/** Module activation bitmask */
#define SMRT_NODE_MOD_ENV           0x01
#define SMRT_NODE_MOD_RLY           0x02
#define SMRT_NODE_MOD_SEC           0x04
#define SMRT_NODE_MOD_PLG           0x08
#define SMRT_NODE_MOD_NRG           0x10
#define SMRT_NODE_MOD_ACC           0x20
#define SMRT_NODE_MOD_ALL           0x3F

/** GPIO conflict pairs (bitmask combinations that conflict) */
#define SMRT_NODE_CONFLICT_PLG_NRG  (SMRT_NODE_MOD_PLG | SMRT_NODE_MOD_NRG)
#define SMRT_NODE_CONFLICT_PLG_ACC  (SMRT_NODE_MOD_PLG | SMRT_NODE_MOD_ACC)

//-----------------------------------------------------------------------------
// Testable functions (work on native + ESP32)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a module bitmask for GPIO conflicts
 * @param  mask  Bitmask of modules to validate
 * @return 0 if valid, or a bitmask of conflicting module pairs
 */
uint8_t smrt_node_validate_modules(uint8_t mask);

/**
 * @brief  Validates a node name string
 * @param  name  Name to validate (non-null, 1-32 chars)
 * @return 1 if valid, 0 if invalid
 */
int smrt_node_validate_name(const char *name);

/**
 * @brief  Converts a module bitmask to a comma-separated string
 * @param  mask  Module bitmask
 * @param  buf   Output buffer (must be at least 32 bytes)
 * @param  len   Buffer length
 * @return Pointer to buf
 */
char *smrt_node_modules_to_string(uint8_t mask, char *buf, int len);

/**
 * @brief  Returns the number of modules enabled in a bitmask
 * @param  mask  Module bitmask
 * @return Count of enabled modules (0-6)
 */
int smrt_node_module_count(uint8_t mask);

#ifndef UNIT_TEST
//-----------------------------------------------------------------------------
// ESP32-only functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes node identity from NVS. Generates node_id from MAC.
 */
void smrt_node_init(void);

/**
 * @brief  Returns the node's unique ID (12 hex chars from MAC)
 * @return Pointer to static string (never null)
 */
const char *smrt_node_get_id(void);

/**
 * @brief  Returns the node's friendly name
 * @return Pointer to static string (may be empty if not set)
 */
const char *smrt_node_get_name(void);

/**
 * @brief  Sets the node's friendly name and saves to NVS
 * @param  name  Name string (max 32 chars)
 * @return 1 on success, 0 on failure
 */
int smrt_node_set_name(const char *name);

/**
 * @brief  Returns the node's room assignment
 * @return Pointer to static string (may be empty if not set)
 */
const char *smrt_node_get_room(void);

/**
 * @brief  Sets the node's room and saves to NVS
 * @param  room  Room string (max 32 chars)
 * @return 1 on success, 0 on failure
 */
int smrt_node_set_room(const char *room);

/**
 * @brief  Returns the active module bitmask
 * @return Module bitmask (0x00-0x3F)
 */
uint8_t smrt_node_get_modules(void);

/**
 * @brief  Sets the active module bitmask and saves to NVS
 * @param  mask  Module bitmask (validated before saving)
 * @return 1 on success, 0 if validation fails
 */
int smrt_node_set_modules(uint8_t mask);

/**
 * @brief  Checks if a specific module is enabled
 * @param  mod  Module bit (e.g., SMRT_NODE_MOD_ENV)
 * @return 1 if enabled, 0 if not
 */
int smrt_node_has_module(uint8_t mod);

/**
 * @brief  Handles node-related WebSocket commands
 * @param  cmd        Command string (node_set_name, node_set_room, node_set_modules, node_status)
 * @param  json_doc   Pointer to JsonDocument
 * @param  client_id  WebSocket client ID
 */
void smrt_node_ws_handler(const char *cmd, void *json_doc, uint32_t client_id);

/**
 * @brief  Adds node identity to a JSON telemetry object
 * @param  json_obj  Pointer to JsonObject
 */
void smrt_node_get_telemetry(void *json_obj);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_NODE_H
