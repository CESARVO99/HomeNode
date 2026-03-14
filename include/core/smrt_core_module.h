/**
 * @file    smrt_core_module.h
 * @brief   Module registration system — struct, registry API and dispatch
 * @project HOMENODE
 * @version 0.2.0
 *
 * Each loadable module fills a smrt_module_t descriptor and registers it
 * with smrt_module_register(). The core platform then calls init/loop/
 * ws_handler/get_telemetry on all registered modules automatically.
 *
 * WebSocket command dispatch uses prefix stripping:
 *   "env_read" -> module "env" receives "read"
 */

#ifndef SMRT_CORE_MODULE_H
#define SMRT_CORE_MODULE_H

#include "smrt_core_config.h"
#include "smrt_mc_format.h"       /* uint8, int32, ... */
#include <string.h>               /* strlen, strncmp */

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * MODULE INTERFACE STRUCT                                    *
 *************************************************************/

/**
 * @brief  Module descriptor — every HomeNode module must provide one.
 *
 * Fields:
 *   id            Short identifier used as WS command prefix ("env", "sec", ...)
 *   name          Human-readable name (for UI/log)
 *   version       Semantic version string
 *   init          Called once during setup (after core init)
 *   loop          Called every main loop iteration
 *   ws_handler    Called when a WS command matches this module's prefix.
 *                 Receives the sub-command (prefix already stripped).
 *   get_telemetry Called periodically to fill a JSON object with sensor data.
 *
 * In UNIT_TEST builds, ArduinoJson / AsyncWebSocket types are replaced
 * with void* so the module system can be tested natively.
 */
typedef struct {
    const char *id;             /**< Module ID prefix (e.g. "env") */
    const char *name;           /**< Human-readable module name */
    const char *version;        /**< Module version string */
    void (*init)(void);         /**< One-time initialization callback */
    void (*loop)(void);         /**< Per-iteration loop callback */
#ifndef UNIT_TEST
    void (*ws_handler)(const char *cmd, void *doc, void *client);
    void (*get_telemetry)(void *data);
#else
    void (*ws_handler)(const char *cmd, void *doc, void *client);
    void (*get_telemetry)(void *data);
#endif
} smrt_module_t;

/*************************************************************
 * REGISTRY API                                              *
 *************************************************************/

/**
 * @brief  Registers a module descriptor into the platform registry.
 * @param  mod  Pointer to a static smrt_module_t (must persist for lifetime)
 * @return 1 on success, 0 if registry is full or mod is NULL
 */
int smrt_module_register(const smrt_module_t *mod);

/**
 * @brief  Returns the number of currently registered modules.
 * @return Count [0..SMRT_MAX_MODULES]
 */
int smrt_module_count(void);

/**
 * @brief  Returns a pointer to the i-th registered module.
 * @param  index  Zero-based index
 * @return Pointer to smrt_module_t, or NULL if index out of range
 */
const smrt_module_t *smrt_module_get(int index);

/**
 * @brief  Looks up a module by its ID string.
 * @param  id  Module ID to search for (e.g. "env")
 * @return Pointer to smrt_module_t, or NULL if not found
 */
const smrt_module_t *smrt_module_find(const char *id);

/*************************************************************
 * LIFECYCLE HELPERS                                         *
 *************************************************************/

/**
 * @brief  Calls init() on all registered modules (in registration order).
 * @return void
 */
void smrt_module_init_all(void);

/**
 * @brief  Calls loop() on all registered modules (in registration order).
 * @return void
 */
void smrt_module_loop_all(void);

/*************************************************************
 * WEBSOCKET DISPATCH                                        *
 *************************************************************/

/**
 * @brief  Routes a WebSocket command to the matching module.
 *
 * The command string is expected as "prefix_subcommand" (e.g. "env_read").
 * This function finds the module whose id matches the prefix, strips it,
 * and calls the module's ws_handler with the sub-command.
 *
 * @param  cmd     Full command string (e.g. "env_read")
 * @param  doc     Pointer to parsed JSON document (void* for portability)
 * @param  client  Pointer to WebSocket client (void* for portability)
 * @return 1 if a module handled the command, 0 otherwise
 */
int smrt_module_dispatch(const char *cmd, void *doc, void *client);

/*************************************************************
 * TELEMETRY AGGREGATION                                     *
 *************************************************************/

/**
 * @brief  Calls get_telemetry() on all registered modules.
 * @param  data  Pointer to a JSON object where modules append their data
 * @return void
 */
void smrt_module_get_telemetry_all(void *data);

/*************************************************************
 * TEST-ONLY: RESET REGISTRY                                 *
 *************************************************************/
#ifdef UNIT_TEST
/**
 * @brief  Resets the module registry to empty state (test use only).
 * @return void
 */
void smrt_module_reset(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_MODULE_H
