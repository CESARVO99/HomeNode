/**
 * @file    smrt_core_node.cpp
 * @brief   Node identity and configuration for multi-node HomeNode system
 * @project HOMENODE
 * @version 1.0.0
 */

#ifdef UNIT_TEST
    #include "smrt_core_node.h"
    #include <string.h>
    #include <stdio.h>
#else
    #include "smrt_core.h"
    #include <Arduino.h>
    #include <WiFi.h>
#endif

//-----------------------------------------------------------------------------
// Module name table (for bitmask-to-string conversion)
//-----------------------------------------------------------------------------
static const char *smrt_node_mod_names[] = {
    "env", "rly", "sec", "plg", "nrg", "acc"
};
#define SMRT_NODE_MOD_COUNT 6

//-----------------------------------------------------------------------------
// Testable functions (native + ESP32)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a module bitmask for GPIO conflicts.
 *         PLG+NRG share GPIO34-39, PLG+ACC share GPIO2.
 * @return 0 if valid, or bitmask of conflicting pairs
 */
uint8_t smrt_node_validate_modules(uint8_t mask) {
    uint8_t conflicts = 0;

    /* PLG and NRG share ADC pins (GPIO34-39) */
    if ((mask & SMRT_NODE_MOD_PLG) && (mask & SMRT_NODE_MOD_NRG)) {
        conflicts |= SMRT_NODE_CONFLICT_PLG_NRG;
    }

    /* PLG and ACC share GPIO2 (relay/lock) */
    if ((mask & SMRT_NODE_MOD_PLG) && (mask & SMRT_NODE_MOD_ACC)) {
        conflicts |= SMRT_NODE_CONFLICT_PLG_ACC;
    }

    return conflicts;
}

/**
 * @brief  Validates a node name (non-null, 1-32 chars, no control chars)
 */
int smrt_node_validate_name(const char *name) {
    if (!name || name[0] == '\0') return 0;

    int len = 0;
    while (name[len] != '\0') {
        if (name[len] < 0x20) return 0; /* No control characters */
        len++;
        if (len > 32) return 0;
    }
    return 1;
}

/**
 * @brief  Converts module bitmask to comma-separated string
 */
char *smrt_node_modules_to_string(uint8_t mask, char *buf, int len) {
    if (!buf || len < 2) return buf;
    buf[0] = '\0';

    int pos = 0;
    for (int i = 0; i < SMRT_NODE_MOD_COUNT; i++) {
        if (mask & (1 << i)) {
            if (pos > 0 && pos < len - 1) {
                buf[pos++] = ',';
            }
            const char *name = smrt_node_mod_names[i];
            while (*name && pos < len - 1) {
                buf[pos++] = *name++;
            }
        }
    }
    buf[pos] = '\0';
    return buf;
}

/**
 * @brief  Returns count of enabled modules in bitmask
 */
int smrt_node_module_count(uint8_t mask) {
    int count = 0;
    for (int i = 0; i < SMRT_NODE_MOD_COUNT; i++) {
        if (mask & (1 << i)) count++;
    }
    return count;
}

//-----------------------------------------------------------------------------
// ESP32-only implementation
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

static char smrt_node_id[SMRT_NODE_ID_MAX]       = {0};
static char smrt_node_name[SMRT_NODE_NAME_MAX]    = {0};
static char smrt_node_room[SMRT_NODE_ROOM_MAX]    = {0};
static uint8_t smrt_node_modules_mask             = SMRT_NODE_MOD_ENV; /* Default: ENV only */

/**
 * @brief  Generates node_id from ESP32 MAC address (12 hex uppercase)
 */
