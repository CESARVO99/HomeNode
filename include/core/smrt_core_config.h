/**
 * @file    smrt_core_config.h
 * @brief   Platform configuration defines for HomeNode
 * @project HOMENODE
 * @version 0.8.0
 *
 * All compile-time constants for the HomeNode platform are centralized here.
 * Modules should NOT define their own network/timing constants — use these.
 */

#ifndef SMRT_CORE_CONFIG_H
#define SMRT_CORE_CONFIG_H

//-----------------------------------------------------------------------------
// Platform identity
//-----------------------------------------------------------------------------
#define SMRT_PLATFORM_NAME          "HomeNode"      /**< Platform display name */
#define SMRT_PLATFORM_VERSION       "0.8.0"         /**< Platform firmware version */

//-----------------------------------------------------------------------------
// Network — HTTP / WebSocket
//-----------------------------------------------------------------------------
#define SMRT_WEB_SERVER_PORT        80              /**< HTTP server port */
#define SMRT_WS_PATH                "/ws"           /**< WebSocket endpoint path */

//-----------------------------------------------------------------------------
// Network — Static IP configuration
// Change these values to match your local network
//-----------------------------------------------------------------------------
#define SMRT_STATIC_IP              192, 168, 1, 100    /**< ESP32 static IP */
#define SMRT_GATEWAY_IP             192, 168, 1, 1      /**< Router/gateway IP */
#define SMRT_SUBNET_MASK            255, 255, 255, 0    /**< Subnet mask */
#define SMRT_DNS_IP                 8, 8, 8, 8          /**< DNS (Google DNS) */

//-----------------------------------------------------------------------------
// WiFi
//-----------------------------------------------------------------------------
#define SMRT_WIFI_RETRY_MS          1000            /**< WiFi reconnect interval (ms) */
#define SMRT_WIFI_SSID_MAX          33              /**< Max SSID length (32+null) */
#define SMRT_WIFI_PASS_MAX          65              /**< Max password length (64+null) */
#define SMRT_WIFI_PIN_MAX           9               /**< Max config PIN length (8+null) */
#define SMRT_WIFI_PIN_DEFAULT       "1234"          /**< Default config access PIN */
#define SMRT_WIFI_STA_TIMEOUT_MS    15000           /**< Max wait for STA connection (15s) */
#define SMRT_WIFI_AP_SSID           "HomeNode-Setup" /**< Soft AP SSID for fallback */
#define SMRT_WIFI_AP_PASS           "homenode123"   /**< Soft AP password (min 8 chars) */
#define SMRT_WIFI_AP_CHANNEL        1               /**< Soft AP WiFi channel */

//-----------------------------------------------------------------------------
// NVS (Non-Volatile Storage)
//-----------------------------------------------------------------------------
#define SMRT_NVS_NAMESPACE          "smrt_cfg"      /**< Default NVS namespace */

//-----------------------------------------------------------------------------
// OTA (Over-The-Air updates)
//-----------------------------------------------------------------------------
#define SMRT_OTA_HOSTNAME           "homenode"      /**< mDNS hostname for OTA (default) */
#define SMRT_OTA_PORT               3232            /**< OTA upload port */
#define SMRT_OTA_PASSWORD           "hN!0t4$ecUr3"  /**< OTA password (change for each device) */
#define SMRT_MDNS_HOSTNAME_MAX      33              /**< Max hostname length (32+null) */

//-----------------------------------------------------------------------------
// Security — Authentication & rate limiting
//-----------------------------------------------------------------------------
#define SMRT_AUTH_MAX_WS_CLIENTS    8               /**< Max tracked WS client slots */
#define SMRT_AUTH_PIN_MAX_ATTEMPTS  3               /**< Failed PIN attempts before lockout */
#define SMRT_AUTH_PIN_LOCKOUT_MS    60000            /**< PIN lockout duration (60s) */
#define SMRT_AUTH_SESSION_TIMEOUT_MS 600000          /**< WS session inactivity timeout (10 min) */
#define SMRT_AUTH_OTA_USER          "admin"          /**< HTTP Basic Auth user for /update */
#define SMRT_AUTH_OTA_PASS          "hN!0t4$ecUr3"  /**< HTTP Basic Auth password for /update */

//-----------------------------------------------------------------------------
// NVS write throttling
//-----------------------------------------------------------------------------
#define SMRT_NVS_WRITE_INTERVAL_MS  300000          /**< Min interval between NVS writes (5 min) */
#define SMRT_NVS_STATE_DEBOUNCE_MS  5000            /**< Min interval for state NVS writes (5s) */

//-----------------------------------------------------------------------------
// Timing
//-----------------------------------------------------------------------------
#define SMRT_LOOP_DELAY_MS          20              /**< Main loop delay (ms) */
#define SMRT_STATUS_INTERVAL_MS     5000            /**< Telemetry broadcast interval (ms) */
#define SMRT_RELAY_PULSE_MS         3000            /**< Relay activation pulse (ms) */

//-----------------------------------------------------------------------------
// Module system
//-----------------------------------------------------------------------------
#define SMRT_MAX_MODULES            8               /**< Maximum registered modules */

