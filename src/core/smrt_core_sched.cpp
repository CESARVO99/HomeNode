/**
 * @file    smrt_core_sched.cpp
 * @brief   Time-based task scheduler implementation
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_sched.h"
    #include <cstring>
    #include <cstdio>
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a task structure
 */
int smrt_sched_validate_task(const smrt_sched_task_t *task) {
    if (!task) return 0;
    if (task->hour > 23) return 0;
    if (task->minute > 59) return 0;
    if (task->days == 0 || task->days > SMRT_SCHED_DAYS_ALL) return 0;
    if (strlen(task->action) == 0) return 0;
    return 1;
}

/**
 * @brief  Checks if a task should fire at the given time
 */
int smrt_sched_match_time(const smrt_sched_task_t *task, int hour, int minute, int dow) {
    if (!task || !task->enabled) return 0;
    if (task->hour != (uint8_t)hour) return 0;
    if (task->minute != (uint8_t)minute) return 0;
    /* Check day bitmask: bit0=Sunday, bit1=Monday, ... bit6=Saturday */
    if (!(task->days & (1 << dow))) return 0;
    return 1;
}

/**
 * @brief  Parses an action string: "rly_set:0:1" → cmd="rly_set", args="0:1"
 */
int smrt_sched_parse_action(const char *action, char *cmd_out, char *args_out) {
    if (!action || !cmd_out || !args_out) return 0;
    cmd_out[0] = '\0';
    args_out[0] = '\0';

    /* Find first colon separator */
    const char *colon = strchr(action, ':');
    if (!colon) {
        /* No args — entire string is the command */
        strncpy(cmd_out, action, SMRT_SCHED_ACTION_LEN - 1);
        cmd_out[SMRT_SCHED_ACTION_LEN - 1] = '\0';
        return (strlen(cmd_out) > 0) ? 1 : 0;
    }

    /* Copy command (before colon) */
    int cmd_len = (int)(colon - action);
    if (cmd_len <= 0 || cmd_len >= SMRT_SCHED_ACTION_LEN) return 0;
    strncpy(cmd_out, action, cmd_len);
    cmd_out[cmd_len] = '\0';

    /* Copy args (after colon) */
    strncpy(args_out, colon + 1, SMRT_SCHED_ACTION_LEN - 1);
    args_out[SMRT_SCHED_ACTION_LEN - 1] = '\0';
    return 1;
}

/**
 * @brief  Validates a task index
 */
int smrt_sched_validate_index(int index) {
    return (index >= 0 && index < SMRT_SCHED_MAX_TASKS) ? 1 : 0;
}

/**
 * @brief  Converts a day bitmask to a human-readable string
 */
void smrt_sched_days_to_string(uint8_t days, char *buf, int buf_len) {
    if (!buf || buf_len < 2) return;
    buf[0] = '\0';

    static const char *day_names[] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};
    int first = 1;
    for (int i = 0; i < 7; i++) {
        if (days & (1 << i)) {
            if (!first) {
                int len = (int)strlen(buf);
                if (len + 1 < buf_len) { buf[len] = ','; buf[len + 1] = '\0'; }
            }
            int len = (int)strlen(buf);
            int name_len = (int)strlen(day_names[i]);
            if (len + name_len < buf_len) {
                strcat(buf, day_names[i]);
            }
            first = 0;
        }
    }
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

//-----------------------------------------------------------------------------
// Static state
//-----------------------------------------------------------------------------
static smrt_sched_task_t smrt_sched_tasks[SMRT_SCHED_MAX_TASKS];
static unsigned long smrt_sched_last_check = 0;
static int smrt_sched_last_fired_minute = -1;  /**< Prevents duplicate fires */

//-----------------------------------------------------------------------------
// NVS persistence helpers
//-----------------------------------------------------------------------------

/**
 * @brief  Saves a task to NVS as JSON string
 */
static void sched_save_task(int index) {
    char key[12];
    snprintf(key, sizeof(key), "task_%d", index);

    JsonDocument doc;
    doc["en"]   = smrt_sched_tasks[index].enabled;
    doc["h"]    = smrt_sched_tasks[index].hour;
    doc["m"]    = smrt_sched_tasks[index].minute;
    doc["d"]    = smrt_sched_tasks[index].days;
    doc["act"]  = smrt_sched_tasks[index].action;
    doc["name"] = smrt_sched_tasks[index].name;

    String json;
    serializeJson(doc, json);
    smrt_nvs_set_string(SMRT_SCHED_NVS_NAMESPACE, key, json.c_str());
}

/**
 * @brief  Loads a task from NVS JSON string
 */
