# HomeNode v0.4.0 — Guia de Usuario

> **Plataforma IoT domestica modular para ESP32**
> Fecha: 2026-03-14 | Version: 0.4.0

---

## Tabla de Contenidos

1. [Introduccion](#1-introduccion)
2. [Requisitos de Hardware](#2-requisitos-de-hardware)
3. [Instalacion y Configuracion Inicial](#3-instalacion-y-configuracion-inicial)
4. [Combinaciones de Modulos y Conflictos de Pines](#4-combinaciones-de-modulos-y-conflictos-de-pines)
5. [Interfaz Web (Dashboard)](#5-interfaz-web-dashboard)
6. [Protocolo WebSocket](#6-protocolo-websocket)
7. [Modulo ENV — Ambiente](#7-modulo-env--ambiente)
8. [Modulo RLY — Reles](#8-modulo-rly--reles)
9. [Modulo SEC — Seguridad](#9-modulo-sec--seguridad)
10. [Modulo PLG — Enchufe Inteligente](#10-modulo-plg--enchufe-inteligente)
11. [Modulo NRG — Monitor de Energia](#11-modulo-nrg--monitor-de-energia)
12. [Modulo ACC — Control de Acceso](#12-modulo-acc--control-de-acceso)
13. [Persistencia NVS](#13-persistencia-nvs)
14. [Actualizacion OTA](#14-actualizacion-ota)
15. [Desarrollo y Tests](#15-desarrollo-y-tests)
16. [Referencia Rapida de Comandos](#16-referencia-rapida-de-comandos)

---

## 1. Introduccion

**HomeNode** es una plataforma IoT modular disenada para automatizacion domestica sobre hardware ESP32. Permite combinar hasta 6 modulos funcionales independientes en un solo dispositivo, activados selectivamente mediante flags de compilacion.

### Caracteristicas principales

- **Arquitectura modular**: 6 modulos independientes (ENV, RLY, SEC, PLG, NRG, ACC)
- **Interfaz web responsive**: Dashboard accesible desde navegador (movil/PC)
- **Comunicacion en tiempo real**: WebSocket con telemetria automatica cada 5 segundos
- **Persistencia**: Configuracion almacenada en NVS (Non-Volatile Storage) del ESP32
- **Actualizacion remota**: OTA via WiFi (ArduinoOTA + HTTP upload)
- **Tests unitarios nativos**: 331 tests ejecutables en PC sin hardware ESP32

### Arquitectura de 3 capas

```
┌─────────────────────────────────────┐
│          MODULES (capa 3)           │
│  ENV | RLY | SEC | PLG | NRG | ACC │
├─────────────────────────────────────┤
│           CORE (capa 2)            │
│  WiFi | HTTP | WS | NVS | OTA     │
│  Module Registry | Dispatch        │
├─────────────────────────────────────┤
│            HAL (capa 1)            │
│  GPIO | Serial | I2C | SPI | ADC  │
└─────────────────────────────────────┘
```

---

## 2. Requisitos de Hardware

### Placa base

| Componente | Especificacion |
|------------|---------------|
| Microcontrolador | ESP32-WROOM-32 (NodeMCU-32S) |
| Flash | 4 MB |
| RAM | 520 KB SRAM |
| WiFi | 802.11 b/g/n 2.4 GHz |
| Alimentacion | 5V via USB o fuente externa |

### Componentes por modulo

| Modulo | Componentes | Pines GPIO |
|--------|------------|------------|
| **ENV** | Sensor DHT22/AM2302 | GPIO4 (data) |
| **RLY** | 1-4 reles mecanicos/SSR | GPIO16, 17, 25, 26 |
| **SEC** | PIR HC-SR501 + Reed switch + Sensor vibracion + Buzzer | GPIO12, 13, 14, 25 |
| **PLG** | Rele + ACS712-30A + ZMPT101B | GPIO2, 34, 35 |
| **NRG** | 1-4x ACS712-30A + 1-2x ZMPT101B | GPIO34, 35, 36, 39, 32, 33 |
| **ACC** | MFRC522 (NFC/RFID) + Rele cerradura | GPIO5, 18, 19, 23, 27, 2 |

### Esquema de pines ESP32 completo

```
                    ESP32 NodeMCU-32S
                  ┌───────────────────┐
           3.3V ─┤                     ├─ VIN (5V)
            GND ─┤                     ├─ GND
  GPIO15 (CAP3)─┤                     ├─ GPIO13 (SEC Reed / CAP4)
   GPIO2 (LED) ─┤ PLG Relay / ACC Lock├─ GPIO12 (SEC PIR / CAP5)
   GPIO4 (ENV) ─┤                     ├─ GPIO14 (SEC Vibr / CAP6)
  GPIO16 (RLY1)─┤                     ├─ GPIO27 (ACC RST / IO8)
  GPIO17 (RLY2)─┤                     ├─ GPIO26 (RLY4 / IO7)
  GPIO5 (ACC SS)─┤                     ├─ GPIO25 (RLY3 / SEC Buzz / IO6)
  GPIO18 (ACC SCK)─┤                  ├─ GPIO33 (NRG Ch3 I / CAP8)
  GPIO19 (ACC MISO)─┤                 ├─ GPIO32 (NRG Ch2 I / CAP9)
  GPIO21 (I2C SDA)─┤                  ├─ GPIO35 (PLG/NRG V ADC)
  GPIO3 (RX) ─┤                       ├─ GPIO34 (PLG/NRG I ADC)
  GPIO1 (TX) ─┤                       ├─ GPIO39 (NRG Ch1 I ADC)
  GPIO22 (I2C SCL)─┤                  ├─ GPIO36 (NRG Ch1 V ADC)
  GPIO23 (ACC MOSI)─┤                 ├─ EN
                  └───────────────────┘
```

---

## 3. Instalacion y Configuracion Inicial

### 3.1 Preparar el entorno

1. Instalar [Visual Studio Code](https://code.visualstudio.com/)
2. Instalar la extension **PlatformIO IDE** desde el marketplace
3. Clonar el repositorio:
   ```bash
   git clone <url-repositorio> HomeNode
   cd HomeNode
   ```

### 3.2 Configurar la red

Editar `include/core/smrt_core_config.h`:

```c
// Cambiar estos valores para tu red local
#define SMRT_STATIC_IP      192, 168, 1, 100   // IP fija del ESP32
#define SMRT_GATEWAY_IP     192, 168, 1, 1     // IP del router
#define SMRT_SUBNET_MASK    255, 255, 255, 0   // Mascara de subred
#define SMRT_DNS_IP         8, 8, 8, 8         // Servidor DNS
```

### 3.3 Configurar credenciales WiFi

Las credenciales se pueden configurar de dos formas:

**Opcion A — Codigo fuente** (primera vez):
En `src/core/smrt_core_wifi.cpp`, cambiar los fallback:
```c
static const char *SMRT_WIFI_FALLBACK_SSID = "TU_RED_WIFI";
static const char *SMRT_WIFI_FALLBACK_PASS = "TU_PASSWORD";
```

**Opcion B — WebUI** (despues del primer arranque):
Usar la tarjeta "Configuracion WiFi" del dashboard (requiere PIN).

### 3.4 Seleccionar modulos

En `platformio.ini`, seccion `[env_common_esp32]`, activar/desactivar con `-D`:

```ini
build_flags =
    -I include
    -I include/core
    -I include/modules
    -D SMRT_PLATFORM_HOMENODE
    -D SMRT_MOD_ENV          ; Ambiente (DHT22)
    -D SMRT_MOD_RLY          ; Reles (1-4)
    -D SMRT_MOD_SEC          ; Seguridad (alarma)
    ; -D SMRT_MOD_PLG        ; Enchufe inteligente (exclusivo con NRG/ACC)
    ; -D SMRT_MOD_NRG        ; Monitor energia (exclusivo con PLG)
    ; -D SMRT_MOD_ACC        ; Control acceso NFC (exclusivo con PLG)
    -std=c++11
```

> **IMPORTANTE:** Respetar los conflictos de pines (ver seccion 4).

### 3.5 Compilar y flashear

**Via USB:**
```bash
pio run -e nodemcu-32s -t upload
```

**Via OTA (despues del primer flash por USB):**
```bash
pio run -e ota -t upload
```

### 3.6 Verificar funcionamiento

1. Abrir el monitor serial: `pio device monitor`
2. Verificar el mensaje de arranque:
   ```
   ========================================
    HOMENODE v0.4.0
    IoT Modular Platform
   ========================================
   Modules registered: X
   WiFi connected. IP: 192.168.1.100
   HTTP server started on port 80
   Setup complete. Entering main loop.
   ```
3. Acceder al dashboard: `http://192.168.1.100`

---

## 4. Combinaciones de Modulos y Conflictos de Pines

### Conflictos conocidos

| Pin GPIO | Modulo A | Modulo B | Tipo |
|----------|----------|----------|------|
| GPIO25 | RLY (Relay 3) | SEC (Buzzer) | Mutuamente excluyentes |
| GPIO2 | PLG (Relay) | ACC (Lock relay) | Mutuamente excluyentes |
| GPIO34-39 | PLG (corriente/voltaje) | NRG (canales) | Mutuamente excluyentes |

### Combinaciones validas

| Combinacion | Modulos | Uso tipico |
|-------------|---------|------------|
| **Hogar basico** | ENV + RLY | Temperatura + control de luces/riego |
| **Seguridad** | ENV + SEC | Temperatura + alarma con sensores |
| **Hogar + seguridad** | ENV + RLY(1-2) + SEC | Sin relay 3 (GPIO25 para buzzer) |
| **Enchufe inteligente** | ENV + PLG | Temperatura + monitoreo energetico |
| **Monitor energetico** | ENV + NRG | Temperatura + multiples circuitos |
| **Control de acceso** | ENV + ACC | Temperatura + cerradura NFC |
| **Acceso + seguridad** | ENV + SEC + ACC | Alarma + NFC (sin relay 3) |
| **Completo (sin energia)** | ENV + RLY(1-2) + SEC + ACC | Excluye PLG y NRG |

> **Regla general:** PLG, NRG y ACC(lock) compiten por GPIOs. Solo activar uno del grupo PLG/NRG y cuidar GPIO2 entre PLG/ACC.

---

## 5. Interfaz Web (Dashboard)

### Acceso

- **URL:** `http://192.168.1.100` (o la IP configurada)
- **Puerto:** 80 (HTTP)
- **Compatibilidad:** Cualquier navegador moderno (Chrome, Firefox, Safari, Edge)
- **Responsive:** Adaptado para pantallas moviles (breakpoint 480px)

### Tarjetas del dashboard

1. **Conexion** — Indicador de estado WebSocket (verde=conectado, naranja=reconectando, rojo=desconectado) + IP
2. **Sistema** — Uptime (HH:MM:SS), RSSI WiFi (dBm + barra visual), Red WiFi (SSID), Clientes WebSocket
3. **Modulos** — Tarjetas especificas por modulo habilitado (actualmente solo ENV tiene tarjeta WebUI)
4. **Configuracion WiFi** — Panel desplegable con campos PIN + SSID + Password
5. **Mantenimiento** — Enlace a pagina de actualizacion OTA (`/update`)

### Configurar WiFi desde el dashboard

1. Click en "Abrir Ajustes" en la tarjeta WiFi
2. Introducir el **PIN de acceso** (default: `1234`)
3. Introducir nuevo SSID y password
4. Click en "Guardar Credenciales"
5. El dispositivo se reiniciara automaticamente en 3 segundos

---

## 6. Protocolo WebSocket

### Conexion

- **Endpoint:** `ws://192.168.1.100/ws`
- **Reconexion automatica:** El dashboard reconecta cada 2 segundos si se pierde la conexion
- **Broadcast de telemetria:** Cada 5 segundos (configurable via `SMRT_STATUS_INTERVAL_MS`)

### Formato de mensajes

Todos los mensajes son **JSON**. El campo `"cmd"` indica el comando:

```json
{"cmd": "nombre_comando", ...parametros}
```

### Comandos core

| Comando | Descripcion | Ejemplo |
|---------|------------|---------|
| `status` | Solicitar telemetria completa | `{"cmd": "status"}` |
| `wifi` | Cambiar credenciales WiFi | `{"cmd": "wifi", "pin": "1234", "ssid": "MiRed", "pass": "MiPass"}` |

### Comandos de modulo

Los comandos de modulo usan el formato **`{id}_{subcomando}`**:

```json
{"cmd": "env_read"}           // Modulo ENV, sub-comando "read"
{"cmd": "rly_toggle", "index": 0}  // Modulo RLY, sub-comando "toggle"
{"cmd": "sec_arm"}            // Modulo SEC, sub-comando "arm"
```

El sistema de dispatch extrae el prefijo del ID del modulo, busca el modulo registrado y llama a su handler con el sub-comando restante.

### Estructura de telemetria

```json
{
  "rssi": -45,
  "uptime": 123456,
  "ip": "192.168.1.100",
  "clients": 2,
  "ssid": "MiRed",
  "modules": {
    "env": { "temperature": 24.5, "humidity": 55.2, "ok": 1 },
    "rly": { "count": 2, "states": [1, 0], "names": ["Luz", "Bomba"], "pulse_ms": 3000 },
    "sec": { "state": 0, "pir": 0, "reed": 1, "vibration": 0, "entry_delay": 30000, "exit_delay": 30000, "events": 3 }
  }
}
```

---

## 7. Modulo ENV — Ambiente

### Descripcion

Monitorea temperatura y humedad ambiental usando un sensor DHT22/AM2302.

### Hardware

| Componente | Pin | Descripcion |
|------------|-----|------------|
| DHT22 data | GPIO4 | Linea de datos del sensor |
| DHT22 VCC | 3.3V | Alimentacion |
| DHT22 GND | GND | Tierra |

> Recomendacion: Colocar una resistencia pull-up de 10K entre data y VCC.

### Comandos WebSocket

| Comando | Campos | Descripcion |
|---------|--------|------------|
| `env_read` | — | Forzar lectura inmediata del sensor |
| `env_set_interval` | `"value": ms` | Cambiar intervalo de lectura (2000-60000 ms) |

**Ejemplos:**
```json
{"cmd": "env_read"}
{"cmd": "env_set_interval", "value": 10000}
```

### Telemetria

```json
{
  "temperature": 24.5,    // Temperatura en Celsius
  "humidity": 55.2,       // Humedad relativa en %
  "ok": 1                 // 1=lectura exitosa, 0=error del sensor
}
```

### Configuracion

| Parametro | Rango | Default | NVS Key |
|-----------|-------|---------|---------|
| Intervalo de lectura | 2000-60000 ms | 5000 ms | `env/read_intv` |
| Offset temperatura | — | 0.0 C | Compilacion |
| Offset humedad | — | 0.0 % | Compilacion |

---

## 8. Modulo RLY — Reles

### Descripcion

Controla de 1 a 4 salidas de rele con soporte para toggle, set, pulso temporizado y nombres personalizados.

### Hardware

| Rele | Pin | Nota |
|------|-----|------|
| Relay 1 | GPIO16 | — |
| Relay 2 | GPIO17 | — |
| Relay 3 | GPIO25 | Compartido con SEC buzzer |
| Relay 4 | GPIO26 | — |

> Los reles se activan con nivel HIGH. Usar modulos de rele con optoacoplador para cargas AC.

### Comandos WebSocket

| Comando | Campos | Descripcion |
|---------|--------|------------|
| `rly_toggle` | `"index": n` | Invertir estado del rele n (0-3) |
| `rly_set` | `"index": n, "state": 0/1` | Establecer estado del rele n |
| `rly_set_all` | `"states": [1,0,1,0]` | Establecer todos los reles |
| `rly_pulse` | `"index": n` | Activar rele n por tiempo de pulso |
| `rly_set_pulse` | `"value": ms` | Cambiar duracion de pulso (100-30000 ms) |
| `rly_set_count` | `"value": n` | Cambiar cantidad de reles activos (1-4) |
| `rly_set_name` | `"index": n, "name": "str"` | Asignar nombre al rele n (max 16 chars) |
| `rly_status` | — | Solicitar estado completo |

**Ejemplos:**
```json
{"cmd": "rly_toggle", "index": 0}
{"cmd": "rly_set", "index": 1, "state": 1}
{"cmd": "rly_pulse", "index": 0}
{"cmd": "rly_set_name", "index": 0, "name": "Luz Salon"}
```

### Telemetria

```json
{
  "count": 2,
  "states": [1, 0],
  "names": ["Luz Salon", "Bomba"],
  "pulse_ms": 3000
}
```

### Configuracion

| Parametro | Rango | Default | NVS Key |
|-----------|-------|---------|---------|
| Cantidad de reles | 1-4 | 1 | `rly/count` |
| Duracion de pulso | 100-30000 ms | 3000 ms | `rly/pulse_ms` |
| Estado de reles | bitmask | 0 | `rly/states` |
| Nombre rele N | max 16 chars | "" | `rly/name_N` |

### Modo pulso

El modo pulso activa un rele por la duracion configurada y luego lo desactiva automaticamente. Es non-blocking (usa `millis()`) y no interfiere con el loop principal.

---

## 9. Modulo SEC — Seguridad

### Descripcion

Sistema de alarma con sensores PIR (movimiento), reed switch (puerta/ventana), vibracion y buzzer. Implementa una maquina de estados con delays de entrada/salida configurables.

### Hardware

| Componente | Pin | Tipo |
|------------|-----|------|
| PIR HC-SR501 | GPIO12 | Input (pulldown) |
| Reed switch | GPIO13 | Input (pullup) |
| Sensor vibracion | GPIO14 | Input |
| Buzzer | GPIO25 | Output (compartido con RLY3) |

### Maquina de estados

```
                    ┌─────────┐
                    │DISARMED │ ◄──── DISARM (cualquier estado)
                    │  (0)    │
                    └────┬────┘
                         │ ARM
                    ┌────▼────┐
                    │EXIT_DELAY│
                    │  (4)    │
                    └────┬────┘
                         │ TIMEOUT
                    ┌────▼────┐
        MOTION ───► │ ARMED   │ ◄──── DOOR_OPEN
                    │  (1)    │
                    └────┬────┘
                         │ MOTION / DOOR_OPEN
                    ┌────▼─────┐
                    │ENTRY_DELAY│
                    │  (3)     │
                    └────┬─────┘
                         │ TIMEOUT
                    ┌────▼─────┐
                    │TRIGGERED │
                    │  (2)     │
                    └──────────┘
```

**Flujo tipico:**
1. El usuario envia `sec_arm` → el sistema entra en EXIT_DELAY
2. Tras el timeout de salida → el sistema pasa a ARMED
3. Si se detecta movimiento o puerta abierta → ENTRY_DELAY
4. Si no se desarma antes del timeout de entrada → TRIGGERED (alarma!)
5. `sec_disarm` desde cualquier estado → vuelve a DISARMED

### Comandos WebSocket

| Comando | Campos | Descripcion |
|---------|--------|------------|
| `sec_arm` | — | Armar sistema (inicia EXIT_DELAY) |
| `sec_disarm` | — | Desarmar sistema |
| `sec_status` | — | Solicitar estado completo |
| `sec_set_entry_delay` | `"value": ms` | Cambiar delay de entrada (5000-120000 ms) |
| `sec_set_exit_delay` | `"value": ms` | Cambiar delay de salida (5000-120000 ms) |
| `sec_get_events` | — | Obtener log de eventos |
| `sec_clear_events` | — | Limpiar log de eventos |

**Ejemplos:**
```json
{"cmd": "sec_arm"}
{"cmd": "sec_disarm"}
{"cmd": "sec_set_entry_delay", "value": 15000}
```

### Telemetria

```json
{
  "state": 1,              // 0=DISARMED, 1=ARMED, 2=TRIGGERED, 3=ENTRY_DELAY, 4=EXIT_DELAY
  "pir": 0,                // Lectura PIR (0/1)
  "reed": 1,               // Lectura reed switch (1=cerrado, 0=abierto)
  "vibration": 0,          // Lectura sensor vibracion (0/1)
  "entry_delay": 30000,    // Delay de entrada configurado (ms)
  "exit_delay": 30000,     // Delay de salida configurado (ms)
  "events": 5              // Cantidad de eventos en el log
}
```

### Alertas en tiempo real

Cuando se detecta un trigger, el modulo envia un mensaje inmediato a todos los clientes:

```json
{
  "type": "sec_alert",
  "trigger": "motion",      // "motion", "door", "vibration"
  "state": 3,               // Estado actual del sistema
  "timestamp": 45230        // millis() del evento
}
```

### Configuracion

| Parametro | Rango | Default | NVS Key |
|-----------|-------|---------|---------|
| Delay de entrada | 5000-120000 ms | 30000 ms | `sec/entry_dly` |
| Delay de salida | 5000-120000 ms | 30000 ms | `sec/exit_dly` |
| Estado armado | 0/1 | 0 | `sec/armed` |

### Log de eventos

Buffer circular de 16 entradas. Cada evento tiene un mensaje (max 32 chars) y un timestamp. El log no persiste tras reinicio.

---

## 10. Modulo PLG — Enchufe Inteligente

### Descripcion

Enchufe inteligente con control de rele y monitoreo de consumo electrico en tiempo real. Incluye sensores de corriente (ACS712-30A) y voltaje (ZMPT101B) con proteccion por sobrecarga.

### Hardware

| Componente | Pin | Descripcion |
|------------|-----|------------|
| Rele de potencia | GPIO2 | Control ON/OFF del enchufe |
| ACS712-30A | GPIO34 | Sensor de corriente (ADC input-only) |
| ZMPT101B | GPIO35 | Sensor de voltaje (ADC input-only) |

> **IMPORTANTE:** GPIO34-39 son solo entrada (input-only, sin pullup interno). GPIO2 es compartido con el LED integrado y con ACC lock.

### Comandos WebSocket

| Comando | Campos | Descripcion |
|---------|--------|------------|
| `plg_toggle` | — | Invertir estado del rele |
| `plg_set` | `"state": 0/1` | Establecer estado del rele |
| `plg_status` | — | Solicitar estado completo |
| `plg_set_interval` | `"value": ms` | Cambiar intervalo de lectura (1000-60000 ms) |
| `plg_set_overload` | `"value": amps` | Cambiar umbral de sobrecarga (1.0-30.0 A) |
| `plg_reset_energy` | — | Resetear acumulador de energia a 0 |

**Ejemplos:**
```json
{"cmd": "plg_toggle"}
{"cmd": "plg_set", "state": 1}
{"cmd": "plg_set_overload", "value": 10.0}
{"cmd": "plg_reset_energy"}
```

### Telemetria

```json
{
  "state": 1,              // 1=ON, 0=OFF
  "voltage": 220.5,        // Voltaje RMS (V)
  "current": 2.3,          // Corriente RMS (A)
  "power": 507.2,          // Potencia (W)
  "energy": 1234.5,        // Energia acumulada (Wh)
  "overload": 15.0,        // Umbral de sobrecarga (A)
  "interval": 2000         // Intervalo de lectura (ms)
}
```

### Proteccion por sobrecarga

Si la corriente medida supera el umbral configurado, el rele se desactiva automaticamente. El sistema reporta esta accion via telemetria (el campo `state` cambiara a 0).

### Calibracion

Los sensores requieren calibracion especifica para cada instalacion:

| Parametro | Default | Archivo |
|-----------|---------|---------|
| Sensibilidad ACS712 | 0.066 V/A (modelo 30A) | `smrt_mod_plg_config.h` |
| Ratio ZMPT101B | 234.26 | `smrt_mod_plg_config.h` |
| Muestras ADC | 100 | `smrt_mod_plg_config.h` |

> Para calibrar, medir con un multimetro y ajustar `SMRT_PLG_ACS712_SENS` y `SMRT_PLG_ZMPT_RATIO`.

### Configuracion

| Parametro | Rango | Default | NVS Key |
|-----------|-------|---------|---------|
| Intervalo lectura | 1000-60000 ms | 2000 ms | `plg/intv` |
| Umbral sobrecarga | 1.0-30.0 A | 15.0 A | `plg/overload` |
| Estado rele | 0/1 | 0 | `plg/state` |
| Energia acumulada | float (Wh) | 0.0 | `plg/kwh` |

---

## 11. Modulo NRG — Monitor de Energia

### Descripcion

Monitor de energia multi-canal (hasta 4 canales). Cada canal mide voltaje, corriente, potencia real, potencia aparente, factor de potencia y energia acumulada. Usa suavizado por media movil.

### Hardware

| Canal | Pin Voltaje | Pin Corriente | Nota |
|-------|------------|---------------|------|
| CH0 | GPIO34 | GPIO35 | Canal principal |
| CH1 | GPIO36 | GPIO39 | Canal independiente |
| CH2 | — (usa V de CH0) | GPIO32 | Comparte voltaje con CH0 |
| CH3 | — (usa V de CH0) | GPIO33 | Comparte voltaje con CH0 |

> Canales 2 y 3 no tienen pin de voltaje propio. Asumen el mismo voltaje medido en el canal 0.

### Comandos WebSocket

| Comando | Campos | Descripcion |
|---------|--------|------------|
| `nrg_status` | — | Solicitar estado de todos los canales |
| `nrg_read` | `"channel": n` | Leer un canal especifico (0-3) |
| `nrg_set_interval` | `"value": ms` | Cambiar intervalo de lectura (1000-60000 ms) |
| `nrg_set_channels` | `"value": n` | Cambiar cantidad de canales activos (1-4) |
| `nrg_set_alert` | `"value": watts` | Cambiar umbral de alerta (100-10000 W) |
| `nrg_reset_energy` | `"channel": n` | Resetear energia acumulada de un canal |

**Ejemplos:**
```json
{"cmd": "nrg_status"}
{"cmd": "nrg_read", "channel": 0}
{"cmd": "nrg_set_channels", "value": 2}
{"cmd": "nrg_set_alert", "value": 5000}
{"cmd": "nrg_reset_energy", "channel": 1}
```

### Telemetria

```json
{
  "channels": 2,
  "interval": 5000,
  "alert": 3000.0,
  "ch": [
    {
      "v": 220.3,          // Voltaje RMS (V)
      "i": 1.5,            // Corriente RMS (A)
      "w": 330.5,          // Potencia real (W)
      "va": 330.5,         // Potencia aparente (VA)
      "pf": 1.0,           // Factor de potencia (0.0-1.0)
      "kwh": 567.8         // Energia acumulada (Wh)
    },
    {
      "v": 220.1,
      "i": 0.8,
      "w": 176.1,
      "va": 176.1,
      "pf": 1.0,
      "kwh": 234.5
    }
  ]
}
```

### Media movil (smoothing)

Cada lectura se promedia con las ultimas N lecturas (ventana = 5 por defecto). Esto reduce ruido del ADC y proporciona lecturas mas estables.

### Configuracion

| Parametro | Rango | Default | NVS Key |
|-----------|-------|---------|---------|
| Canales activos | 1-4 | 1 | `nrg/ch_count` |
| Intervalo lectura | 1000-60000 ms | 5000 ms | `nrg/intv` |
| Umbral alerta | 100-10000 W | 3000 W | `nrg/alert_w` |
| Energia canal N | float (Wh) | 0.0 | `nrg/kwh_N` |

---

## 12. Modulo ACC — Control de Acceso

### Descripcion

Control de acceso basado en tarjetas NFC/RFID usando un lector MFRC522 via SPI. Gestiona una lista de hasta 20 UIDs autorizados con persistencia NVS. Activa un rele de cerradura con pulso temporizado al detectar un UID autorizado.

### Hardware

| Componente | Pin | Descripcion |
|------------|-----|------------|
| MFRC522 SCK | GPIO18 | SPI clock |
| MFRC522 MISO | GPIO19 | SPI data in |
| MFRC522 MOSI | GPIO23 | SPI data out |
| MFRC522 SS | GPIO5 | SPI chip select |
| MFRC522 RST | GPIO27 | Reset del lector |
| Rele cerradura | GPIO2 | Activacion de cerradura |

> **Conexion MFRC522:** Alimentar a 3.3V (NO 5V). Conectar IRQ al aire (no se usa).

### Comandos WebSocket

| Comando | Campos | Descripcion |
|---------|--------|------------|
| `acc_toggle` | — | Activar/desactivar rele de cerradura |
| `acc_status` | — | Solicitar estado completo |
| `acc_add_uid` | `"uid": "XX:XX:XX:XX"` | Agregar UID autorizado |
| `acc_remove_uid` | `"uid": "XX:XX:XX:XX"` | Eliminar UID autorizado |
| `acc_list_uids` | — | Listar todos los UIDs autorizados |
| `acc_clear_uids` | — | Eliminar todos los UIDs |
| `acc_set_pulse` | `"value": ms` | Cambiar duracion del pulso (500-15000 ms) |
| `acc_get_events` | — | Obtener log de eventos |

**Ejemplos:**
```json
{"cmd": "acc_add_uid", "uid": "AB:CD:EF:01"}
{"cmd": "acc_remove_uid", "uid": "AB:CD:EF:01"}
{"cmd": "acc_list_uids"}
{"cmd": "acc_set_pulse", "value": 5000}
```

### Formato de UID

Los UIDs se representan como cadenas hexadecimales separadas por dos puntos:
- **4 bytes:** `"AB:CD:EF:01"`
- **7 bytes:** `"AB:CD:EF:01:23:45:67"`

Soporta mayusculas y minusculas: `"ab:cd:ef:01"` es equivalente a `"AB:CD:EF:01"`.

### Telemetria

```json
{
  "locked": 1,                    // 1=cerrado, 0=abierto
  "uids": ["AB:CD:EF:01", "11:22:33:44"],  // UIDs autorizados
  "pulse_ms": 3000,               // Duracion del pulso (ms)
  "events": 3,                    // Cantidad de eventos en log
  "last_event": "Acceso: AB:CD:EF:01"  // Ultimo evento
}
```

### Flujo de operacion

1. El lector MFRC522 detecta una tarjeta NFC
2. Lee el UID de la tarjeta
3. Convierte los bytes a formato string (`"XX:XX:XX:XX"`)
4. Busca el UID en la lista de autorizados
5. **Si autorizado:** Activa el rele de cerradura por `pulse_ms` milisegundos, registra evento "Acceso concedido"
6. **Si no autorizado:** Registra evento "Acceso denegado"
7. Espera a que la tarjeta sea retirada antes de leer otra

### Configuracion

| Parametro | Rango | Default | NVS Key |
|-----------|-------|---------|---------|
| Duracion pulso | 500-15000 ms | 3000 ms | `acc/pulse_ms` |
| UIDs autorizados | max 20 | 0 | `acc/uid_cnt` + `acc/uid_N` |

---

## 13. Persistencia NVS

HomeNode usa la API de Preferences del ESP32 para almacenar configuracion en memoria flash no volatil. Los datos persisten tras reinicios y actualizaciones OTA.

### API NVS del core

| Funcion | Descripcion |
|---------|------------|
| `smrt_nvs_set_string(ns, key, value)` | Guardar string |
| `smrt_nvs_get_string(ns, key, buf, len)` | Leer string (retorna bool) |
| `smrt_nvs_set_int(ns, key, value)` | Guardar int32 |
| `smrt_nvs_get_int(ns, key, out, default)` | Leer int32 (retorna bool) |
| `smrt_nvs_set_bool(ns, key, value)` | Guardar bool |
| `smrt_nvs_get_bool(ns, key, out, default)` | Leer bool (retorna bool) |
| `smrt_nvs_remove(ns, key)` | Eliminar key |
| `smrt_nvs_clear(ns)` | Limpiar todo un namespace |

### Tabla completa de NVS

| Namespace | Key | Tipo | Descripcion |
|-----------|-----|------|------------|
| `smrt_cfg` | `wifi_ssid` | string | SSID de la red WiFi |
| `smrt_cfg` | `wifi_pass` | string | Password WiFi |
| `smrt_cfg` | `cfg_pin` | string | PIN de configuracion |
| `env` | `read_intv` | int | Intervalo de lectura ENV (ms) |
| `rly` | `count` | int | Cantidad de reles activos |
| `rly` | `states` | int | Bitmask de estados |
| `rly` | `pulse_ms` | int | Duracion de pulso (ms) |
| `rly` | `name_0`..`name_3` | string | Nombres de reles |
| `sec` | `armed` | bool | Estado armado |
| `sec` | `entry_dly` | int | Delay de entrada (ms) |
| `sec` | `exit_dly` | int | Delay de salida (ms) |
| `plg` | `state` | int | Estado del rele |
| `plg` | `intv` | int | Intervalo de lectura (ms) |
| `plg` | `overload` | float | Umbral sobrecarga (A) |
| `plg` | `kwh` | float | Energia acumulada (Wh) |
| `nrg` | `ch_count` | int | Canales activos |
| `nrg` | `intv` | int | Intervalo de lectura (ms) |
| `nrg` | `alert_w` | float | Umbral alerta (W) |
| `nrg` | `kwh_0`..`kwh_3` | float | Energia por canal (Wh) |
| `acc` | `uid_cnt` | int | Cantidad de UIDs |
| `acc` | `uid_0`..`uid_19` | string | UIDs autorizados |
| `acc` | `pulse_ms` | int | Duracion pulso cerradura (ms) |

---

## 14. Actualizacion OTA

### Via ArduinoOTA (PlatformIO)

```bash
# Requiere que el ESP32 este conectado a la misma red
pio run -e ota -t upload
```

Configuracion en `platformio.ini`:
- Host: `192.168.1.100`
- Puerto: `3232`
- mDNS hostname: `homenode`

### Via HTTP (navegador)

1. Acceder a `http://192.168.1.100/update`
2. Seleccionar el archivo `.bin` (ubicacion tipica: `.pio/build/nodemcu-32s/firmware.bin`)
3. Click en "Subir Firmware"
4. Esperar a que la barra de progreso llegue al 100%
5. El dispositivo se reinicia automaticamente

> **NOTA:** La actualizacion HTTP actualmente no requiere autenticacion. Ver la auditoria de seguridad para recomendaciones.

---

## 15. Desarrollo y Tests

### Estructura del proyecto

```
HomeNode/
├── include/
│   ├── core/                     # Headers de plataforma
│   │   ├── smrt_core.h           # Include maestro
│   │   ├── smrt_core_config.h    # Configuracion global
│   │   ├── smrt_core_module.h    # Interface de modulo
│   │   ├── smrt_core_webui.h     # HTML del dashboard
│   │   └── ...
│   └── modules/                  # Headers de modulos
│       ├── smrt_mod_env.h        # Prototipos ENV
│       ├── smrt_mod_env_config.h # Config ENV
│       └── ...                   # (rly, sec, plg, nrg, acc)
├── src/
│   ├── core/                     # Fuentes de plataforma
│   │   ├── smrt_core_http.cpp    # Servidor HTTP
│   │   ├── smrt_core_ws.cpp      # WebSocket
│   │   ├── smrt_core_wifi.cpp    # Gestion WiFi
│   │   ├── smrt_core_nvs.cpp     # Persistencia NVS
│   │   ├── smrt_core_ota.cpp     # OTA updates
│   │   ├── smrt_core_module.cpp  # Registro de modulos
│   │   └── ...
│   ├── modules/                  # Fuentes de modulos
│   │   ├── smrt_mod_env.cpp
│   │   └── ...                   # (rly, sec, plg, nrg, acc)
│   └── smrt_main.cpp             # Entry point (setup/loop)
├── test/
│   ├── test_mod_env/             # Tests unitarios por modulo
│   ├── test_mod_rly/
│   ├── test_mod_sec/
│   ├── test_mod_plg/
│   ├── test_mod_nrg/
│   ├── test_mod_acc/
│   ├── test_core_module/
│   └── test_core_types/
├── docs/                         # Documentacion
├── reports/                      # Reportes QA (en .gitignore)
├── platformio.ini                # Configuracion PlatformIO
├── smrt_qa.py                    # Script de QA automatizado
└── CLAUDE.md                     # Metodologia de trabajo
```

### Ejecutar tests unitarios

```bash
# Ejecutar todos los tests nativos (en PC, sin ESP32)
pio test -e native

# Ejecutar tests con salida verbose
pio test -e native -v
```

Los tests se ejecutan en un entorno `native` (PC) sin hardware ESP32. Las dependencias de hardware se aislan con `#ifdef UNIT_TEST`.

### Compilar para ESP32

```bash
# Compilar (sin flashear)
pio run -e nodemcu-32s

# Compilar y flashear via USB
pio run -e nodemcu-32s -t upload
```

### QA automatizado

```bash
# Ejecutar suite completa: tests + analisis estatico + metricas
python smrt_qa.py

# Solo tests
python smrt_qa.py --tests

# Solo metricas
python smrt_qa.py --metrics
```

Los reportes se generan en `reports/<timestamp>/`.

### Patron para crear un nuevo modulo

Cada modulo sigue un patron de 4 archivos:

1. **Config:** `include/modules/smrt_mod_<id>_config.h` — Pines, limites, NVS keys
2. **Header:** `include/modules/smrt_mod_<id>.h` — Descriptor extern + prototipos
3. **Source:** `src/modules/smrt_mod_<id>.cpp` — Implementacion (7 secciones)
4. **Tests:** `test/test_mod_<id>/test_smrt_mod_<id>.cpp` — Tests unitarios

La implementacion del `.cpp` tiene esta estructura:

```
1. Includes (con guard UNIT_TEST)
2. State (variables static)
3. Utility functions (siempre compiladas, testables)
4. Hardware functions (#ifndef UNIT_TEST)
5. WebSocket handler (#ifndef UNIT_TEST)
6. Telemetry function (#ifndef UNIT_TEST)
7. Module descriptor (const smrt_module_t)
```

---

## 16. Referencia Rapida de Comandos

### Comandos Core

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "status"}` | Solicitar telemetria completa |
| `{"cmd": "wifi", "pin": "...", "ssid": "...", "pass": "..."}` | Cambiar WiFi |

### Comandos ENV

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "env_read"}` | Lectura inmediata |
| `{"cmd": "env_set_interval", "value": ms}` | Cambiar intervalo |

### Comandos RLY

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "rly_toggle", "index": n}` | Toggle rele |
| `{"cmd": "rly_set", "index": n, "state": 0/1}` | Set rele |
| `{"cmd": "rly_set_all", "states": [...]}` | Set todos |
| `{"cmd": "rly_pulse", "index": n}` | Pulso |
| `{"cmd": "rly_set_pulse", "value": ms}` | Config pulso |
| `{"cmd": "rly_set_count", "value": n}` | Config cantidad |
| `{"cmd": "rly_set_name", "index": n, "name": "..."}` | Nombrar rele |
| `{"cmd": "rly_status"}` | Estado completo |

### Comandos SEC

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "sec_arm"}` | Armar |
| `{"cmd": "sec_disarm"}` | Desarmar |
| `{"cmd": "sec_status"}` | Estado |
| `{"cmd": "sec_set_entry_delay", "value": ms}` | Config delay entrada |
| `{"cmd": "sec_set_exit_delay", "value": ms}` | Config delay salida |
| `{"cmd": "sec_get_events"}` | Log eventos |
| `{"cmd": "sec_clear_events"}` | Limpiar log |

### Comandos PLG

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "plg_toggle"}` | Toggle enchufe |
| `{"cmd": "plg_set", "state": 0/1}` | Set estado |
| `{"cmd": "plg_status"}` | Estado |
| `{"cmd": "plg_set_interval", "value": ms}` | Config intervalo |
| `{"cmd": "plg_set_overload", "value": amps}` | Config sobrecarga |
| `{"cmd": "plg_reset_energy"}` | Reset energia |

### Comandos NRG

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "nrg_status"}` | Estado todos los canales |
| `{"cmd": "nrg_read", "channel": n}` | Leer canal |
| `{"cmd": "nrg_set_interval", "value": ms}` | Config intervalo |
| `{"cmd": "nrg_set_channels", "value": n}` | Config canales |
| `{"cmd": "nrg_set_alert", "value": watts}` | Config alerta |
| `{"cmd": "nrg_reset_energy", "channel": n}` | Reset energia canal |

### Comandos ACC

| Comando | Descripcion |
|---------|------------|
| `{"cmd": "acc_toggle"}` | Toggle cerradura |
| `{"cmd": "acc_status"}` | Estado |
| `{"cmd": "acc_add_uid", "uid": "XX:XX:XX:XX"}` | Agregar UID |
| `{"cmd": "acc_remove_uid", "uid": "XX:XX:XX:XX"}` | Eliminar UID |
| `{"cmd": "acc_list_uids"}` | Listar UIDs |
| `{"cmd": "acc_clear_uids"}` | Eliminar todos |
| `{"cmd": "acc_set_pulse", "value": ms}` | Config pulso |
| `{"cmd": "acc_get_events"}` | Log eventos |

---

> **HomeNode v0.4.0** — Plataforma IoT domestica modular
> Documentacion generada: 2026-03-14