//-----------------------------------------------------------------------------
// Watchdog
//-----------------------------------------------------------------------------
#define SMRT_WDT_TIMEOUT_S          30              /**< Task watchdog timeout (seconds) */

//-----------------------------------------------------------------------------
// Serial
//-----------------------------------------------------------------------------
#define SMRT_SERIAL_BAUDRATE        115200          /**< Default serial baudrate */
#define SMRT_SERIAL_TIMEOUT         20              /**< Serial read timeout (ms) */
#define SMRT_SERIAL_BUFF_SIZE       20              /**< Serial buffer size (bytes) */

//-----------------------------------------------------------------------------
// Debug logging — only active when -D SMRT_DEBUG is set in build flags
//-----------------------------------------------------------------------------
#ifdef SMRT_DEBUG
    #define SMRT_DEBUG_LOG(...)     Serial.println(__VA_ARGS__)
    #define SMRT_DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
    #define SMRT_DEBUG_LOG(...)     ((void)0)
    #define SMRT_DEBUG_PRINTF(...)  ((void)0)
#endif

//-----------------------------------------------------------------------------
// GPIO — Built-in LED (generic, no module-specific pins here)
//-----------------------------------------------------------------------------
#define SMRT_LED_BUILTIN            2               /**< Built-in LED pin (GPIO2) */
#define SMRT_LED_BLINK_NORMAL_MS    5000            /**< LED blink interval — normal operation */
#define SMRT_LED_BLINK_AP_MS        500             /**< LED blink interval — AP fallback mode */

//-----------------------------------------------------------------------------
// I2C bus defaults
//-----------------------------------------------------------------------------
#define SMRT_I2C_SDA                21              /**< I2C data line (GPIO21) */
#define SMRT_I2C_SCL                22              /**< I2C clock line (GPIO22) */

//-----------------------------------------------------------------------------
// SPI bus (internal flash — DO NOT USE)
//-----------------------------------------------------------------------------
#define SMRT_SPI_SCK                6               /**< SPI clock (GPIO6) */
#define SMRT_SPI_SDO                7               /**< SPI data out (GPIO7) */
#define SMRT_SPI_SDI                8               /**< SPI data in (GPIO8) */
#define SMRT_SPI_SHD                9               /**< SPI hold (GPIO9) */
#define SMRT_SPI_SWP                10              /**< SPI write protect (GPIO10) */
#define SMRT_SPI_CSC                11              /**< SPI chip select (GPIO11) */

//-----------------------------------------------------------------------------
// UART pins
//-----------------------------------------------------------------------------
#define SMRT_TXD_232                1               /**< UART TX pin (GPIO1) */
#define SMRT_RXD_232                3               /**< UART RX pin (GPIO3) */

//-----------------------------------------------------------------------------
// Analog inputs (ADC — input only, no pullup)
//-----------------------------------------------------------------------------
#define SMRT_ADC_IN1                34              /**< Analog input 1 (GPIO34) */
#define SMRT_ADC_IN2                35              /**< Analog input 2 (GPIO35) */
#define SMRT_ADC_IN3                36              /**< Analog input 3 (GPIO36) */
#define SMRT_ADC_IN4                39              /**< Analog input 4 (GPIO39) */

//-----------------------------------------------------------------------------
// General purpose I/O pins (available for modules)
//-----------------------------------------------------------------------------
#define SMRT_GPIO_IO1               16              /**< GP I/O 1 (GPIO16) */
#define SMRT_GPIO_IO2               17              /**< GP I/O 2 (GPIO17) */
#define SMRT_GPIO_IO3               18              /**< GP I/O 3 (GPIO18) */
#define SMRT_GPIO_IO4               19              /**< GP I/O 4 (GPIO19) */
#define SMRT_GPIO_IO5               23              /**< GP I/O 5 (GPIO23) */
#define SMRT_GPIO_IO6               25              /**< GP I/O 6 (GPIO25) */
#define SMRT_GPIO_IO7               26              /**< GP I/O 7 (GPIO26) */
#define SMRT_GPIO_IO8               27              /**< GP I/O 8 (GPIO27) */

//-----------------------------------------------------------------------------
// Capacitive touch inputs
//-----------------------------------------------------------------------------
#define SMRT_CAP_IN0                4               /**< Touch input 0 (GPIO4) */
#define SMRT_CAP_IN1                0               /**< Touch input 1 (GPIO0) */
#define SMRT_CAP_IN3                15              /**< Touch input 3 (GPIO15) */
#define SMRT_CAP_IN4                13              /**< Touch input 4 (GPIO13) */
#define SMRT_CAP_IN5                12              /**< Touch input 5 (GPIO12) */
#define SMRT_CAP_IN6                14              /**< Touch input 6 (GPIO14) */
#define SMRT_CAP_IN8                33              /**< Touch input 8 (GPIO33) */
#define SMRT_CAP_IN9                32              /**< Touch input 9 (GPIO32) */

#endif // SMRT_CORE_CONFIG_H