static void smrt_node_generate_id(void) {
    uint64_t mac = ESP.getEfuseMac();
    uint8_t *bytes = (uint8_t *)&mac;
    snprintf(smrt_node_id, SMRT_NODE_ID_MAX, "%02X%02X%02X%02X%02X%02X",
             bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
}

/**
 * @brief  Initializes node identity from NVS
 */
void smrt_node_init(void) {
    smrt_node_generate_id();

    /* Load name and room from NVS */
    smrt_nvs_get_string(SMRT_NODE_NVS_NAMESPACE, "name", smrt_node_name, SMRT_NODE_NAME_MAX);
    smrt_nvs_get_string(SMRT_NODE_NVS_NAMESPACE, "room", smrt_node_room, SMRT_NODE_ROOM_MAX);

    /* Load module bitmask (default: ENV only) */
    int32_t mask = 0;
    if (smrt_nvs_get_int(SMRT_NODE_NVS_NAMESPACE, "modules", &mask, SMRT_NODE_MOD_ENV)) {
        smrt_node_modules_mask = (uint8_t)(mask & SMRT_NODE_MOD_ALL);
    }

    SMRT_DEBUG_PRINTF("[NODE] ID: %s  Name: '%s'  Room: '%s'  Modules: 0x%02X\n",
                      smrt_node_id, smrt_node_name, smrt_node_room, smrt_node_modules_mask);
}

const char *smrt_node_get_id(void) { return smrt_node_id; }
const char *smrt_node_get_name(void) { return smrt_node_name; }
const char *smrt_node_get_room(void) { return smrt_node_room; }
uint8_t smrt_node_get_modules(void) { return smrt_node_modules_mask; }

int smrt_node_set_name(const char *name) {
    if (!smrt_node_validate_name(name)) return 0;
    strncpy(smrt_node_name, name, SMRT_NODE_NAME_MAX - 1);
    smrt_node_name[SMRT_NODE_NAME_MAX - 1] = '\0';
    smrt_nvs_set_string(SMRT_NODE_NVS_NAMESPACE, "name", smrt_node_name);
    return 1;
}

int smrt_node_set_room(const char *room) {
    if (!smrt_node_validate_name(room)) return 0; /* Same validation */
    strncpy(smrt_node_room, room, SMRT_NODE_ROOM_MAX - 1);
    smrt_node_room[SMRT_NODE_ROOM_MAX - 1] = '\0';
    smrt_nvs_set_string(SMRT_NODE_NVS_NAMESPACE, "room", smrt_node_room);
    return 1;
}

int smrt_node_set_modules(uint8_t mask) {
    mask &= SMRT_NODE_MOD_ALL;
    if (smrt_node_validate_modules(mask) != 0) return 0;
    smrt_node_modules_mask = mask;
    smrt_nvs_set_int(SMRT_NODE_NVS_NAMESPACE, "modules", (int32_t)mask);
    return 1;
}

int smrt_node_has_module(uint8_t mod) {
    return (smrt_node_modules_mask & mod) ? 1 : 0;
}

/**
 * @brief  Handles node WebSocket commands
 */
void smrt_node_ws_handler(const char *cmd, void *json_doc, uint32_t client_id) {
    extern AsyncWebSocket smrt_ws;
    JsonDocument &doc = *(JsonDocument *)json_doc;
    JsonDocument resp;

    if (strcmp(cmd, "node_set_name") == 0) {
        const char *name = doc["name"];
        if (name && smrt_node_set_name(name)) {
            resp["node_result"] = true;
            resp["node_msg"]    = "Nombre actualizado";
        } else {
            resp["node_result"] = false;
            resp["node_msg"]    = "Nombre invalido";
        }
    }
    else if (strcmp(cmd, "node_set_room") == 0) {
        const char *room = doc["room"];
        if (room && smrt_node_set_room(room)) {
            resp["node_result"] = true;
            resp["node_msg"]    = "Habitacion actualizada";
        } else {
            resp["node_result"] = false;
            resp["node_msg"]    = "Nombre de habitacion invalido";
        }
    }
    else if (strcmp(cmd, "node_set_modules") == 0) {
        int mask = doc["modules"] | -1;
        if (mask < 0 || mask > SMRT_NODE_MOD_ALL) {
            resp["node_result"] = false;
            resp["node_msg"]    = "Bitmask invalido (0x00-0x3F)";
        } else {
            uint8_t conflicts = smrt_node_validate_modules((uint8_t)mask);
            if (conflicts != 0) {
                resp["node_result"] = false;
                resp["node_msg"]    = "Conflicto GPIO entre modulos seleccionados";
                resp["conflicts"]   = conflicts;
            } else if (smrt_node_set_modules((uint8_t)mask)) {
                resp["node_result"] = true;
                resp["node_msg"]    = "Modulos actualizados. Reinicia para aplicar.";
                resp["reboot"]      = true;
            } else {
                resp["node_result"] = false;
                resp["node_msg"]    = "Error al guardar";
            }
        }
    }
    else if (strcmp(cmd, "node_status") == 0) {
        resp["type"]     = "node_status";
        resp["node_id"]  = smrt_node_get_id();
        resp["name"]     = smrt_node_get_name();
        resp["room"]     = smrt_node_get_room();
        resp["modules"]  = smrt_node_get_modules();

        char mod_str[32];
        smrt_node_modules_to_string(smrt_node_get_modules(), mod_str, sizeof(mod_str));
        resp["modules_str"] = mod_str;
    }
    else {
        return;
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.text(client_id, output);
}

/**
 * @brief  Adds node identity fields to telemetry JSON
 */
void smrt_node_get_telemetry(void *json_obj) {
    JsonDocument &doc = *(JsonDocument *)json_obj;
    doc["node_id"] = smrt_node_get_id();
    doc["name"]    = smrt_node_get_name();
    doc["room"]    = smrt_node_get_room();
}

#endif // UNIT_TEST
