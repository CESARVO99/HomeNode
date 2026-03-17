# HomeNode v0.8.0 — Guia de Administrador

> **Plataforma IoT domestica modular para ESP32**
> Fecha: 2026-03-17 | Version: 0.8.0
> Audiencia: Administradores, integradores, desarrolladores

---

## Tabla de Contenidos

1. [Descripcion General](#1-descripcion-general)
2. [Requisitos de Sistema](#2-requisitos-de-sistema)
3. [Arquitectura de la Plataforma](#3-arquitectura-de-la-plataforma)
4. [Compilacion y Herramientas](#4-compilacion-y-herramientas)
5. [Configuracion de Red y WiFi](#5-configuracion-de-red-y-wifi)
6. [Seguridad](#6-seguridad)
7. [Protocolo WebSocket — Referencia Completa](#7-protocolo-websocket--referencia-completa)
8. [Servicios del Core](#8-servicios-del-core)
9. [Modulo ENV — Ambiente](#9-modulo-env--ambiente)
10. [Modulo RLY — Reles](#10-modulo-rly--reles)
11. [Modulo SEC — Seguridad](#11-modulo-sec--seguridad)
12. [Modulo PLG — Enchufe Inteligente](#12-modulo-plg--enchufe-inteligente)
13. [Modulo NRG — Monitor de Energia](#13-modulo-nrg--monitor-de-energia)
14. [Modulo ACC — Control de Acceso](#14-modulo-acc--control-de-acceso)
15. [Scheduler — Tareas Programadas](#15-scheduler--tareas-programadas)
16. [MQTT — Integracion con Brokers](#16-mqtt--integracion-con-brokers)
17. [Webhooks — Notificaciones HTTP](#17-webhooks--notificaciones-http)
18. [Backup y Restauracion de Configuracion](#18-backup-y-restauracion-de-configuracion)
19. [Actualizacion OTA](#19-actualizacion-ota)
20. [Persistencia NVS](#20-persistencia-nvs)
21. [Tests Unitarios y QA](#21-tests-unitarios-y-qa)
22. [Mapa de Pines y Conflictos de Hardware](#22-mapa-de-pines-y-conflictos-de-hardware)
23. [Referencia Rapida de Comandos WS](#23-referencia-rapida-de-comandos-ws)
24. [Historial de Versiones](#24-historial-de-versiones)

---

## 1. Descripcion General

**HomeNode** es una plataforma firmware modular para automatizacion domestica sobre hardware ESP32-WROOM (NodeMCU-32S). Permite combinar hasta 6 modulos funcionales independientes en un solo dispositivo, activados selectivamente en tiempo de compilacion.

### Caracteristicas v0.8.0

| Categoria | Descripcion |
|-----------|-------------|
| **Modulos** | ENV, RLY, SEC, PLG, NRG, ACC (6 modulos independientes) |
| **Core** | WiFi, HTTP, WebSocket, NVS, OTA, mDNS |
| **Seguridad** | Autenticacion PIN, sesiones WS, rate limiting, NVS cifrado AES-128 |
| **Tiempo** | Sincronizacion NTP (pool.ntp.org + time.nist.gov), zona horaria configurable |
| **Scheduler** | 8 tareas programadas tipo cron con bitmask de dias |
| **Eventos** | Bus de eventos interno (ENV alerts, SEC arm/trigger, ACC auth/deny, RLY change, NRG overload) |
| **MQTT** | Cliente PubSubClient opcional (flag -D SMRT_MQTT) con reconexion exponencial |
| **Webhooks** | Hasta 4 endpoints HTTP POST con filtros de eventos por tipo |
| **Backup** | Exportacion/importacion de configuracion NVS (proteccion doble confirmacion) |
| **Tests** | Tests nativos en PC sin hardware ESP32 (framework Unity) |

### Alcance funcional tipico

```
ESP32 (HomeNode)
    ├── Dashboard web (HTTP/80) ──► Browser (PC/movil)
    ├── WebSocket /ws ──────────► Comandos bidireccionales en tiempo real
    ├── MQTT ──────────────────► Broker (Home Assistant, Mosquitto, etc.)
    ├── Webhooks ──────────────► Node-RED, IFTTT, endpoints HTTP propios
    ├── OTA (ArduinoOTA + HTTP) ► Actualizacion sin cables
    └── mDNS homenode.local ───► Descubrimiento en red local
```

---

## 2. Requisitos de Sistema

### Hardware ESP32

| Componente | Especificacion | Notas |
|------------|---------------|-------|
| Microcontrolador | ESP32-WROOM (NodeMCU-32S) | Probado en NodeMCU-32S |
| Flash | 4 MB minimo | Firmware ~70.9% de 4 MB |
| RAM | 520 KB SRAM | Uso ~15.1% en config basica |
| WiFi | 802.11 b/g/n 2.4 GHz | Solo 2.4 GHz (no 5 GHz) |
| USB | CH340C / CP2102 para serial | Solo para flash inicial |

### Software de Desarrollo

| Herramienta | Version Minima | Instalacion |
|-------------|---------------|-------------|
| Python | 3.8+ | `winget install Python.Python.3` |
| PlatformIO | 6.x | `pip install platformio` o extension VS Code |
| GCC (nativo) | 11+ | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |
| cppcheck | 2.x | `winget install cppcheck` |
| Git | 2.x | `winget install Git.Git` |

### Librerias (gestionadas por PlatformIO)

| Libreria | Version | Proposito |
|----------|---------|-----------|
| mathieucarbou/ESP Async WebServer | ^3.0.6 | HTTP + WebSocket async |
| bblanchon/ArduinoJson | ^7 | Serializacion/deserializacion JSON |
| adafruit/DHT sensor library | ^1.4.6 | Sensor DHT22 (modulo ENV) |
| adafruit/Adafruit Unified Sensor | ^1.1.14 | Dependencia de DHT |
| miguelbalboa/MFRC522 | ^1.4.11 | Lector NFC (modulo ACC) |
| knolleary/PubSubClient | ^2.8 | MQTT (descomentado si SMRT_MQTT) |

> **Nota:** mbedTLS (para AES-128-CBC) esta incluido en el ESP-IDF/Arduino ESP32 sin libreria adicional.

---

## 3. Arquitectura de la Plataforma

### Capas del sistema

```
┌─────────────────────────────────────────────────────────┐
│                    MODULES (capa 4)                     │
│     ENV  |  RLY  |  SEC  |  PLG  |  NRG  |  ACC        │
│   Logica de aplicacion, sensores, actuadores            │
├─────────────────────────────────────────────────────────┤
│                    CORE SERVICES (capa 3)               │
│  Crypto | NTP/Sched | EventBus | MQTT | Webhook | Backup│
│   Servicios transversales, opcionales por build flag    │
├─────────────────────────────────────────────────────────┤
│                    CORE PLATFORM (capa 2)               │
│    WiFi | HTTP | WebSocket | NVS | Auth | OTA | WebUI   │
│   Infraestructura de red, persistencia, seguridad base  │
├─────────────────────────────────────────────────────────┤
│                    HAL (capa 1)                         │
│         GPIO | Serial | I2C | SPI | ADC                 │
│         Abstraccion de hardware, sin logica de negocio  │
└─────────────────────────────────────────────────────────┘
```

### Estructura de archivos

```
HomeNode/
├── include/
│   ├── core/
│   │   ├── smrt_core_config.h       # Constantes globales de la plataforma
│   │   ├── smrt_core.h              # Header umbrella (incluye todo el core)
│   │   ├── smrt_core_module.h       # Sistema de registro de modulos
│   │   ├── smrt_core_nvs.h          # API de persistencia NVS
│   │   ├── smrt_core_auth.h         # Autenticacion y sesiones WS
│   │   ├── smrt_core_wifi.h         # Gestion WiFi STA + AP fallback
│   │   ├── smrt_core_ws.h           # Servidor WebSocket
│   │   ├── smrt_core_http.h         # Servidor HTTP
│   │   ├── smrt_core_ota.h          # Actualizacion OTA
│   │   ├── smrt_core_crypto.h       # AES-128-CBC, NVS cifrado
│   │   ├── smrt_core_time.h         # NTP, timezone, hora actual
│   │   ├── smrt_core_event.h        # Bus de eventos
│   │   ├── smrt_core_sched.h        # Scheduler de tareas
│   │   ├── smrt_core_sched_config.h # Configuracion del scheduler
│   │   ├── smrt_core_mqtt.h         # Cliente MQTT
│   │   ├── smrt_core_mqtt_config.h  # Configuracion MQTT
│   │   ├── smrt_core_webhook.h      # Webhooks HTTP
│   │   ├── smrt_core_webhook_config.h # Configuracion webhooks
│   │   ├── smrt_core_backup.h       # Backup/restore NVS
│   │   ├── smrt_mc_format.h         # Tipos de datos y conversion
│   │   ├── smrt_mc_serial.h         # Puerto serie
│   │   └── smrt_mc_gpio.h           # Control GPIO
│   └── modules/
│       ├── smrt_mod_env*.h          # Modulo ambiental
│       ├── smrt_mod_rly*.h          # Modulo reles
│       ├── smrt_mod_sec*.h          # Modulo seguridad
│       ├── smrt_mod_plg*.h          # Modulo enchufes
│       ├── smrt_mod_nrg*.h          # Modulo energia
│       └── smrt_mod_acc*.h          # Modulo control de acceso
├── src/
│   ├── core/                        # Implementaciones del core
│   ├── modules/                     # Implementaciones de modulos
│   └── smrt_main.cpp                # Punto de entrada setup()/loop()
├── test/
│   ├── test_mc_format/
│   ├── test_core_module/
│   ├── test_core_sched/             # NUEVO en v0.8.0
│   ├── test_mod_env/
│   ├── test_mod_sec/
│   ├── test_mod_plg/
│   ├── test_mod_nrg/
│   ├── test_mod_rly/
│   └── test_mod_acc/
├── docs/
│   ├── guia_administrador.md        # Este documento
│   ├── architecture.md
│   ├── requirements.md
│   └── auditoria_seguridad.md
├── platformio.ini
├── smrt_qa.py
└── add_gcc_path.py
```

### Sistema de modulos

Cada modulo implementa la interfaz `smrt_module_t`:

```c
typedef struct {
    const char *id;           // Prefijo de comando (ej: "env")
    const char *name;         // Nombre legible
    const char *version;      // Version semantica
    void (*init)(void);       // Inicializacion (llamado en setup)
    void (*loop)(void);       // Loop periodico (llamado en loop)
    void (*ws_handler)(const char *cmd, void *doc, void *client); // WS commands
    void (*get_telemetry)(void *json_obj); // Telemetria periodica
} smrt_module_t;
```

El dispatcher central (`smrt_module_dispatch`) extrae el prefijo del comando WS y llama al `ws_handler` del modulo correspondiente con el sufijo ya limpio.

---

## 4. Compilacion y Herramientas

### Build flags disponibles

| Flag | Por defecto | Descripcion |
|------|------------|-------------|
| `-D SMRT_PLATFORM_HOMENODE` | ON | Identifica la plataforma |
| `-D SMRT_MOD_ENV` | ON | Habilita modulo ambiental (DHT22) |
| `-D SMRT_MOD_RLY` | ON | Habilita modulo de reles |
| `-D SMRT_MOD_SEC` | ON | Habilita modulo de seguridad |
| `-D SMRT_MOD_PLG` | OFF | Habilita modulo enchufe inteligente |
| `-D SMRT_MOD_NRG` | OFF | Habilita monitor de energia |
| `-D SMRT_MOD_ACC` | OFF | Habilita control de acceso NFC |
| `-D SMRT_CRYPTO` | ON | Cifrado AES-128-CBC para NVS |
| `-D SMRT_SCHED` | ON | Scheduler de tareas + NTP |
| `-D SMRT_MQTT` | OFF | Cliente MQTT (requiere PubSubClient) |
| `-D SMRT_WEBHOOK` | OFF | Webhooks HTTP |
| `-D SMRT_DEBUG` | ON (dev) | Logs de depuracion por Serial |

> **Conflictos de modulos:** Ver seccion 22 (Mapa de Pines).

### Comandos de compilacion

```bash
# Compilar para ESP32 (produccion)
pio run -e nodemcu-32s

# Compilar y flashear por USB
pio run -e nodemcu-32s --target upload

# Compilar y flashear por OTA
pio run -e ota --target upload

# Ejecutar tests nativos en PC
pio test -e native

# Ejecutar tests con salida verbose
pio test -e native -v

# Monitor serie
pio device monitor --baud 115200

# QA completo (tests + analisis + metricas + changelog)
python smrt_qa.py

# Solo analisis estatico
cppcheck --enable=all --suppress=missingInclude --suppress=unknownMacro \
         --std=c++11 -I include -I include/core -I include/modules src/ include/
```

### Configurar MQTT (ejemplo)

Para habilitar MQTT, editar `platformio.ini`:

```ini
build_flags =
    ...
    -D SMRT_MQTT
    ...
lib_deps =
    ...
    knolleary/PubSubClient@^2.8
```

---

## 5. Configuracion de Red y WiFi

### Primera configuracion (modo AP)

Si el dispositivo no tiene credenciales WiFi guardadas, arranca en modo AP:

1. Conectar al SSID `HomeNode-Setup` (pass: `homenode123`)
2. Abrir `http://192.168.4.1` en el navegador
3. Autenticarse con el PIN por defecto (`1234`)
4. Enviar el comando de configuracion WiFi

### Comando WebSocket para configurar WiFi

```json
{
  "cmd": "wifi",
  "ssid": "MiRedWiFi",
  "pass": "contraseña_wifi",
  "pin": "1234",
  "new_pin": "9876"
}
```

> **Nota:** `new_pin` es opcional. Si se omite, el PIN no cambia.

El dispositivo guarda las credenciales en NVS (cifradas si `SMRT_CRYPTO` esta activo), confirma y reinicia.

### Modo STA (normal)

En modo STA el dispositivo intenta conectar durante `SMRT_WIFI_STA_TIMEOUT_MS` (15 s). Si falla, activa el AP de fallback automaticamente.

### Configurar hostname mDNS

```json
{ "cmd": "set_hostname", "hostname": "salon" }
```

Acceso posterior: `http://salon.local` (requiere mDNS en el cliente).

### IP estatica

Configurar en `smrt_core_config.h`:

```c
#define SMRT_STATIC_IP    192, 168, 1, 100
#define SMRT_GATEWAY_IP   192, 168, 1, 1
#define SMRT_SUBNET_MASK  255, 255, 255, 0
#define SMRT_DNS_IP       8, 8, 8, 8
```

---

## 6. Seguridad

### Modelo de seguridad

HomeNode esta disenado para uso en red local (LAN) domestica. No esta disenado para exposicion directa a internet sin proxy/VPN.

| Capa | Mecanismo | Estado |
|------|-----------|--------|
| Red | Filtrado por IP privada (RFC1918) en WebSocket | ACTIVO |
| Autenticacion | PIN con rate limiting (3 intentos, bloqueo 60s) | ACTIVO |
| Sesion | Timeout inactividad 10 min, logout en desconexion | ACTIVO |
| Transporte HTTP | Sin TLS (AsyncWebServer no soporta HTTPS) | LIMITACION |
| Transporte WS | Sin TLS nativo — mitigado con cifrado de payload AES | MITIGADO |
| Almacenamiento | NVS cifrado AES-128-CBC (si `SMRT_CRYPTO`) | ACTIVO |
| OTA | Autenticacion HTTP Basic + password OTA | ACTIVO |
| Secure Boot | Requiere burning de eFuses ESP32 | NO IMPL. |

### Cifrado NVS (`SMRT_CRYPTO`)

Cuando el flag `SMRT_CRYPTO` esta activo:

- La clave de cifrado se deriva del MAC del chip mediante HMAC-SHA256 (unica por dispositivo)
- Las credenciales sensibles (WiFi password, PIN) se almacenan como `ENC:<base64(IV+ciphertext)>`
- Los valores en texto plano existentes se migran automaticamente al primer acceso
- Algoritmo: AES-128-CBC con IV aleatorio por cada escritura

### Autenticacion WebSocket

Todos los comandos de escritura requieren autenticacion previa:

```json
// 1. Iniciar sesion
{ "cmd": "auth", "pin": "1234" }

// Respuesta exito:
{ "auth_result": true, "auth_msg": "Autenticado correctamente" }

// Respuesta fallo (con rate limiting):
{ "auth_result": false, "auth_msg": "PIN incorrecto" }
{ "auth_result": false, "auth_msg": "Bloqueado. Reintenta en 45s" }
```

Comandos que NO requieren autenticacion: `status`, `auth`, `wifi` (este ultimo tiene su propia validacion PIN).

### Configurar PIN de acceso

El PIN se cambia mediante el campo `new_pin` en el comando `wifi`:

```json
{ "cmd": "wifi", "ssid": "...", "pass": "...", "pin": "PIN_ACTUAL", "new_pin": "NUEVO_PIN" }
```

PIN por defecto: `1234` (cambiar obligatoriamente en produccion).

### Credenciales OTA

Configurar en `smrt_core_config.h` antes de compilar:

```c
#define SMRT_OTA_PASSWORD    "mi_password_ota_seguro"
#define SMRT_AUTH_OTA_PASS   "mi_password_ota_seguro"
```

### Limitaciones conocidas

- **V-07 (HTTPS):** AsyncWebServer no soporta TLS. El trafico HTTP/WS viaja en texto claro. Mitigacion: payload WS cifrado AES, uso solo en LAN de confianza.
- **V-04 (Secure Boot):** No implementado. Requiere herramientas ESP-IDF y burning de eFuses hardware. Documentado como limitacion de plataforma.
- **V-10 (Firmware signing):** No implementado. OTA protegido por password pero sin firma criptografica del binario.

---

## 7. Protocolo WebSocket — Referencia Completa

### Conexion

```
ws://<ip_dispositivo>/ws
```

La conexion solo se acepta desde IPs en rangos privados (RFC1918): `192.168.x.x`, `10.x.x.x`, `172.16-31.x.x`, `127.x.x.x`.

### Telemetria automatica

El dispositivo emite un JSON de estado cada 5 segundos a todos los clientes conectados y tambien al conectarse un nuevo cliente:

```json
{
  "rssi": -62,
  "uptime": 125400,
  "ip": "192.168.1.100",
  "clients": 1,
  "ssid": "MiRedWiFi",
  "ap_mode": false,
  "version": "0.8.0",
  "modules": {
    "env": { "temperature": 22.5, "humidity": 55.0, "ok": true, "alert_enabled": false },
    "rly": { "count": 4, "states": [false, false, true, false] },
    "sec": { "state": "ARMED", "pir": false, "reed": false }
  }
}
```

### Formato de comandos

Todos los comandos siguen la estructura:

```json
{ "cmd": "<modulo_subcmd>", "<campo1>": <valor1>, ... }
```

El campo `cmd` sigue el patron `<modulo>_<subcomando>`:
- `env_read`, `env_set_interval`
- `rly_toggle`, `rly_set`, `rly_status`
- `sec_arm`, `sec_disarm`, `sec_status`
- `sched_list`, `sched_set`, `sched_delete`
- `mqtt_config`, `mqtt_enable`, `mqtt_status`
- `webhook_set`, `webhook_list`, `webhook_test`
- `cfg_export`, `cfg_import`
- `time_set_tz`

### Errores

```json
{ "error": "No autenticado. Envia {\"cmd\":\"auth\",\"pin\":\"XXXX\"} primero" }
{ "error": "Mensaje de error especifico" }
```

---

## 8. Servicios del Core

### 8.1 NTP — Sincronizacion de Tiempo

El servicio NTP se inicializa automaticamente con WiFi:

- Servidores: `pool.ntp.org` + `time.nist.gov`
- Espera hasta 10 s en el boot para sincronizar
- Resincroniza periodicamente en background (lwIP SNTP)
- La zona horaria se almacena en NVS (namespace `time`, claves `gmt_off`, `dst_off`)

#### Configurar zona horaria

```json
{
  "cmd": "time_set_tz",
  "gmt_offset": 3600,
  "dst_offset": 3600
}
```

- `gmt_offset`: offset UTC en segundos (ej: 3600 = UTC+1, -21600 = UTC-6)
- `dst_offset`: offset horario de verano en segundos (3600 si hay DST, 0 si no)

Ejemplo para zona horaria de Espana:
- Invierno (CET): `gmt_offset: 3600, dst_offset: 0`
- Verano (CEST): `gmt_offset: 3600, dst_offset: 3600`

### 8.2 Bus de Eventos

El bus de eventos permite que los modulos publiquen eventos sin conocer los destinos. Los consumidores actuales son MQTT y Webhooks.

#### Tipos de eventos

| Constante | Valor | Disparado por |
|-----------|-------|---------------|
| `SMRT_EVT_SEC_TRIGGERED` | 0x01 | SEC: zona de alarma activada |
| `SMRT_EVT_SEC_ARMED` | 0x02 | SEC: sistema armado |
| `SMRT_EVT_SEC_DISARMED` | 0x04 | SEC: sistema desarmado |
| `SMRT_EVT_ACC_AUTHORIZED` | 0x08 | ACC: tarjeta NFC autorizada |
| `SMRT_EVT_ACC_DENIED` | 0x10 | ACC: tarjeta NFC denegada |
| `SMRT_EVT_NRG_OVERLOAD` | 0x20 | NRG: sobrecarga de potencia |
| `SMRT_EVT_ENV_ALERT` | 0x40 | ENV: umbral temperatura/humedad superado |
| `SMRT_EVT_RLY_CHANGED` | 0x80 | RLY: estado de rele cambiado |

---

## 9. Modulo ENV — Ambiente

**Hardware:** Sensor DHT22 en GPIO4.
**Build flag:** `-D SMRT_MOD_ENV`

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `env_read` | — | Solicita lectura inmediata del sensor |
| `env_set_interval` | `value` (ms) | Cambia intervalo de lectura (2000-60000 ms) |
| `env_set_alert` | `temp_hi`, `temp_lo`, `hum_hi`, `hum_lo`, `enabled` | Configura alertas de temperatura/humedad |
| `env_get_alert` | — | Devuelve configuracion actual de alertas |

### Alertas ambientales

Cuando `enabled` es `true`, el modulo comprueba si los valores de temperatura/humedad superan los umbrales configurados. Si se supera un umbral (y el estado cambia), se publica el evento `SMRT_EVT_ENV_ALERT` con el payload:

```json
{ "alert": 5, "temp": 42.1, "hum": 88.3 }
```

El campo `alert` es un bitmask:
- `0x01` = temperatura alta
- `0x02` = temperatura baja
- `0x04` = humedad alta
- `0x08` = humedad baja

### Telemetria

```json
{
  "temperature": 22.5,
  "humidity": 55.0,
  "ok": true,
  "alert_enabled": false,
  "alert": 0
}
```

### Configuracion

| Define | Valor por defecto | Descripcion |
|--------|------------------|-------------|
| `SMRT_ENV_DHT_PIN` | 4 | GPIO del sensor |
| `SMRT_ENV_READ_INTERVAL_MS` | 5000 | Intervalo de lectura |
| `SMRT_ENV_TEMP_ALERT_HI` | 40.0 | Umbral alta temperatura |
| `SMRT_ENV_TEMP_ALERT_LO` | 5.0 | Umbral baja temperatura |
| `SMRT_ENV_HUM_ALERT_HI` | 85.0 | Umbral alta humedad |
| `SMRT_ENV_HUM_ALERT_LO` | 20.0 | Umbral baja humedad |

---

## 10. Modulo RLY — Reles

**Hardware:** 4 reles en GPIOs configurables.
**Build flag:** `-D SMRT_MOD_RLY`

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `rly_toggle` | `index` (0-3) | Invierte el estado del rele |
| `rly_set` | `index` (0-3), `state` (0/1) | Fija el estado del rele |
| `rly_pulse` | `index` (0-3), `ms` (duracion) | Activa rele por tiempo |
| `rly_status` | — | Devuelve estados de los 4 reles |
| `rly_set_label` | `index`, `label` | Asigna etiqueta al rele |

### Eventos publicados

Cada cambio de estado de rele publica `SMRT_EVT_RLY_CHANGED` con el payload:

```json
{ "index": 0, "state": true }
```

### Telemetria

```json
{
  "count": 4,
  "states": [false, true, false, false],
  "labels": ["Luz salon", "Ventilador", "Persiana", "Extra"]
}
```

---

## 11. Modulo SEC — Seguridad

**Hardware:** PIR (GPIO12), Reed switch (GPIO13), Vibration (GPIO14), Buzzer (GPIO25).
**Build flag:** `-D SMRT_MOD_SEC`

### Estados de la maquina de estados

```
DISARMED ──arm──> ARMING ──exit_delay──> ARMED
ARMED ──sensor──> ENTRY_DELAY ──timeout──> TRIGGERED
ARMED ──sensor──> TRIGGERED (si no hay entry delay)
TRIGGERED ──disarm──> DISARMED
ARMING ──disarm──> DISARMED
```

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `sec_arm` | — | Arma el sistema (inicia exit delay) |
| `sec_disarm` | — | Desarma el sistema |
| `sec_status` | — | Devuelve estado completo |
| `sec_set_entry_delay` | `value` (ms) | Configura retardo de entrada |
| `sec_set_exit_delay` | `value` (ms) | Configura retardo de salida |
| `sec_get_events` | — | Devuelve log circular de eventos |

### Eventos publicados

| Evento | Constante |
|--------|-----------|
| Sistema armado | `SMRT_EVT_SEC_ARMED` |
| Alarma activada | `SMRT_EVT_SEC_TRIGGERED` |
| Sistema desarmado | `SMRT_EVT_SEC_DISARMED` |

---

## 12. Modulo PLG — Enchufe Inteligente

**Hardware:** Rele de carga en GPIO2, medicion de corriente via ADC.
**Build flag:** `-D SMRT_MOD_PLG`
**Conflicto:** No compatible con NRG ni con ACC (mismo GPIO2 que el lock relay).

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `plg_set` | `state` (0/1) | Controla el rele de carga |
| `plg_toggle` | — | Invierte el estado |
| `plg_status` | — | Devuelve estado y consumo |
| `plg_set_limit` | `watts` | Configura limite de potencia |

---

## 13. Modulo NRG — Monitor de Energia

**Hardware:** Sensor SCT-013 via ADC (GPIO34-39), opcionalmente PZEM-004T via UART.
**Build flag:** `-D SMRT_MOD_NRG`
**Conflicto:** No compatible con PLG (mismos ADC pins GPIO34-39).

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `nrg_status` | — | Devuelve mediciones actuales |
| `nrg_set_limit` | `channel` (0-1), `watts` | Configura limite de sobrecarga |
| `nrg_reset_energy` | `channel` (0-1) | Reinicia contador de energia acumulada |
| `nrg_calibrate` | `channel`, `vref` | Calibra el factor de voltaje |

### Campos de telemetria por canal

```json
{
  "channels": [
    {
      "voltage": 230.1,
      "current": 1.24,
      "power": 285.3,
      "energy_wh": 1420.5,
      "power_factor": 0.98,
      "overload": false
    }
  ]
}
```

### Eventos publicados

Cuando la potencia supera el limite configurado se publica `SMRT_EVT_NRG_OVERLOAD`:

```json
{ "channel": 0, "power": 3200.0, "limit": 3000.0 }
```

---

## 14. Modulo ACC — Control de Acceso

**Hardware:** MFRC522 via SPI, lock relay en GPIO2.
**Build flag:** `-D SMRT_MOD_ACC`
**Conflicto:** No compatible con PLG (GPIO2), no compatible con SEC (GPIO12-14 solapados).

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `acc_toggle` | — | Pulsa el rele del cerrojo |
| `acc_status` | — | Devuelve estado completo |
| `acc_add_uid` | `uid` ("XX:XX:XX:XX") | Agrega UID autorizado |
| `acc_remove_uid` | `uid` | Elimina UID autorizado |
| `acc_list_uids` | — | Lista todos los UIDs autorizados |
| `acc_clear_uids` | — | Borra todos los UIDs |
| `acc_set_pulse` | `value` (ms) | Duracion del pulso de cerrojo |
| `acc_get_events` | — | Log circular de accesos |
| `acc_learn` | — | Activa modo aprendizaje (30s) |
| `acc_learn_cancel` | — | Cancela modo aprendizaje |

### Modo aprendizaje

Al enviar `acc_learn`, el sistema entra en modo aprendizaje durante `SMRT_ACC_LEARN_TIMEOUT_MS` (30 s por defecto). El siguiente NFC leido se agrega automaticamente a la lista de autorizados y se publica el evento `SMRT_EVT_ACC_AUTHORIZED` con `action: "learned"`.

### Eventos publicados

| Evento | Constante | Payload |
|--------|-----------|---------|
| Acceso autorizado | `SMRT_EVT_ACC_AUTHORIZED` | `{ "uid": "XX:XX:XX:XX", "action": "granted" }` |
| UID aprendido | `SMRT_EVT_ACC_AUTHORIZED` | `{ "uid": "XX:XX:XX:XX", "action": "learned" }` |
| Acceso denegado | `SMRT_EVT_ACC_DENIED` | `{ "uid": "XX:XX:XX:XX" }` |

### Configuracion

| Define | Valor | Descripcion |
|--------|-------|-------------|
| `SMRT_ACC_MAX_UIDS` | 20 | UIDs maximos autorizados |
| `SMRT_ACC_PULSE_DEFAULT_MS` | 500 | Duracion de pulso de cerrojo |
| `SMRT_ACC_LEARN_TIMEOUT_MS` | 30000 | Timeout modo aprendizaje |
| `SMRT_ACC_LOCKOUT_ATTEMPTS` | 5 | Intentos fallidos antes de bloqueo |
| `SMRT_ACC_LOCKOUT_MS` | 60000 | Duracion del bloqueo (ms) |

---

## 15. Scheduler — Tareas Programadas

**Requisito:** `-D SMRT_SCHED` (incluye NTP automaticamente).

El scheduler permite ejecutar comandos de modulos de forma automatica a horas especificas, usando un bitmask de dias de la semana.

### Capacidad

- 8 tareas simultaneas (`SMRT_SCHED_MAX_TASKS`)
- Comprobacion cada 30 s (`SMRT_SCHED_CHECK_INTERVAL`)
- Persistencia en NVS (namespace `sched`, claves `task_0`..`task_7`)

### Formato de tarea

```json
{
  "index": 0,
  "enabled": true,
  "hour": 7,
  "minute": 30,
  "days": 62,
  "action": "rly_toggle:0",
  "name": "Encender luz"
}
```

**Bitmask de dias (`days`):**

| Bit | Dia | Valor |
|-----|-----|-------|
| 0 | Domingo | 0x01 (1) |
| 1 | Lunes | 0x02 (2) |
| 2 | Martes | 0x04 (4) |
| 3 | Miercoles | 0x08 (8) |
| 4 | Jueves | 0x10 (16) |
| 5 | Viernes | 0x20 (32) |
| 6 | Sabado | 0x40 (64) |

Ejemplos comunes:
- Todos los dias: `127` (0x7F)
- Lunes a viernes: `62` (0x3E)
- Fin de semana: `65` (0x41)
- Solo lunes: `2` (0x02)

### Formato de accion

La accion sigue el patron `<comando_ws>:<arg1>:<arg2>...`:

| Ejemplo | Descripcion |
|---------|-------------|
| `rly_toggle:0` | Toggle rele 0 |
| `rly_set:1:1` | Encender rele 1 |
| `rly_set:1:0` | Apagar rele 1 |
| `env_read` | Leer sensor ambiental |
| `plg_set:1` | Encender enchufe |

### Comandos WS del scheduler

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `sched_list` | — | Lista todas las tareas (0-7) |
| `sched_set` | Objeto tarea completo | Crea o actualiza tarea |
| `sched_delete` | `index` | Elimina tarea |
| `sched_status` | — | Numero de tareas activas y hora actual |
| `time_set_tz` | `gmt_offset`, `dst_offset` | Configura zona horaria |

### Ejemplo: crear tarea

```json
{
  "cmd": "sched_set",
  "index": 0,
  "enabled": true,
  "hour": 7,
  "minute": 30,
  "days": 62,
  "action": "rly_toggle:0",
  "name": "Despertar"
}
```

---

## 16. MQTT — Integracion con Brokers

**Requisito:** `-D SMRT_MQTT` + descomentrar `knolleary/PubSubClient@^2.8` en `platformio.ini`.

### Topicos

| Topico | Sentido | Descripcion |
|--------|---------|-------------|
| `homenode/<hostname>/telemetry` | OUT | Telemetria completa cada 30 s |
| `homenode/<hostname>/event/<tipo>` | OUT | Evento puntual (SEC, ACC, ENV, RLY, NRG) |
| `homenode/<hostname>/cmd` | IN | Comandos entrantes (mismo formato WS) |

El `<hostname>` por defecto es el nombre mDNS configurado (por defecto `homenode`).

### Configuracion MQTT

```json
{
  "cmd": "mqtt_config",
  "server": "192.168.1.10",
  "port": 1883,
  "user": "usuario_mqtt",
  "pass": "contraseña_mqtt"
}
```

```json
{ "cmd": "mqtt_enable", "enabled": true }
{ "cmd": "mqtt_status" }
```

Las credenciales MQTT se almacenan cifradas en NVS (si `SMRT_CRYPTO`).

### Reconexion

El cliente usa backoff exponencial: inicio 1 s, maximo 60 s. No bloquea el loop principal.

### Integracion con Home Assistant

Ejemplo de configuracion `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "HomeNode Temperatura"
      state_topic: "homenode/salon/telemetry"
      value_template: "{{ value_json.modules.env.temperature }}"
      unit_of_measurement: "°C"
    - name: "HomeNode Humedad"
      state_topic: "homenode/salon/telemetry"
      value_template: "{{ value_json.modules.env.humidity }}"
      unit_of_measurement: "%"
  switch:
    - name: "HomeNode Rele 0"
      command_topic: "homenode/salon/cmd"
      payload_on: '{"cmd":"rly_set","index":0,"state":1}'
      payload_off: '{"cmd":"rly_set","index":0,"state":0}'
      state_topic: "homenode/salon/telemetry"
      value_template: "{{ value_json.modules.rly.states[0] }}"
      payload_available: "true"
      payload_not_available: "false"
```

---

## 17. Webhooks — Notificaciones HTTP

**Requisito:** `-D SMRT_WEBHOOK`

Permite enviar notificaciones POST a hasta 4 endpoints HTTP cuando ocurren eventos.

### Payload de webhook

```json
{
  "event": "sec_triggered",
  "hostname": "salon",
  "uptime": 125400,
  "ip": "192.168.1.100",
  "payload": { "state": "triggered" }
}
```

### Comandos WS

| Comando | Campos | Descripcion |
|---------|--------|-------------|
| `webhook_set` | `index` (0-3), `url`, `filter` | Configura webhook |
| `webhook_delete` | `index` (0-3) | Elimina webhook |
| `webhook_list` | — | Lista webhooks configurados |
| `webhook_test` | `index` (0-3) | Envia payload de prueba |

El campo `filter` es un bitmask de eventos `SMRT_EVT_*` (0xFF para todos los eventos).

### Ejemplo: configurar webhook

```json
{
  "cmd": "webhook_set",
  "index": 0,
  "url": "http://192.168.1.20:1880/homenode",
  "filter": 255
}
```

### Reintentos

Cada webhook tiene `SMRT_WEBHOOK_RETRY_COUNT` (2) reintentos con timeout de `SMRT_WEBHOOK_TIMEOUT_MS` (5 s) por intento.

---

## 18. Backup y Restauracion de Configuracion

### Exportar configuracion

```json
{ "cmd": "cfg_export" }
```

Respuesta: JSON con todas las claves NVS conocidas. Los campos sensibles (passwords, PIN) se muestran como `"***"`.

### Importar configuracion

La importacion usa una proteccion de doble confirmacion para evitar sobreescrituras accidentales:

```json
// 1. Primera confirmacion (inicia ventana de 10s)
{ "cmd": "cfg_import", "data": { ... } }

// 2. Segunda confirmacion dentro de 10s (ejecuta la importacion)
{ "cmd": "cfg_import", "data": { ... } }
```

Si la segunda confirmacion no llega en 10 s, el estado se resetea. El dispositivo reinicia automaticamente tras una importacion exitosa. Los campos sensibles son ignorados durante la importacion.

---

## 19. Actualizacion OTA

### Via ArduinoOTA (PlatformIO)

```bash
pio run -e ota --target upload
```

Requiere el password definido en `SMRT_OTA_PASSWORD`. El dispositivo debe estar en la misma red local y el puerto 3232 debe ser accesible.

### Via HTTP (`/update`)

1. Acceder a `http://<ip>/update` en el navegador
2. Autenticarse (usuario: `admin`, password: `SMRT_AUTH_OTA_PASS`)
3. Subir el archivo `.bin` generado por PlatformIO (en `.pio/build/nodemcu-32s/firmware.bin`)

### Generacion del binario

```bash
pio run -e nodemcu-32s
# Binario en: .pio/build/nodemcu-32s/firmware.bin
```

---

## 20. Persistencia NVS

### Namespaces por modulo

| Namespace | Contenido |
|-----------|-----------|
| `smrt_cfg` | WiFi credentials, hostname, PIN |
| `time` | Timezone offsets (gmt_off, dst_off) |
| `sched` | Tasks task_0 .. task_7 |
| `mqtt` | Server, port, user, pass, enabled |
| `webhook` | URLs y filtros (hook_0 .. hook_3) |
| `env` | Intervalo de lectura, umbrales de alerta |
| `rly` | Etiquetas de reles |
| `sec` | Delays, configuracion |
| `acc` | UIDs autorizados (uid_0 .. uid_19), pulse_ms |
| `plg` | Estado persistido |
| `nrg` | Limites de potencia, calibracion |

### Limitaciones NVS

- La flash NVS tiene un ciclo de vida de ~100.000 escrituras por celda
- El write throttling interno (`SMRT_NVS_WRITE_INTERVAL_MS` = 5 min) protege la flash
- Los cambios de estado efimeros (`SMRT_NVS_STATE_DEBOUNCE_MS` = 5 s) tienen su propio throttle

---

## 21. Tests Unitarios y QA

### Ejecutar tests

```bash
# Todos los tests nativos (sin hardware)
pio test -e native

# Solo un suite de tests
pio test -e native -f test_core_sched

# Verbose (muestra cada test)
pio test -e native -v
```

### Suites de tests disponibles

| Suite | Tests aprox. | Modulo/Componente |
|-------|-------------|-------------------|
| `test_mc_format` | ~40 | Tipos de datos y conversion |
| `test_core_module` | ~20 | Sistema de registro de modulos |
| `test_core_sched` | ~35 | Scheduler (NUEVO v0.8.0) |
| `test_mod_env` | ~32 | Modulo ENV + alertas (NUEVO v0.8.0) |
| `test_mod_sec` | ~45 | Modulo SEC y maquina de estados |
| `test_mod_plg` | ~30 | Modulo PLG |
| `test_mod_nrg` | ~40 | Modulo NRG |
| `test_mod_rly` | ~35 | Modulo RLY |
| `test_mod_acc` | ~50 | Modulo ACC y gestion UIDs |

### Script QA completo

```bash
python smrt_qa.py
```

Genera en `reports/<timestamp>/`:
- `test_results.txt` — Resultados Unity
- `static_analysis.txt` — Analisis cppcheck
- `metrics.txt` — LOC, funciones, complejidad ciclomatica
- `summary.txt` — Resumen consolidado
- `changelog.md` — Registro de cambios

### Criterios de aceptacion

| Criterio | Umbral |
|----------|--------|
| Tests unitarios | 100% PASS |
| Errores cppcheck | 0 errores |
| Complejidad ciclomatica | <= 10 (simple), <= 20 (moderado) |
| Lineas por funcion | <= 50 |

---

## 22. Mapa de Pines y Conflictos de Hardware

### Pines utilizados por modulo

| GPIO | Modulo | Funcion | Conflicto con |
|------|--------|---------|---------------|
| 2 | ACC / PLG | Lock relay / Power relay | ACC vs PLG (exclusivos) |
| 4 | ENV | DHT22 data | — |
| 12 | SEC | PIR sensor | ACC (si usa Touch12) |
| 13 | SEC | Reed switch | — |
| 14 | SEC | Vibration sensor | — |
| 25 | SEC | Buzzer | — |
| 26-29 | RLY | Reles 0-3 | — |
| 34 | NRG/PLG | ADC canal 0 (SCT-013) | NRG vs PLG |
| 35 | NRG | ADC canal 1 | NRG vs PLG |
| 18 | ACC | SPI SCK (MFRC522) | — |
| 19 | ACC | SPI MISO (MFRC522) | — |
| 23 | ACC | SPI MOSI (MFRC522) | — |
| 5 | ACC | SPI CS (MFRC522) | — |
| 21 | I2C | SDA (bus I2C general) | — |
| 22 | I2C | SCL (bus I2C general) | — |

### Combinaciones validas de modulos

| Combinacion | Estado |
|-------------|--------|
| ENV + RLY + SEC | OK (configuracion por defecto) |
| ENV + RLY + ACC | OK (si no se usa SEC) |
| ENV + RLY + PLG | OK |
| ENV + RLY + NRG | OK |
| ENV + RLY + SEC + ACC | CONFLICTO: GPIO2 y posiblemente GPIO12-14 |
| NRG + PLG | CONFLICTO: GPIO34-39 y GPIO2 |

---

## 23. Referencia Rapida de Comandos WS

### Autenticacion y red

```json
{ "cmd": "auth", "pin": "1234" }
{ "cmd": "status" }
{ "cmd": "wifi", "ssid": "Red", "pass": "Pass", "pin": "1234" }
{ "cmd": "set_hostname", "hostname": "salon" }
```

### Modulos

```json
{ "cmd": "env_read" }
{ "cmd": "env_set_interval", "value": 10000 }
{ "cmd": "env_set_alert", "temp_hi": 35.0, "temp_lo": 5.0, "hum_hi": 90.0, "hum_lo": 15.0, "enabled": true }

{ "cmd": "rly_toggle", "index": 0 }
{ "cmd": "rly_set", "index": 0, "state": 1 }
{ "cmd": "rly_pulse", "index": 0, "ms": 500 }

{ "cmd": "sec_arm" }
{ "cmd": "sec_disarm" }
{ "cmd": "sec_status" }

{ "cmd": "acc_learn" }
{ "cmd": "acc_add_uid", "uid": "AB:CD:EF:12" }
{ "cmd": "acc_remove_uid", "uid": "AB:CD:EF:12" }
{ "cmd": "acc_list_uids" }
```

### Scheduler

```json
{ "cmd": "sched_list" }
{ "cmd": "sched_set", "index": 0, "enabled": true, "hour": 7, "minute": 30, "days": 62, "action": "rly_toggle:0", "name": "Despertar" }
{ "cmd": "sched_delete", "index": 0 }
{ "cmd": "sched_status" }
{ "cmd": "time_set_tz", "gmt_offset": 3600, "dst_offset": 3600 }
```

### MQTT y Webhooks

```json
{ "cmd": "mqtt_config", "server": "192.168.1.10", "port": 1883, "user": "u", "pass": "p" }
{ "cmd": "mqtt_enable", "enabled": true }
{ "cmd": "mqtt_status" }

{ "cmd": "webhook_set", "index": 0, "url": "http://192.168.1.20:1880/hnode", "filter": 255 }
{ "cmd": "webhook_list" }
{ "cmd": "webhook_test", "index": 0 }
{ "cmd": "webhook_delete", "index": 0 }
```

### Backup

```json
{ "cmd": "cfg_export" }
{ "cmd": "cfg_import", "data": { ... } }
```

---

## 24. Historial de Versiones

| Version | Fecha | Cambios principales |
|---------|-------|---------------------|
| 0.1.0 | 2026-03-12 | Estructura inicial, HAL, modulo ENV |
| 0.2.0 | 2026-03-12 | Sistema de modulos, WebSocket, NVS |
| 0.3.0 | 2026-03-13 | Auth PIN, sesiones WS, rate limiting |
| 0.4.0 | 2026-03-13 | Modulo ACC, tests unitarios, cppcheck |
| 0.5.0 | 2026-03-13 | Modulo SEC (maquina de estados) |
| 0.6.0 | 2026-03-13 | Modulos NRG, PLG, refactorizacion |
| 0.7.0 | 2026-03-16 | Auditoria seguridad, filtrado IP LAN, OTA mejorado |
| 0.8.0 | 2026-03-17 | NVS cifrado AES-128, NTP, Scheduler, Bus de eventos, MQTT, Webhooks, Backup, Alertas ENV, Learn mode ACC, Power factor NRG |
