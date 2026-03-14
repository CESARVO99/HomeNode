/**
 * @file    smrt_core_nvs.cpp
 * @brief   Generic NVS persistence implementation using ESP32 Preferences API
 * @project HOMENODE
 * @version 0.2.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// Static Preferences instance (shared, opened/closed per operation)
//-----------------------------------------------------------------------------
static Preferences smrt_prefs;

//-----------------------------------------------------------------------------
// String operations
//-----------------------------------------------------------------------------

/**
 * @brief  Saves a string value to NVS
 * @param  ns     Namespace
 * @param  key    Key name
 * @param  value  String value to store
 * @return void
 */
void smrt_nvs_set_string(const char *ns, const char *key, const char *value) {
    smrt_prefs.begin(ns, false);
    smrt_prefs.putString(key, value);
    smrt_prefs.end();
}

/**
 * @brief  Loads a string value from NVS
 * @param  ns       Namespace
 * @param  key      Key name
 * @param  buf      Buffer to store the loaded string
 * @param  buf_len  Maximum buffer size
 * @return true if key was found and non-empty, false otherwise
 */
bool smrt_nvs_get_string(const char *ns, const char *key, char *buf, size_t buf_len) {
    smrt_prefs.begin(ns, true);
    String stored = smrt_prefs.getString(key, "");
    smrt_prefs.end();

    if (stored.length() == 0) {
        return false;
    }

    strncpy(buf, stored.c_str(), buf_len - 1);
    buf[buf_len - 1] = '\0';
    return true;
}

//-----------------------------------------------------------------------------
// Integer operations
//-----------------------------------------------------------------------------

/**
 * @brief  Saves an integer value to NVS
 * @param  ns     Namespace
 * @param  key    Key name
 * @param  value  Integer value to store
 * @return void
 */
void smrt_nvs_set_int(const char *ns, const char *key, int32_t value) {
    smrt_prefs.begin(ns, false);
    smrt_prefs.putInt(key, value);
    smrt_prefs.end();
}

/**
 * @brief  Loads an integer value from NVS
 * @param  ns           Namespace
 * @param  key          Key name
 * @param  out_value    Pointer to store the loaded value
 * @param  default_val  Default value if key not found
 * @return true if key was found, false if default was used
 */
bool smrt_nvs_get_int(const char *ns, const char *key, int32_t *out_value, int32_t default_val) {
    smrt_prefs.begin(ns, true);
    bool exists = smrt_prefs.isKey(key);
    if (exists) {
        *out_value = smrt_prefs.getInt(key, default_val);
    } else {
        *out_value = default_val;
    }
    smrt_prefs.end();
    return exists;
}

//-----------------------------------------------------------------------------
// Boolean operations
//-----------------------------------------------------------------------------

/**
 * @brief  Saves a boolean value to NVS
 * @param  ns     Namespace
 * @param  key    Key name
 * @param  value  Boolean value to store
 * @return void
 */
void smrt_nvs_set_bool(const char *ns, const char *key, bool value) {
    smrt_prefs.begin(ns, false);
    smrt_prefs.putBool(key, value);
    smrt_prefs.end();
}

/**
 * @brief  Loads a boolean value from NVS
 * @param  ns           Namespace
 * @param  key          Key name
 * @param  out_value    Pointer to store the loaded value
 * @param  default_val  Default value if key not found
 * @return true if key was found, false if default was used
 */
bool smrt_nvs_get_bool(const char *ns, const char *key, bool *out_value, bool default_val) {
    smrt_prefs.begin(ns, true);
    bool exists = smrt_prefs.isKey(key);
    if (exists) {
        *out_value = smrt_prefs.getBool(key, default_val);
    } else {
        *out_value = default_val;
    }
    smrt_prefs.end();
    return exists;
}

//-----------------------------------------------------------------------------
// Key management
//-----------------------------------------------------------------------------

/**
 * @brief  Removes a key from NVS
 * @param  ns   Namespace
 * @param  key  Key name to remove
 * @return void
 */
void smrt_nvs_remove(const char *ns, const char *key) {
    smrt_prefs.begin(ns, false);
    smrt_prefs.remove(key);
    smrt_prefs.end();
}

/**
 * @brief  Clears all keys in a namespace
 * @param  ns  Namespace to clear
 * @return void
 */
void smrt_nvs_clear(const char *ns) {
    smrt_prefs.begin(ns, false);
    smrt_prefs.clear();
    smrt_prefs.end();
}

#endif // UNIT_TEST
