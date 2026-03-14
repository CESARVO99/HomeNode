/**
 * @file    smrt_core_wifi.h
 * @brief   WiFi management — connect, reconnect, credential storage
 * @project HOMENODE
 * @version 0.2.0
 */

#ifndef SMRT_CORE_WIFI_H
#define SMRT_CORE_WIFI_H

#ifndef UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes WiFi in STA mode with static IP and connects.
 *         Loads credentials from NVS; uses fallback if none saved.
 * @return void
 */
void smrt_wifi_init(void);

/**
 * @brief  Saves WiFi credentials to NVS
 * @param  ssid  SSID string to store
 * @param  pass  Password string to store
 * @return void
 */
void smrt_wifi_save_credentials(const char *ssid, const char *pass);

/**
 * @brief  Loads WiFi credentials from NVS
 * @param  ssid     Buffer to store the loaded SSID
 * @param  pass     Buffer to store the loaded password
 * @param  max_len  Maximum buffer size
 * @return true if credentials were found, false otherwise
 */
bool smrt_wifi_load_credentials(char *ssid, char *pass, size_t max_len);

/**
 * @brief  Saves the WiFi config access PIN to NVS
 * @param  pin  PIN string to store
 * @return void
 */
void smrt_wifi_save_pin(const char *pin);

/**
 * @brief  Loads the WiFi config access PIN from NVS
 * @param  pin      Buffer to store the loaded PIN
 * @param  max_len  Maximum buffer size
 * @return true if a PIN was found, false otherwise
 */
bool smrt_wifi_load_pin(char *pin, size_t max_len);

/**
 * @brief  Returns the active SSID string
 * @return Pointer to active SSID buffer
 */
const char *smrt_wifi_get_ssid(void);

/**
 * @brief  Returns the config access PIN string
 * @return Pointer to config PIN buffer
 */
const char *smrt_wifi_get_pin(void);

/**
 * @brief  Updates the in-memory config PIN
 * @param  new_pin  New PIN string
 * @return void
 */
void smrt_wifi_set_pin(const char *new_pin);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST

#endif // SMRT_CORE_WIFI_H
