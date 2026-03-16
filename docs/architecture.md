# HomeNode — Firmware Architecture

> **Project:** HomeNode — Modular Domestic IoT Platform
> **Version:** 0.7.0
> **Date:** 2026-03-16
> **Phase:** FASE 7 — Security P2 + Features + Documentation

---

## 1. Layer Diagram

```
+=====================================================================+
|                     LAYER 3: APPLICATION MODULES                     |
|  (Conditional compilation via #ifdef SMRT_MOD_xxx in platformio.ini) |
|                                                                      |
|  smrt_mod_env  | smrt_mod_sec  | smrt_mod_rly  | smrt_mod_plg  ... |
|  Environmental | Security/NFC  | Relay Control | Smart Plug         |
|                                                                      |
|  Each module implements the smrt_module_t interface:                  |
|    init()  loop()  ws_handler()  get_telemetry()                     |
+======================================================================+
         |              |               |              |
         v              v               v              v
         +--- smrt_module_register(&smrt_mod_xxx) -----+
         |                                              |
+======================================================================+
|                   LAYER 2: CORE PLATFORM SERVICES                    |
|                                                                      |
|  smrt_core_module  Module registry, dispatch, lifecycle              |
|  smrt_core_wifi    WiFi STA + AP fallback, credentials (NVS)        |
|  smrt_core_ws      WebSocket server, JSON protocol, telemetry       |
|  smrt_core_http    AsyncWebServer + route registration              |
|  smrt_core_ota     ArduinoOTA + HTTP firmware upload (auth)         |
|  smrt_core_auth    WS session auth, PIN rate limiting, timeout      |
|  smrt_core_nvs     Generic NVS persistence (Preferences API)        |
|  smrt_core_config  All #define constants (pins, timings, sizes)     |
|  smrt_core_webui   Embedded HTML/CSS/JS dashboard (PROGMEM)         |
+======================================================================+
         |              |               |
         v              v               v
+======================================================================+
|                LAYER 1: HARDWARE ABSTRACTION LAYER                   |
|                   (Reused from Smart_Lock v1.3.0)                    |
|                                                                      |
|  smrt_mc_format   Custom types (uint8..uint32, bit), conversions    |
|  smrt_mc_serial   UART initialization and read                      |
|  smrt_mc_gpio     Digital I/O: set, clear, toggle                   |
+======================================================================+
         |
         v
+======================================================================+
|                       HARDWARE PLATFORM                              |
|              ESP32-WROOM (NodeMCU-32S, 4MB Flash)                    |
|              Arduino Framework, PlatformIO Build System               |
+======================================================================+
```

---

## 2. Module Interface (smrt_module_t)

Every application module is described by a single struct:

```c
typedef struct {
    const char *id;           // Short identifier: "env", "sec", "rly"
    const char *name;         // Human-readable: "Environmental", "Security"
    const char *version;      // Semantic version: "1.0.0"

    void (*init)(void);                                          // Called once at startup
    void (*loop)(void);                                          // Called every main loop iteration
    void (*ws_handler)(const char *cmd, void *doc, void *client);// WebSocket command handler
    void (*get_telemetry)(void *data);                           // Fills telemetry JSON object
} smrt_module_t;
```

### Design Decisions

| Decision | Rationale |
|----------|-----------|
| C-style struct + function pointers | Simpler than C++ classes, no vtable overhead, compatible with C linkage |
| `void *` for doc/client parameters | Avoids ArduinoJson/AsyncWebSocket dependency in native tests |
| Static array registry (no malloc) | Deterministic memory, no fragmentation on embedded target |
| `SMRT_MAX_MODULES = 8` | Sufficient for foreseeable modules, easily adjustable |
| NULL callback tolerance | All lifecycle functions skip NULL pointers safely |

---

## 3. Module Registry API

```
smrt_module_register(mod)     Register a module descriptor (returns 1=ok, 0=fail)
smrt_module_count()           Number of registered modules
smrt_module_get(index)        Get module by index (bounds-checked)
smrt_module_find(id)          Find module by string id
smrt_module_init_all()        Call init() on all registered modules
smrt_module_loop_all()        Call loop() on all registered modules
smrt_module_dispatch(cmd,..)  Route a prefixed command to the correct module
smrt_module_get_telemetry_all(data)  Aggregate telemetry from all modules
smrt_module_reset()           [TEST ONLY] Clear registry for test isolation
```

---

## 4. Command Dispatch (Prefix Stripping)

WebSocket commands follow the pattern `<module_id>_<action>`. The dispatch
engine automatically strips the module prefix before forwarding:

```
Client sends:  "env_read"
                  |
                  v
          smrt_module_dispatch("env_read", doc, client)
                  |
       +----------+----------+
       | For each registered  |
       | module, check if cmd |
       | starts with id + "_" |
       +----------+----------+
                  |
          Match: module "env"
                  |
                  v
       mod_env.ws_handler("read", doc, client)
                          ^^^^^^
                   Prefix stripped!
```

### Dispatch Rules

