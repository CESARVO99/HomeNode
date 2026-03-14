/**
 * @file    smrt_core_nvs.h
 * @brief   Generic NVS (Non-Volatile Storage) persistence API
 * @project HOMENODE
 * @version 0.2.0
 *
 * Provides get/set operations for strings, integers and booleans
 * with a configurable namespace. Each module can store its config
 * under its own namespace prefix.
 */

#ifndef SMRT_CORE_NVS_H
#define SMRT_CORE_NVS_H

#ifndef UNIT_TEST

#include <Preferences.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Saves a string value to NVS
 * @param  ns     Namespace (e.g. "smrt_cfg", "mod_env")
 * @param  key    Key name (max 15 chars)
 * @param  value  String value to store
 * @return void
 */
void smrt_nvs_set_string(const char *ns, const char *key, const char *value);

/**
 * @brief  Loads a string value from NVS
 * @param  ns       Namespace
 * @param  key      Key name
 * @param  buf      Buffer to store the loaded string
 * @param  buf_len  Maximum buffer size
 * @return true if key was found and loaded, false otherwise
 */
bool smrt_nvs_get_string(const char *ns, const char *key, char *buf, size_t buf_len);

/**
 * @brief  Saves an integer value to NVS
 * @param  ns     Namespace
 * @param  key    Key name
 * @param  value  Integer value to store
 * @return void
 */
void smrt_nvs_set_int(const char *ns, const char *key, int32_t value);

/**
 * @brief  Loads an integer value from NVS
 * @param  ns           Namespace
 * @param  key          Key name
 * @param  out_value    Pointer to store the loaded value
 * @param  default_val  Default value if key not found
 * @return true if key was found, false if default was used
 */
bool smrt_nvs_get_int(const char *ns, const char *key, int32_t *out_value, int32_t default_val);

/**
 * @brief  Saves a boolean value to NVS
 * @param  ns     Namespace
 * @param  key    Key name
 * @param  value  Boolean value to store
 * @return void
 */
void smrt_nvs_set_bool(const char *ns, const char *key, bool value);

/**
 * @brief  Loads a boolean value from NVS
 * @param  ns           Namespace
 * @param  key          Key name
 * @param  out_value    Pointer to store the loaded value
 * @param  default_val  Default value if key not found
 * @return true if key was found, false if default was used
 */
bool smrt_nvs_get_bool(const char *ns, const char *key, bool *out_value, bool default_val);

/**
 * @brief  Removes a key from NVS
 * @param  ns   Namespace
 * @param  key  Key name to remove
 * @return void
 */
void smrt_nvs_remove(const char *ns, const char *key);

/**
 * @brief  Clears all keys in a namespace
 * @param  ns  Namespace to clear
 * @return void
 */
void smrt_nvs_clear(const char *ns);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST

#endif // SMRT_CORE_NVS_H
