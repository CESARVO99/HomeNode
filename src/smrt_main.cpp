/**
 * @file    smrt_main.cpp
 * @brief   HomeNode entry point — simplified setup/loop with modular architecture
 * @project HOMENODE
 * @version 0.5.0
 *
 * The main file is intentionally minimal. All functionality is delegated to
 * core subsystems and registered modules:
 *
 *   setup():
 *     1. smrt_register_modules()   — Conditionally registers modules via #ifdef
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

//-----------------------------------------------------------------------------
// External global objects (defined in smrt_core_http.cpp)
//-----------------------------------------------------------------------------
extern AsyncWebSocket smrt_ws;

//-----------------------------------------------------------------------------
// Telemetry timing
//-----------------------------------------------------------------------------
static unsigned long smrt_last_status_ms = 0;

//-----------------------------------------------------------------------------
// Module registration (conditional compilation)
//
// To enable a module, add its build flag in platformio.ini:
//   -D SMRT_MOD_ENV    (environmental sensors)
//   -D SMRT_MOD_SEC    (security / NFC)
//   -D SMRT_MOD_RLY    (relay control)
//   -D SMRT_MOD_PLG    (smart plug / energy)
//   -D SMRT_MOD_ACC    (access control)
//   -D SMRT_MOD_NRG    (energy monitoring)
//-----------------------------------------------------------------------------

/**
 * @brief  Registers all enabled modules.
 *         Called once before core init in setup().
 * @return void
 */
static void smrt_register_modules(void) {
    #ifdef SMRT_MOD_ENV
        extern const smrt_module_t smrt_mod_env;
        smrt_module_register(&smrt_mod_env);
    #endif
    #ifdef SMRT_MOD_RLY
        extern const smrt_module_t smrt_mod_rly;
        smrt_module_register(&smrt_mod_rly);
    #endif
    #ifdef SMRT_MOD_SEC
        extern const smrt_module_t smrt_mod_sec;
        smrt_module_register(&smrt_mod_sec);
    #endif
    #ifdef SMRT_MOD_PLG
        extern const smrt_module_t smrt_mod_plg;
        smrt_module_register(&smrt_mod_plg);
    #endif
    #ifdef SMRT_MOD_NRG
        extern const smrt_module_t smrt_mod_nrg;
        smrt_module_register(&smrt_mod_nrg);
    #endif
    #ifdef SMRT_MOD_ACC
        extern const smrt_module_t smrt_mod_acc;
        smrt_module_register(&smrt_mod_acc);
    #endif

    Serial.println("Modules registered: " + String(smrt_module_count()));
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

    // Register modules (before WiFi/HTTP so they can hook events)
    smrt_register_modules();

    // Core services init
    smrt_wifi_init();
    smrt_http_init();

    // Module init (after core is ready)
    smrt_module_init_all();

    Serial.println("Setup complete. Entering main loop.");
}

/**
 * @brief  Main loop — handles OTA, serial, modules, WS cleanup, telemetry.
 * @return void
 */
void loop() {
    // OTA
    ArduinoOTA.handle();

    // Serial debug
    String serial_value = smrt_serial_read();
    if (serial_value != "") {
        Serial.println("Serial received: " + serial_value);
    }

    // Module loop
    smrt_module_loop_all();

    // WebSocket cleanup
    smrt_ws.cleanupClients();

    // Periodic telemetry broadcast
    unsigned long now = millis();
    if (now - smrt_last_status_ms >= SMRT_STATUS_INTERVAL_MS) {
        smrt_last_status_ms = now;
        smrt_ws_send_status();
    }

    delay(SMRT_LOOP_DELAY_MS);
}
