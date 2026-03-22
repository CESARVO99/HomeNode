/**
 * @file    smrt_core_auto.cpp
 * @brief   Local automation rules engine
 * @project HOMENODE
 * @version 1.4.0
 */

#ifdef UNIT_TEST
    #include "smrt_core_auto.h"
    #include <string.h>
    #include <math.h>
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Testable functions
//-----------------------------------------------------------------------------

int smrt_auto_evaluate(float value, float threshold, uint8_t comparator, float last_value) {
    switch (comparator) {
        case SMRT_AUTO_CMP_GT:     return (value > threshold) ? 1 : 0;
        case SMRT_AUTO_CMP_LT:     return (value < threshold) ? 1 : 0;
        case SMRT_AUTO_CMP_EQ:     return (fabsf(value - threshold) < 0.1f) ? 1 : 0;
        case SMRT_AUTO_CMP_CHANGE: return (fabsf(value - last_value) > 0.01f) ? 1 : 0;
        default: return 0;
    }
}

int smrt_auto_validate_comparator(uint8_t cmp) {
    return (cmp <= SMRT_AUTO_CMP_CHANGE) ? 1 : 0;
}

int smrt_auto_validate_rule(const smrt_auto_rule_t *rule) {
    if (!rule) return 0;
    if (rule->source_channel[0] == '\0') return 0;
    if (rule->action[0] == '\0') return 0;
    if (!smrt_auto_validate_comparator(rule->comparator)) return 0;
    if (rule->cooldown_s > 86400) return 0; /* Max 24h cooldown */
    return 1;
}

//-----------------------------------------------------------------------------
// ESP32-only implementation
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

static smrt_auto_rule_t smrt_auto_rules[SMRT_AUTO_MAX_RULES];
static float smrt_auto_last_values[SMRT_AUTO_MAX_RULES]; /* For CMP_CHANGE */

static void auto_save_rule(int index) {
    char key[8];
    snprintf(key, sizeof(key), "rule_%d", index);

    JsonDocument doc;
    doc["en"]   = smrt_auto_rules[index].enabled;
    doc["sn"]   = smrt_auto_rules[index].source_node;
    doc["sc"]   = smrt_auto_rules[index].source_channel;
    doc["cmp"]  = smrt_auto_rules[index].comparator;
    doc["thr"]  = smrt_auto_rules[index].threshold;
    doc["tn"]   = smrt_auto_rules[index].target_node;
    doc["act"]  = smrt_auto_rules[index].action;
    doc["name"] = smrt_auto_rules[index].name;
    doc["cd"]   = smrt_auto_rules[index].cooldown_s;

    String output;
    serializeJson(doc, output);
    smrt_nvs_set_string(SMRT_AUTO_NVS_NAMESPACE, key, output.c_str());
}

static void auto_load_rule(int index) {
    char key[8];
    snprintf(key, sizeof(key), "rule_%d", index);
    char buf[256];

    if (!smrt_nvs_get_string(SMRT_AUTO_NVS_NAMESPACE, key, buf, sizeof(buf))) return;

    JsonDocument doc;
    if (deserializeJson(doc, buf)) return;

    smrt_auto_rules[index].enabled = doc["en"] | 0;
    strncpy(smrt_auto_rules[index].source_node, doc["sn"] | "*", SMRT_AUTO_NODE_MAX - 1);
    strncpy(smrt_auto_rules[index].source_channel, doc["sc"] | "", SMRT_AUTO_SOURCE_MAX - 1);
    smrt_auto_rules[index].comparator = doc["cmp"] | 0;
    smrt_auto_rules[index].threshold = doc["thr"] | 0.0f;
    strncpy(smrt_auto_rules[index].target_node, doc["tn"] | "*", SMRT_AUTO_NODE_MAX - 1);
    strncpy(smrt_auto_rules[index].action, doc["act"] | "", SMRT_AUTO_ACTION_MAX - 1);
    strncpy(smrt_auto_rules[index].name, doc["name"] | "", SMRT_AUTO_NAME_MAX - 1);
    smrt_auto_rules[index].cooldown_s = doc["cd"] | 60;
}

void smrt_auto_init(void) {
    memset(smrt_auto_rules, 0, sizeof(smrt_auto_rules));
    memset(smrt_auto_last_values, 0, sizeof(smrt_auto_last_values));

    for (int i = 0; i < SMRT_AUTO_MAX_RULES; i++) {
        auto_load_rule(i);
    }
    SMRT_DEBUG_PRINTF("[AUTO] Initialized, %d active rules\n", smrt_auto_active_count());
}

void smrt_auto_loop(void) {
    /* Rules are evaluated via smrt_auto_check_value() called from modules */
}

int smrt_auto_active_count(void) {
    int count = 0;
    for (int i = 0; i < SMRT_AUTO_MAX_RULES; i++) {
        if (smrt_auto_rules[i].enabled) count++;
    }
    return count;
}

int smrt_auto_set_rule(int index, const smrt_auto_rule_t *rule) {
    if (index < 0 || index >= SMRT_AUTO_MAX_RULES) return 0;
    if (!smrt_auto_validate_rule(rule)) return 0;
    memcpy(&smrt_auto_rules[index], rule, sizeof(smrt_auto_rule_t));
    auto_save_rule(index);
    return 1;
}

int smrt_auto_delete_rule(int index) {
    if (index < 0 || index >= SMRT_AUTO_MAX_RULES) return 0;
    memset(&smrt_auto_rules[index], 0, sizeof(smrt_auto_rule_t));
    auto_save_rule(index);
    return 1;
}

