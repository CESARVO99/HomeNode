/**
 * @file    smrt_core_module.cpp
 * @brief   Module registry, lifecycle management and WS command dispatch
 * @project HOMENODE
 * @version 0.2.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_module.h"
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Static registry
//-----------------------------------------------------------------------------
static const smrt_module_t *smrt_registry[SMRT_MAX_MODULES];  /**< Module pointer array */
static int smrt_registry_count = 0;                           /**< Number of registered modules */

//-----------------------------------------------------------------------------
// Registry API
//-----------------------------------------------------------------------------

/**
 * @brief  Registers a module descriptor into the platform registry.
 * @param  mod  Pointer to a static smrt_module_t
 * @return 1 on success, 0 if registry full or mod/id is NULL
 */
int smrt_module_register(const smrt_module_t *mod) {
    if (!mod || !mod->id) {
        return 0;
    }
    if (smrt_registry_count >= SMRT_MAX_MODULES) {
        return 0;
    }
    smrt_registry[smrt_registry_count] = mod;
    smrt_registry_count++;
    return 1;
}

/**
 * @brief  Returns the number of currently registered modules.
 * @return Count [0..SMRT_MAX_MODULES]
 */
int smrt_module_count(void) {
    return smrt_registry_count;
}

/**
 * @brief  Returns a pointer to the i-th registered module.
 * @param  index  Zero-based index
 * @return Pointer to smrt_module_t, or NULL if index out of range
 */
const smrt_module_t *smrt_module_get(int index) {
    if (index < 0 || index >= smrt_registry_count) {
        return (const smrt_module_t *)0;
    }
    return smrt_registry[index];
}

/**
 * @brief  Looks up a module by its ID string.
 * @param  id  Module ID to search for
 * @return Pointer to smrt_module_t, or NULL if not found
 */
const smrt_module_t *smrt_module_find(const char *id) {
    int i;
    if (!id) {
        return (const smrt_module_t *)0;
    }
    for (i = 0; i < smrt_registry_count; i++) {
        if (strcmp(smrt_registry[i]->id, id) == 0) {
            return smrt_registry[i];
        }
    }
    return (const smrt_module_t *)0;
}

//-----------------------------------------------------------------------------
// Lifecycle helpers
//-----------------------------------------------------------------------------

/**
 * @brief  Calls init() on all registered modules (in registration order).
 * @return void
 */
void smrt_module_init_all(void) {
    int i;
    for (i = 0; i < smrt_registry_count; i++) {
        if (smrt_registry[i]->init) {
            smrt_registry[i]->init();
        }
    }
}

/**
 * @brief  Calls loop() on all registered modules (in registration order).
 * @return void
 */
void smrt_module_loop_all(void) {
    int i;
    for (i = 0; i < smrt_registry_count; i++) {
        if (smrt_registry[i]->loop) {
            smrt_registry[i]->loop();
        }
    }
}

//-----------------------------------------------------------------------------
// WebSocket dispatch with prefix stripping
//-----------------------------------------------------------------------------

/**
 * @brief  Routes a WebSocket command to the matching module.
 *
 * Iterates through registered modules. For each, checks if the command
 * starts with "id_". If so, strips the prefix and calls ws_handler
 * with the remaining sub-command.
 *
 * Example: cmd="env_read", module id="env" -> handler receives "read"
 *
 * @param  cmd     Full command string
 * @param  doc     Pointer to parsed JSON document
 * @param  client  Pointer to WebSocket client
 * @return 1 if a module handled the command, 0 otherwise
 */
int smrt_module_dispatch(const char *cmd, void *doc, void *client) {
    int i;
    if (!cmd) {
        return 0;
    }
    for (i = 0; i < smrt_registry_count; i++) {
        const char *id = smrt_registry[i]->id;
        int id_len = (int)strlen(id);
        if (strncmp(cmd, id, id_len) == 0 && cmd[id_len] == '_') {
            if (smrt_registry[i]->ws_handler) {
                smrt_registry[i]->ws_handler(&cmd[id_len + 1], doc, client);
            }
            return 1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Telemetry aggregation
//-----------------------------------------------------------------------------

/**
 * @brief  Calls get_telemetry() on all registered modules.
 * @param  data  Pointer to JSON object where modules append their data
 * @return void
 */
void smrt_module_get_telemetry_all(void *data) {
    int i;
    for (i = 0; i < smrt_registry_count; i++) {
        if (smrt_registry[i]->get_telemetry) {
            smrt_registry[i]->get_telemetry(data);
        }
    }
}

//-----------------------------------------------------------------------------
// Test-only: reset registry
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
/**
 * @brief  Resets the module registry to empty state.
 *         Only available in UNIT_TEST builds.
 * @return void
 */
void smrt_module_reset(void) {
    int i;
    for (i = 0; i < SMRT_MAX_MODULES; i++) {
        smrt_registry[i] = (const smrt_module_t *)0;
    }
    smrt_registry_count = 0;
}
#endif
