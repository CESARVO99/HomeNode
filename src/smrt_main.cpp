/**
 * @file    smrt_main.cpp
 * @brief   HomeNode entry point — simplified setup/loop with modular architecture
 * @project HOMENODE
 * @version 1.1.0
 *
 * The main file is intentionally minimal. All functionality is delegated to
 * core subsystems and registered modules:
 *
 *   setup():
 *     1. smrt_register_modules()   — Registers modules based on NVS bitmask
 *     2. Core subsystem init       — GPIO, Serial, WiFi, HTTP (which inits WS + OTA)
 *     3. smrt_module_init_all()    — Calls init() on all registered modules
 *
 *   loop():
 *     1. ArduinoOTA.handle()       — Process OTA requests
 *     2. Serial debug read         — Echo serial input
 *     3. smrt_module_loop_all()    — Calls loop() on all registered modules
 *     4. smrt_ws cleanup           — Prune disconnected WS clients
 *     5. Telemetry broadcast       — Periodic status + module data
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "smrt_core.h"
#include <esp_task_wdt.h>

//-----------------------------------------------------------------------------
// External global objects (defined in smrt_core_http.cpp)
//-----------------------------------------------------------------------------
extern AsyncWebSocket smrt_ws;

//-----------------------------------------------------------------------------
// Telemetry timing
//-----------------------------------------------------------------------------
static unsigned long smrt_last_status_ms = 0;

//-----------------------------------------------------------------------------
// LED status blink (only when GPIO2 is not used by ACC lock relay)
//-----------------------------------------------------------------------------
static unsigned long smrt_led_last_ms = 0;

//-----------------------------------------------------------------------------
// Module descriptors (extern — all modules are compiled into the binary)
//-----------------------------------------------------------------------------
extern const smrt_module_t smrt_mod_env;
extern const smrt_module_t smrt_mod_rly;
extern const smrt_module_t smrt_mod_sec;
extern const smrt_module_t smrt_mod_plg;
extern const smrt_module_t smrt_mod_nrg;
extern const smrt_module_t smrt_mod_acc;

//-----------------------------------------------------------------------------
// Module registration (runtime selection from NVS bitmask)
//
// All 6 modules are compiled into the firmware. At boot, the NVS bitmask
// determines which ones are registered and active. This allows a single
// firmware binary to work for any module combination.
//
// Configure via WebUI "Nodo" card or WS command node_set_modules.
//-----------------------------------------------------------------------------

/**
 * @brief  Registers modules based on the NVS bitmask.
 *         Must be called AFTER smrt_node_init() (which is called in smrt_http_init).
 *         However, node_init needs NVS which needs to be ready.
 *         We read the bitmask directly from NVS here since node_init
 *         happens later in smrt_http_init.
 * @return void
 */
static void smrt_register_modules(void) {
    /* Read module bitmask from NVS (same logic as smrt_node_init) */
    int32_t mask_val = 0;
    smrt_nvs_get_int(SMRT_NODE_NVS_NAMESPACE, "modules", &mask_val, SMRT_NODE_MOD_ENV);
    uint8_t mask = (uint8_t)(mask_val & SMRT_NODE_MOD_ALL);

    /* Validate — if conflicts detected, fall back to ENV only */
    if (smrt_node_validate_modules(mask) != 0) {
        Serial.println("[MAIN] Module conflict detected! Falling back to ENV only.");
        mask = SMRT_NODE_MOD_ENV;
    }

    if (mask & SMRT_NODE_MOD_ENV) smrt_module_register(&smrt_mod_env);
    if (mask & SMRT_NODE_MOD_RLY) smrt_module_register(&smrt_mod_rly);
    if (mask & SMRT_NODE_MOD_SEC) smrt_module_register(&smrt_mod_sec);
    if (mask & SMRT_NODE_MOD_PLG) smrt_module_register(&smrt_mod_plg);
    if (mask & SMRT_NODE_MOD_NRG) smrt_module_register(&smrt_mod_nrg);
    if (mask & SMRT_NODE_MOD_ACC) smrt_module_register(&smrt_mod_acc);

    char mod_str[32];
    smrt_node_modules_to_string(mask, mod_str, sizeof(mod_str));
    Serial.printf("Modules registered: %d [%s]\n", smrt_module_count(), mod_str);
}

//-----------------------------------------------------------------------------
// Arduino entry points
//-----------------------------------------------------------------------------

/**
 * @brief  System initialization.
 *         Initializes all core subsystems and registered modules.
 * @return void
 */
void setup() {
    // HAL init
    smrt_gpio_init();
    smrt_serial_init();

    Serial.println("========================================");
    Serial.println(" HOMENODE v" SMRT_PLATFORM_VERSION);
    Serial.println(" IoT Modular Platform");
    Serial.println("========================================");

    // Register modules (reads NVS bitmask — before WiFi/HTTP so they can hook events)
    smrt_register_modules();

    // Core services init
    smrt_wifi_init();
    smrt_http_init();

    // Module init (after core is ready)
    smrt_module_init_all();

    // Watchdog — auto-reset if loop hangs for SMRT_WDT_TIMEOUT_S seconds
    esp_task_wdt_init(SMRT_WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    Serial.println("Setup complete. Entering main loop.");
}

/**
 * @brief  Main loop — handles OTA, serial, modules, WS cleanup, telemetry.
 * @return void
 */
void loop() {
    // Feed watchdog
    esp_task_wdt_reset();

    // OTA
    ArduinoOTA.handle();

    // Serial debug
    String serial_value = smrt_serial_read();
    if (serial_value != "") {
        Serial.println("Serial received: " + serial_value);
    }

    // Module loop
    smrt_module_loop_all();

    // Scheduler loop
    #ifdef SMRT_SCHED
    smrt_sched_loop();
    #endif

    // MQTT loop (reconnect + process messages + discovery)
    #ifdef SMRT_MQTT
    smrt_mqtt_loop();
    #endif

    // WebSocket cleanup + session timeout
    smrt_ws.cleanupClients();
    smrt_auth_ws_cleanup_expired();

    // Periodic telemetry broadcast
    unsigned long now = millis();
    if (now - smrt_last_status_ms >= SMRT_STATUS_INTERVAL_MS) {
        smrt_last_status_ms = now;
        smrt_ws_send_status();
    }

    // LED status blink (GPIO2 only if ACC module is not active — it uses GPIO2 for lock)
    if (!smrt_node_has_module(SMRT_NODE_MOD_ACC)) {
        unsigned long led_interval = smrt_wifi_is_ap_mode()
                                     ? SMRT_LED_BLINK_AP_MS
                                     : SMRT_LED_BLINK_NORMAL_MS;
        if (now - smrt_led_last_ms >= led_interval) {
            smrt_led_last_ms = now;
            smrt_gpio_toggle_state(SMRT_LED_BUILTIN);
        }
    }

    delay(SMRT_LOOP_DELAY_MS);
}
