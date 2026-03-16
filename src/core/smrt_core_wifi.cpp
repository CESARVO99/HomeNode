/**
 * @file    smrt_core_wifi.cpp
 * @brief   WiFi connection management with AP fallback and credential persistence
 * @project HOMENODE
 * @version 0.7.0
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
static char smrt_hostname[SMRT_MDNS_HOSTNAME_MAX] = {0};  /**< mDNS hostname */
static bool smrt_wifi_ap_active = false;            /**< True if AP fallback is active */

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
static void smrt_wifi_start_ap(void);

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes WiFi in STA mode with AP fallback.
 *         Attempts STA connection for SMRT_WIFI_STA_TIMEOUT_MS.
 *         If timeout expires, starts soft AP for reconfiguration via WebUI.
 * @return void
 */
void smrt_wifi_init(void) {
    smrt_wifi_ap_active = false;

    // Load WiFi credentials from NVS or use fallback
    if (!smrt_wifi_load_credentials(smrt_active_ssid, smrt_active_pass, SMRT_WIFI_SSID_MAX)) {
        strncpy(smrt_active_ssid, SMRT_WIFI_FALLBACK_SSID, SMRT_WIFI_SSID_MAX - 1);
        strncpy(smrt_active_pass, SMRT_WIFI_FALLBACK_PASS, SMRT_WIFI_PASS_MAX - 1);
        SMRT_DEBUG_LOG("NVS: No saved credentials, using fallback");
    } else {
        SMRT_DEBUG_LOG("NVS: Loaded WiFi credentials for: " + String(smrt_active_ssid));
    }

    // Load config PIN from NVS or use default
    if (!smrt_wifi_load_pin(smrt_config_pin, SMRT_WIFI_PIN_MAX)) {
        strncpy(smrt_config_pin, SMRT_WIFI_PIN_DEFAULT, SMRT_WIFI_PIN_MAX - 1);
        SMRT_DEBUG_LOG("NVS: No saved PIN, using default");
    } else {
        SMRT_DEBUG_LOG("NVS: Loaded config PIN");
    }

    // Load mDNS hostname from NVS or use default
    if (!smrt_nvs_get_string(SMRT_NVS_NAMESPACE, "mdns_host", smrt_hostname, SMRT_MDNS_HOSTNAME_MAX)) {
        strncpy(smrt_hostname, SMRT_OTA_HOSTNAME, SMRT_MDNS_HOSTNAME_MAX - 1);
    }

    // Configure static IP address
    IPAddress local_ip(SMRT_STATIC_IP);
    IPAddress gateway(SMRT_GATEWAY_IP);
    IPAddress subnet(SMRT_SUBNET_MASK);
    IPAddress dns(SMRT_DNS_IP);

    // Attempt STA connection with timeout
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(smrt_hostname);
    WiFi.disconnect();

    if (!WiFi.config(local_ip, gateway, subnet, dns)) {
        Serial.println("ERROR: Static IP configuration failed");
    }

    WiFi.begin(smrt_active_ssid, smrt_active_pass);
    SMRT_DEBUG_LOG("WiFi: Connecting to " + String(smrt_active_ssid) + "...");

    unsigned long start_ms = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start_ms >= SMRT_WIFI_STA_TIMEOUT_MS) {
            Serial.println("WiFi: STA timeout — starting AP fallback");
            smrt_wifi_start_ap();
            return;
        }
        delay(SMRT_WIFI_RETRY_MS);
        Serial.println("Connecting.. status: " + String(WiFi.status()));
    }

    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
}

/**
 * @brief  Starts soft AP for fallback configuration.
 *         Device is accessible at 192.168.4.1 via WebUI.
 * @return void
 */
static void smrt_wifi_start_ap(void) {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SMRT_WIFI_AP_SSID, SMRT_WIFI_AP_PASS, SMRT_WIFI_AP_CHANNEL);
    smrt_wifi_ap_active = true;

    Serial.println("WiFi AP active:");
    Serial.println("  SSID: " SMRT_WIFI_AP_SSID);
    Serial.println("  IP:   " + WiFi.softAPIP().toString());
    Serial.println("  Connect and open WebUI to configure WiFi credentials");
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
    SMRT_DEBUG_LOG("NVS: WiFi credentials saved");
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
    SMRT_DEBUG_LOG("NVS: Config PIN saved");
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

/**
 * @brief  Checks if WiFi is running in AP fallback mode.
 * @return true if AP mode active, false if STA connected
 */
bool smrt_wifi_is_ap_mode(void) {
    return smrt_wifi_ap_active;
}

/**
 * @brief  Returns the active mDNS hostname
 * @return Pointer to hostname buffer
 */
const char *smrt_wifi_get_hostname(void) {
    return smrt_hostname;
}

/**
 * @brief  Saves mDNS hostname to NVS
 * @param  hostname  Hostname string to store
 * @return void
 */
void smrt_wifi_save_hostname(const char *hostname) {
    smrt_nvs_set_string(SMRT_NVS_NAMESPACE, "mdns_host", hostname);
    SMRT_DEBUG_LOG("NVS: Hostname saved");
}

/**
 * @brief  Updates the in-memory mDNS hostname
 * @param  hostname  New hostname string
 * @return void
 */
void smrt_wifi_set_hostname(const char *hostname) {
    strncpy(smrt_hostname, hostname, SMRT_MDNS_HOSTNAME_MAX - 1);
    smrt_hostname[SMRT_MDNS_HOSTNAME_MAX - 1] = '\0';
}

#endif // UNIT_TEST