1. Command must contain `_` separator (e.g., `env_read`, not `envread`).
2. Text before `_` must match a registered module `id` exactly.
3. Text after `_` is passed as the `cmd` parameter to the module's `ws_handler`.
4. If `ws_handler` is NULL, the module is matched but no callback fires (no crash).
5. If no module matches, dispatch returns 0 (unhandled).
6. Core commands (`status`, `wifi`, `pin`) are handled before module dispatch.

---

## 5. Module Lifecycle

```
                         BOOT
                           |
                           v
               +------------------------+
               | smrt_register_modules()|  <-- Conditional #ifdef registration
               +------------------------+
                           |
                           v
               +------------------------+
               | Core subsystem init    |  <-- GPIO, Serial, WiFi, HTTP, WS, OTA
               +------------------------+
                           |
                           v
               +------------------------+
               | smrt_module_init_all() |  <-- Calls mod->init() for each module
               +------------------------+
                           |
                           v
                    +--------------+
               +--->| MAIN LOOP    |
               |    +--------------+
               |           |
               |           v
               |    +-------------------+
               |    | ArduinoOTA.handle |
               |    +-------------------+
               |           |
               |           v
               |    +------------------------+
               |    | smrt_module_loop_all() |  <-- Calls mod->loop() for each
               |    +------------------------+
               |           |
               |           v
               |    +------------------------+
               |    | smrt_ws.cleanupClients |
               |    +------------------------+
               |           |
               |           v
               |    +------------------------+
               |    | Periodic telemetry     |  <-- Every SMRT_STATUS_INTERVAL_MS
               |    | smrt_ws_send_status()  |      Aggregates all module telemetry
               |    +------------------------+
               |           |
               +-----------+
```

---

## 6. Telemetry Aggregation

Periodic status broadcasts combine core platform data with per-module telemetry:

```json
{
  "type": "status",
  "heap": 245760,
  "uptime": 12345,
  "wifi_rssi": -42,
  "ip": "192.168.1.100",
  "modules": {
    "env": {
      "temperature": 23.5,
      "humidity": 45.2
    },
    "sec": {
      "armed": true,
      "last_uid": "A1B2C3D4"
    }
  }
}
```

Each module's `get_telemetry(data)` callback receives a JSON object scoped to
its own `id` key. Modules only write their own data — the core aggregates.

---

## 7. Conditional Compilation

Modules are enabled/disabled via build flags in `platformio.ini`:

```ini
build_flags =
    -D SMRT_MOD_ENV    ; Environmental sensors (DHT22, BMP280)
    -D SMRT_MOD_SEC    ; Security / NFC (PN532)
    -D SMRT_MOD_RLY    ; Relay control
    -D SMRT_MOD_PLG    ; Smart plug / energy monitoring
    -D SMRT_MOD_ACC    ; Access control
    -D SMRT_MOD_NRG    ; Energy monitoring (PZEM)
```

Registration in `smrt_main.cpp`:

```c
static void smrt_register_modules(void) {
    #ifdef SMRT_MOD_ENV
        extern const smrt_module_t smrt_mod_env;
        smrt_module_register(&smrt_mod_env);
    #endif
    #ifdef SMRT_MOD_SEC
        extern const smrt_module_t smrt_mod_sec;
        smrt_module_register(&smrt_mod_sec);
    #endif
    // ... etc
}
```

This approach means:
- **Zero overhead** for disabled modules (not even compiled).
- **No runtime checks** — the linker excludes unused module code.
- **Easy toggling** — change one build flag to add/remove a module.

---

## 8. Global Object Sharing

Two global objects are shared across core modules:

```c
// Defined in smrt_core_http.cpp
AsyncWebServer smrt_server(SMRT_WEB_SERVER_PORT);
AsyncWebSocket smrt_ws(SMRT_WS_PATH);

// Used via extern in other core modules
extern AsyncWebServer smrt_server;  // smrt_core_ota.cpp
extern AsyncWebSocket smrt_ws;      // smrt_core_ws.cpp, smrt_main.cpp
```

---

## 9. File Map

