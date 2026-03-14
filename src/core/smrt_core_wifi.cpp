/**
 * @file    smrt_core_wifi.cpp
 * @brief   WiFi connection management and credential persistence
 * @project HOMENODE
 * @version 0.2.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// WiFi fallback credentials (used if NVS is empty)
//-----------------------------------------------------------------------------
static const char *SMRT_WIFI_FALLBACK_SSID = "CHANGE_ME_SSID";
static const char *SMRT_WIFI_FALLBACK_PASS = "CHANGE_ME_PASS";

//-----------------------------------------------------------------------------
// Runtime state
//-----------------------------------------------------------------------------
static char smrt_active_ssid[SMRT_WIFI_SSID_MAX] = {0};
static char smrt_active_pass[SMRT_WIFI_PASS_MAX] = {0};
static char smrt_config_pin[SMRT_WIFI_PIN_MAX]   = {0};

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes WiFi in STA mode with static IP and connects.
 *         Loads credentials from NVS; uses fallback if none saved.
 * @return void
 */
void smrt_wifi_init(void) {
    // Load WiFi credentials from NVS or use fallback
    if (!smrt_wifi_load_credentials(smrt_active_ssid, smrt_active_pass, SMRT_WIFI_SSID_MAX)) {
        strncpy(smrt_active_ssid, SMRT_WIFI_FALLBACK_SSID, SMRT_WIFI_SSID_MAX - 1);
        strncpy(smrt_active_pass, SMRT_WIFI_FALLBACK_PASS, SMRT_WIFI_PASS_MAX - 1);
        Serial.println("NVS: No saved credentials, using fallback");
    } else {
        Serial.println("NVS: Loaded WiFi credentials for: " + String(smrt_active_ssid));
    }

    // Load config PIN from NVS or use default
    if (!smrt_wifi_load_pin(smrt_config_pin, SMRT_WIFI_PIN_MAX)) {
        strncpy(smrt_config_pin, SMRT_WIFI_PIN_DEFAULT, SMRT_WIFI_PIN_MAX - 1);
        Serial.println("NVS: No saved PIN, using default");
    } else {
        Serial.println("NVS: Loaded config PIN");
    }

    // Configure static IP address
    IPAddress local_ip(SMRT_STATIC_IP);
    IPAddress gateway(SMRT_GATEWAY_IP);
    IPAddress subnet(SMRT_SUBNET_MASK);
    IPAddress dns(SMRT_DNS_IP);

    // WiFi station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (!WiFi.config(local_ip, gateway, subnet, dns)) {
        Serial.println("ERROR: Static IP configuration failed");
    }

    WiFi.begin(smrt_active_ssid, smrt_active_pass);
    delay(SMRT_WIFI_RETRY_MS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(SMRT_WIFI_RETRY_MS);
        Serial.println("Connecting.. status: " + String(WiFi.status()));
    }

    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
}

/**
 * @brief  Saves WiFi credentials to NVS
 * @param  ssid  SSID string to store
 * @param  pass  Password string to store
 * @return void
 */
void smrt_wifi_save_credentials(const char *ssid, const char *pass) {
    smrt_nvs_set_string(SMRT_NVS_NAMESPACE, "wifi_ssid", ssid);
    smrt_nvs_set_string(SMRT_NVS_NAMESPACE, "wifi_pass", pass);
    Serial.println("NVS: WiFi credentials saved");
}

/**
 * @brief  Loads WiFi credentials from NVS
 * @param  ssid     Buffer to store the loaded SSID
 * @param  pass     Buffer to store the loaded password
 * @param  max_len  Maximum buffer size
 * @return true if credentials were found, false otherwise
 */
bool smrt_wifi_load_credentials(char *ssid, char *pass, size_t max_len) {
    if (!smrt_nvs_get_string(SMRT_NVS_NAMESPACE, "wifi_ssid", ssid, max_len)) {
        return false;
    }
    smrt_nvs_get_string(SMRT_NVS_NAMESPACE, "wifi_pass", pass, max_len);
    return true;
}

/**
 * @brief  Saves the WiFi config access PIN to NVS
 * @param  pin  PIN string to store
 * @return void
 */
void smrt_wifi_save_pin(const char *pin) {
    smrt_nvs_set_string(SMRT_NVS_NAMESPACE, "cfg_pin", pin);
    Serial.println("NVS: Config PIN saved");
}

/**
 * @brief  Loads the WiFi config access PIN from NVS
 * @param  pin      Buffer to store the loaded PIN
 * @param  max_len  Maximum buffer size
 * @return true if a PIN was found, false otherwise
 */
bool smrt_wifi_load_pin(char *pin, size_t max_len) {
    return smrt_nvs_get_string(SMRT_NVS_NAMESPACE, "cfg_pin", pin, max_len);
}

/**
 * @brief  Returns the active SSID string
 * @return Pointer to active SSID buffer
 */
const char *smrt_wifi_get_ssid(void) {
    return smrt_active_ssid;
}

/**
 * @brief  Returns the config access PIN string
 * @return Pointer to config PIN buffer
 */
const char *smrt_wifi_get_pin(void) {
    return smrt_config_pin;
}

/**
 * @brief  Updates the in-memory config PIN
 * @param  new_pin  New PIN string
 * @return void
 */
void smrt_wifi_set_pin(const char *new_pin) {
    strncpy(smrt_config_pin, new_pin, SMRT_WIFI_PIN_MAX - 1);
    smrt_config_pin[SMRT_WIFI_PIN_MAX - 1] = '\0';
}

#endif // UNIT_TEST
