/**
 * @file    smrt_core_auto.h
 * @brief   Local automation rules engine — IF sensor THEN action
 * @project HOMENODE
 * @version 1.4.0
 *
 * Evaluates rules on each sensor reading. Rules can trigger local actions
 * (via module dispatch) or remote actions (via MQTT publish to target node).
 *
 * Max 8 rules, stored in NVS namespace "auto".
 */

#ifndef SMRT_CORE_AUTO_H
#define SMRT_CORE_AUTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define SMRT_AUTO_MAX_RULES         8
#define SMRT_AUTO_NVS_NAMESPACE     "auto"
#define SMRT_AUTO_SOURCE_MAX        24   /**< e.g., "env.temperature" */
#define SMRT_AUTO_ACTION_MAX        32   /**< e.g., "rly_set:0:1" */
#define SMRT_AUTO_NODE_MAX          13   /**< node_id or "*" for local */
#define SMRT_AUTO_NAME_MAX          24

/** Comparator types */
#define SMRT_AUTO_CMP_GT            0    /**< Greater than */
#define SMRT_AUTO_CMP_LT            1    /**< Less than */
#define SMRT_AUTO_CMP_EQ            2    /**< Equal (within 0.1 tolerance) */
#define SMRT_AUTO_CMP_CHANGE        3    /**< Any change from last value */

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

typedef struct {
    uint8_t  enabled;
    char     source_node[SMRT_AUTO_NODE_MAX];
    char     source_channel[SMRT_AUTO_SOURCE_MAX];
    uint8_t  comparator;
    float    threshold;
    char     target_node[SMRT_AUTO_NODE_MAX];
    char     action[SMRT_AUTO_ACTION_MAX];
    char     name[SMRT_AUTO_NAME_MAX];
    uint32_t cooldown_s;
    uint32_t last_trigger;  /**< Epoch timestamp of last trigger */
} smrt_auto_rule_t;

//-----------------------------------------------------------------------------
// Testable functions
//-----------------------------------------------------------------------------

/**
 * @brief  Evaluates a comparator condition
 * @param  value       Current sensor value
 * @param  threshold   Rule threshold
 * @param  comparator  SMRT_AUTO_CMP_*
 * @param  last_value  Previous value (for CMP_CHANGE)
 * @return 1 if condition is met, 0 otherwise
 */
int smrt_auto_evaluate(float value, float threshold, uint8_t comparator, float last_value);

/**
 * @brief  Validates a rule structure
 * @return 1 if valid, 0 if invalid
 */
int smrt_auto_validate_rule(const smrt_auto_rule_t *rule);

/**
 * @brief  Validates a comparator value
 */
int smrt_auto_validate_comparator(uint8_t cmp);

#ifndef UNIT_TEST
//-----------------------------------------------------------------------------
// ESP32-only functions
//-----------------------------------------------------------------------------

void smrt_auto_init(void);
void smrt_auto_loop(void);  /**< Called from main loop — checks rules */
int  smrt_auto_set_rule(int index, const smrt_auto_rule_t *rule);
int  smrt_auto_delete_rule(int index);
const smrt_auto_rule_t *smrt_auto_get_rule(int index);
int  smrt_auto_active_count(void);
void smrt_auto_ws_handler(const char *cmd, void *json_doc, uint32_t client_id);

/**
 * @brief  Called by modules when a sensor value changes.
 *         Checks all rules matching this channel and fires actions.
 * @param  channel  Channel name (e.g., "env.temperature")
 * @param  value    Current value
 */
void smrt_auto_check_value(const char *channel, float value);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_AUTO_H