```
HomeNode/
├── include/
│   └── core/
│       ├── smrt_core.h              Umbrella header (includes all below)
│       ├── smrt_core_config.h       Platform constants and #defines
│       ├── smrt_mc_format.h         Custom types and conversion macros
│       ├── smrt_mc_serial.h         Serial port API
│       ├── smrt_mc_gpio.h           GPIO control API
│       ├── smrt_core_module.h       Module interface and registry API
│       ├── smrt_core_nvs.h          NVS persistence API
│       ├── smrt_core_wifi.h         WiFi management API
│       ├── smrt_core_ws.h           WebSocket server API
│       ├── smrt_core_http.h         HTTP server API
│       ├── smrt_core_ota.h          OTA update API
│       └── smrt_core_webui.h        Embedded web UI (PROGMEM)
│
├── src/
│   ├── smrt_main.cpp                Simplified entry point (~120 LOC)
│   └── core/
│       ├── smrt_mc_format.cpp       Type conversion implementations
│       ├── smrt_mc_serial.cpp       Serial init/read
│       ├── smrt_mc_gpio.cpp         GPIO init/set/clear/toggle
│       ├── smrt_core_module.cpp     Module registry and dispatch engine
│       ├── smrt_core_nvs.cpp        NVS get/set over Preferences API
│       ├── smrt_core_wifi.cpp       WiFi STA + AP fallback, hostname
│       ├── smrt_core_ws.cpp         WebSocket events, dispatch, telemetry
│       ├── smrt_core_auth.cpp       WS session auth, PIN rate limiting
│       ├── smrt_core_http.cpp       Server + route registration
│       └── smrt_core_ota.cpp        ArduinoOTA + HTTP upload (auth)
│
├── test/
│   ├── test_mc_format/
│   │   └── test_smrt_mc_format.cpp  85 unit tests (data types/conversions)
│   └── test_core_module/
│       └── test_smrt_core_module.cpp 17 unit tests (registry/dispatch)
│
├── docs/
│   ├── requirements.md              Platform requirements (FASE 1)
│   └── architecture.md              This document (FASE 2)
│
├── platformio.ini                   Build environments (esp32, ota, native)
├── smrt_qa.py                       QA automation script
├── add_gcc_path.py                  WinLibs GCC PATH helper
├── CLAUDE.md                        Development methodology
├── claude_rules.xml                 Exportable rules (XML)
├── README.md                        Project overview
└── .gitignore
```

**Total FASE 2:** 25 files, ~3000 LOC estimated

---

## 10. Test Strategy

### Native Tests (PC, Unity Framework)

Tests run on the host PC without ESP32 hardware:

```bash
pio test -e native
```

**Isolation techniques:**
- `#define UNIT_TEST` excludes Arduino/ESP32 headers.
- `void *` replaces ArduinoJson and AsyncWebSocket types.
- Source files included directly: `#include "../../src/core/module.cpp"`.
- `smrt_module_reset()` clears the static registry between tests.

### Test Coverage (FASE 2)

| Test File | Tests | Target |
|-----------|-------|--------|
| `test_smrt_mc_format.cpp` | 85 | Data type conversions, formatting |
| `test_smrt_core_module.cpp` | 17 | Registry, dispatch, lifecycle, telemetry |
| **Total** | **102** | |

---

## 11. WebSocket JSON Protocol

### Core Commands (handled by smrt_core_ws.cpp)

| Command | Direction | Description |
|---------|-----------|-------------|
| `status` | Client -> Server | Request status broadcast (no auth) |
| `auth` | Client -> Server | Authenticate with PIN |
| `wifi` | Client -> Server | Update WiFi credentials (own PIN check) |
| `set_hostname` | Client -> Server | Change mDNS hostname (requires auth) |

### Module Commands (dispatched to modules)

| Command | Routed To | Handler Receives |
|---------|-----------|-----------------|
| `env_read` | Module "env" | `"read"` |
| `sec_arm` | Module "sec" | `"arm"` |
| `rly_toggle` | Module "rly" | `"toggle"` |
| `plg_set` | Module "plg" | `"set"` |

### Status Response

```json
{
  "type": "status",
  "heap": <free_heap_bytes>,
  "uptime": <millis>,
  "wifi_rssi": <rssi_dbm>,
  "ip": "<ip_address>",
  "modules": {
    "<module_id>": { ... module-specific telemetry ... }
  }
}
```

---

## 12. Adding a New Module (Guide)

To create a new module (e.g., `smrt_mod_env` for environmental sensors):

### Step 1: Create header and source

```
include/modules/smrt_mod_env.h
src/modules/smrt_mod_env.cpp
```

### Step 2: Implement the interface

```c
// smrt_mod_env.cpp
#include "smrt_core.h"

static void env_init(void)  { /* Setup DHT22, BMP280 */ }
static void env_loop(void)  { /* Read sensors periodically */ }

static void env_ws_handler(const char *cmd, void *doc, void *client) {
    // cmd = "read", "config", etc. (prefix already stripped)
    JsonDocument &json = *(JsonDocument *)doc;
    AsyncWebSocketClient *ws = (AsyncWebSocketClient *)client;
    // ... handle command
}

static void env_get_telemetry(void *data) {
    JsonObject &obj = *(JsonObject *)data;
    obj["temperature"] = last_temp;
    obj["humidity"]    = last_hum;
}

const smrt_module_t smrt_mod_env = {
    "env", "Environmental", "1.0.0",
    env_init, env_loop, env_ws_handler, env_get_telemetry
};
```

### Step 3: Enable via build flag

In `platformio.ini`:
```ini
build_flags = ... -D SMRT_MOD_ENV
```

### Step 4: Register in main

In `smrt_main.cpp`, the `#ifdef SMRT_MOD_ENV` block automatically
registers the module at boot.

### Step 5: Write tests

```
test/test_native/test_smrt_mod_env.cpp
```

---

*Document generated during FASE 2. Updated through FASE 7 (v0.7.0) — Security P2, Features, Documentation.*