static void sched_load_task(int index) {
    char key[12];
    snprintf(key, sizeof(key), "task_%d", index);

    char json_buf[256];
    if (!smrt_nvs_get_string(SMRT_SCHED_NVS_NAMESPACE, key, json_buf, sizeof(json_buf))) {
        memset(&smrt_sched_tasks[index], 0, sizeof(smrt_sched_task_t));
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, json_buf)) {
        memset(&smrt_sched_tasks[index], 0, sizeof(smrt_sched_task_t));
        return;
    }

    smrt_sched_tasks[index].enabled = doc["en"] | 0;
    smrt_sched_tasks[index].hour    = doc["h"]  | 0;
    smrt_sched_tasks[index].minute  = doc["m"]  | 0;
    smrt_sched_tasks[index].days    = doc["d"]  | 0;

    const char *act = doc["act"];
    if (act) strncpy(smrt_sched_tasks[index].action, act, SMRT_SCHED_ACTION_LEN - 1);

    const char *name = doc["name"];
    if (name) strncpy(smrt_sched_tasks[index].name, name, SMRT_SCHED_NAME_LEN - 1);
}

/**
 * @brief  Executes a scheduled action by dispatching through module system
 */
static void sched_execute_action(const smrt_sched_task_t *task) {
    char cmd[SMRT_SCHED_ACTION_LEN];
    char args[SMRT_SCHED_ACTION_LEN];
    if (!smrt_sched_parse_action(task->action, cmd, args)) return;

    SMRT_DEBUG_PRINTF("SCHED: Firing '%s' (action: %s)\n", task->name, task->action);

    /* Build synthetic JSON and dispatch */
    JsonDocument doc;
    doc["cmd"] = cmd;

    /* Parse colon-separated args: first arg = "index", second = "state"/"value" */
    if (strlen(args) > 0) {
        char *arg1 = strtok((char *)args, ":");
        char *arg2 = strtok(NULL, ":");

        if (arg1) {
            /* Try numeric first */
            char *endptr;
            long val = strtol(arg1, &endptr, 10);
            if (*endptr == '\0') {
                doc["index"] = (int)val;
            } else {
                doc["arg1"] = arg1;
            }
        }
        if (arg2) {
            char *endptr;
            long val = strtol(arg2, &endptr, 10);
            if (*endptr == '\0') {
                doc["state"] = (int)val;
            } else {
                doc["arg2"] = arg2;
            }
        }
    }

    smrt_module_dispatch(cmd, (void *)&doc, (void *)0);
}

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes the scheduler — loads tasks from NVS
 */
void smrt_sched_init(void) {
    for (int i = 0; i < SMRT_SCHED_MAX_TASKS; i++) {
        sched_load_task(i);
    }
    SMRT_DEBUG_PRINTF("Scheduler initialized: %d active tasks\n", smrt_sched_active_count());
}

/**
 * @brief  Scheduler loop — checks time and fires matching tasks
 */
void smrt_sched_loop(void) {
    unsigned long now = millis();
    if (now - smrt_sched_last_check < SMRT_SCHED_CHECK_INTERVAL) return;
    smrt_sched_last_check = now;

    if (!smrt_time_is_synced()) return;

    int hour   = smrt_time_get_hour();
    int minute = smrt_time_get_minute();
    int dow    = smrt_time_get_dow();
    if (hour < 0 || minute < 0 || dow < 0) return;

    /* Prevent firing same tasks multiple times in the same minute */
    int current_minute_id = hour * 60 + minute;
    if (current_minute_id == smrt_sched_last_fired_minute) return;
    smrt_sched_last_fired_minute = current_minute_id;

    /* Check all tasks */
    for (int i = 0; i < SMRT_SCHED_MAX_TASKS; i++) {
        if (smrt_sched_match_time(&smrt_sched_tasks[i], hour, minute, dow)) {
            sched_execute_action(&smrt_sched_tasks[i]);
        }
    }
}

/**
 * @brief  Sets/updates a scheduled task
 */
int smrt_sched_set_task(int index, const smrt_sched_task_t *task) {
    if (!smrt_sched_validate_index(index) || !task) return 0;
    if (!smrt_sched_validate_task(task)) return 0;
    memcpy(&smrt_sched_tasks[index], task, sizeof(smrt_sched_task_t));
    sched_save_task(index);
    return 1;
}

/**
 * @brief  Deletes a scheduled task
 */
int smrt_sched_delete_task(int index) {
    if (!smrt_sched_validate_index(index)) return 0;
    memset(&smrt_sched_tasks[index], 0, sizeof(smrt_sched_task_t));
    sched_save_task(index);
    return 1;
}

/**
 * @brief  Returns a pointer to a task (read-only)
 */