const smrt_auto_rule_t *smrt_auto_get_rule(int index) {
    if (index < 0 || index >= SMRT_AUTO_MAX_RULES) return NULL;
    return &smrt_auto_rules[index];
}

/**
 * @brief  Called by modules when sensor values change.
 *         Evaluates matching rules and fires actions.
 */
void smrt_auto_check_value(const char *channel, float value) {
    if (!channel) return;
    uint32_t now_s = (uint32_t)(millis() / 1000);

    /* Try NTP epoch */
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        time_t epoch;
        time(&epoch);
        now_s = (uint32_t)epoch;
    }

    for (int i = 0; i < SMRT_AUTO_MAX_RULES; i++) {
        smrt_auto_rule_t *r = &smrt_auto_rules[i];
        if (!r->enabled) continue;
        if (strcmp(r->source_channel, channel) != 0) continue;

        /* Check if source_node matches (local = "*" or own node_id) */
        if (r->source_node[0] != '*' && strcmp(r->source_node, smrt_node_get_id()) != 0) continue;

        /* Cooldown check */
        if (r->last_trigger > 0 && (now_s - r->last_trigger) < r->cooldown_s) continue;

        /* Evaluate condition */
        if (!smrt_auto_evaluate(value, r->threshold, r->comparator, smrt_auto_last_values[i])) {
            smrt_auto_last_values[i] = value;
            continue;
        }

        smrt_auto_last_values[i] = value;
        r->last_trigger = now_s;

        SMRT_DEBUG_PRINTF("[AUTO] Rule '%s' fired: %s %.2f → %s\n",
                          r->name, channel, value, r->action);

        /* Execute action */
        if (r->target_node[0] == '*' || strcmp(r->target_node, smrt_node_get_id()) == 0) {
            /* Local action — dispatch via module system */
            JsonDocument cmd_doc;
            cmd_doc["cmd"] = r->action;
            smrt_module_dispatch(r->action, (void *)&cmd_doc, (void *)0);
        } else {
            /* Remote action — publish command to target node via MQTT */
            #ifdef SMRT_MQTT
            char topic[96];
            snprintf(topic, sizeof(topic), "homenode/%s/cmd", r->target_node);
            JsonDocument cmd_doc;
            cmd_doc["cmd"] = r->action;
            String output;
            serializeJson(cmd_doc, output);
            smrt_mqtt_publish_to(topic, output.c_str());
            #endif
        }
    }
}

/**
 * @brief  WebSocket command handler for automation rules
 */
void smrt_auto_ws_handler(const char *cmd, void *json_doc, uint32_t client_id) {
    extern AsyncWebSocket smrt_ws;
    JsonDocument &doc = *(JsonDocument *)json_doc;
    JsonDocument resp;

    if (strcmp(cmd, "auto_list") == 0) {
        resp["type"] = "auto_list";
        JsonArray rules = resp["rules"].to<JsonArray>();
        for (int i = 0; i < SMRT_AUTO_MAX_RULES; i++) {
            const smrt_auto_rule_t *r = &smrt_auto_rules[i];
            JsonObject robj = rules.add<JsonObject>();
            robj["index"]    = i;
            robj["enabled"]  = r->enabled;
            robj["name"]     = r->name;
            robj["src_node"] = r->source_node;
            robj["src_ch"]   = r->source_channel;
            robj["cmp"]      = r->comparator;
            robj["threshold"]= r->threshold;
            robj["tgt_node"] = r->target_node;
            robj["action"]   = r->action;
            robj["cooldown"] = r->cooldown_s;
        }
    }
    else if (strcmp(cmd, "auto_set") == 0) {
        int index = doc["index"] | -1;
        smrt_auto_rule_t rule;
        memset(&rule, 0, sizeof(rule));
        rule.enabled = doc["enabled"] | 1;
        strncpy(rule.source_node, doc["src_node"] | "*", SMRT_AUTO_NODE_MAX - 1);
        strncpy(rule.source_channel, doc["src_ch"] | "", SMRT_AUTO_SOURCE_MAX - 1);
        rule.comparator = doc["cmp"] | 0;
        rule.threshold = doc["threshold"] | 0.0f;
        strncpy(rule.target_node, doc["tgt_node"] | "*", SMRT_AUTO_NODE_MAX - 1);
        strncpy(rule.action, doc["action"] | "", SMRT_AUTO_ACTION_MAX - 1);
        strncpy(rule.name, doc["name"] | "", SMRT_AUTO_NAME_MAX - 1);
        rule.cooldown_s = doc["cooldown"] | 60;

        if (smrt_auto_set_rule(index, &rule)) {
            resp["auto_result"] = true;
            resp["auto_msg"]    = "Regla guardada";
        } else {
            resp["auto_result"] = false;
            resp["auto_msg"]    = "Error: datos invalidos";
        }
    }
    else if (strcmp(cmd, "auto_delete") == 0) {
        int index = doc["index"] | -1;
        if (smrt_auto_delete_rule(index)) {
            resp["auto_result"] = true;
            resp["auto_msg"]    = "Regla eliminada";
        } else {
            resp["auto_result"] = false;
            resp["auto_msg"]    = "Indice invalido";
        }
    }
    else { return; }

    String output;
    serializeJson(resp, output);
    smrt_ws.text(client_id, output);
}

#endif // UNIT_TEST