const smrt_sched_task_t *smrt_sched_get_task(int index) {
    if (!smrt_sched_validate_index(index)) return NULL;
    return &smrt_sched_tasks[index];
}

/**
 * @brief  Returns the number of active (enabled) tasks
 */
int smrt_sched_active_count(void) {
    int count = 0;
    for (int i = 0; i < SMRT_SCHED_MAX_TASKS; i++) {
        if (smrt_sched_tasks[i].enabled) count++;
    }
    return count;
}

/**
 * @brief  Handles scheduler WebSocket commands
 */
void smrt_sched_ws_handler(const char *cmd, void *json_doc, uint32_t client_id) {
    extern AsyncWebSocket smrt_ws;
    JsonDocument &doc = *(JsonDocument *)json_doc;
    JsonDocument resp;

    if (strcmp(cmd, "sched_list") == 0) {
        JsonArray tasks = resp["tasks"].to<JsonArray>();
        for (int i = 0; i < SMRT_SCHED_MAX_TASKS; i++) {
            JsonObject t = tasks.add<JsonObject>();
            t["index"]   = i;
            t["enabled"] = smrt_sched_tasks[i].enabled;
            t["hour"]    = smrt_sched_tasks[i].hour;
            t["minute"]  = smrt_sched_tasks[i].minute;
            t["days"]    = smrt_sched_tasks[i].days;
            t["action"]  = smrt_sched_tasks[i].action;
            t["name"]    = smrt_sched_tasks[i].name;
        }
        resp["type"] = "sched_list";
    }
    else if (strcmp(cmd, "sched_set") == 0) {
        int index = doc["index"] | -1;
        smrt_sched_task_t task;
        task.enabled = doc["enabled"] | 1;
        task.hour    = doc["hour"]    | 0;
        task.minute  = doc["minute"]  | 0;
        task.days    = doc["days"]    | SMRT_SCHED_DAYS_ALL;

        const char *action = doc["action"];
        const char *name   = doc["name"];
        memset(task.action, 0, SMRT_SCHED_ACTION_LEN);
        memset(task.name, 0, SMRT_SCHED_NAME_LEN);
        if (action) strncpy(task.action, action, SMRT_SCHED_ACTION_LEN - 1);
        if (name)   strncpy(task.name, name, SMRT_SCHED_NAME_LEN - 1);

        if (smrt_sched_set_task(index, &task)) {
            resp["sched_result"] = true;
            resp["sched_msg"]    = "Tarea programada guardada";
        } else {
            resp["sched_result"] = false;
            resp["sched_msg"]    = "Error: datos invalidos";
        }
    }
    else if (strcmp(cmd, "sched_delete") == 0) {
        int index = doc["index"] | -1;
        if (smrt_sched_delete_task(index)) {
            resp["sched_result"] = true;
            resp["sched_msg"]    = "Tarea eliminada";
        } else {
            resp["sched_result"] = false;
            resp["sched_msg"]    = "Error: indice invalido";
        }
    }
    else if (strcmp(cmd, "sched_status") == 0) {
        resp["type"]        = "sched_status";
        resp["ntp_synced"]  = smrt_time_is_synced() ? true : false;
        resp["active"]      = smrt_sched_active_count();
        char time_buf[9];
        smrt_time_get_formatted(time_buf, sizeof(time_buf));
        resp["time"] = time_buf;
    }
    else if (strcmp(cmd, "time_set_tz") == 0) {
        long gmt = doc["gmt_offset"] | 0L;
        long dst = doc["dst_offset"] | 0L;
        if (smrt_time_validate_gmt_offset(gmt) && smrt_time_validate_dst_offset(dst)) {
            smrt_time_set_timezone(gmt, dst);
            smrt_time_save_timezone(gmt, dst);
            resp["time_result"] = true;
            resp["time_msg"]    = "Timezone actualizado";
        } else {
            resp["time_result"] = false;
            resp["time_msg"]    = "Valores de timezone invalidos";
        }
    }
    else {
        return; /* Unknown command */
    }

    String output;
    serializeJson(resp, output);
    smrt_ws.text(client_id, output);
}

/**
 * @brief  Adds scheduler telemetry to a JSON object
 */
void smrt_sched_get_telemetry(void *json_obj) {
    JsonObject &obj = *(JsonObject *)json_obj;
    obj["active_tasks"] = smrt_sched_active_count();
    obj["ntp_synced"]   = smrt_time_is_synced() ? true : false;

    char time_buf[9];
    smrt_time_get_formatted(time_buf, sizeof(time_buf));
    obj["time"] = String(time_buf);
}

#endif // UNIT_TEST
