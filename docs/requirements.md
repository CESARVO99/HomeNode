# HomeNode — Documento de Requisitos de la Plataforma

> **Version:** 0.1.0 | **FASE 1** — Definicion de la Plataforma
> **Fecha:** 2026-03-13
> **Proyecto base de referencia:** Smart_Lock v1.3.0

---

## 1. Vision y Alcance

### 1.1 Descripcion del Proyecto

HomeNode es una **plataforma IoT domestica modular** basada en ESP32-WROOM.

Proporciona un firmware CORE compartido que gestiona WiFi, WebSocket,
OTA, almacenamiento persistente, serial, GPIO y utilidades de tipos.
Sobre este CORE se conectan MODULOS intercambiables que implementan
funcionalidades especificas para cada tipo de dispositivo.

### 1.2 Filosofia del Sistema

```
Hardware comun (PCB base ESP32)
    + Modulos funcionales intercambiables
    + Firmware modular con deteccion automatica
    = Ecosistema IoT domestico completo
```

### 1.3 Hardware Objetivo

| Elemento | Especificacion |
|----------|----------------|
| MCU principal | ESP32-WROOM-32 (NodeMCU-32S) |
| MCU futuro | ESP32-S3, ESP32-C3 |
| Placa base | PCB universal con conectores de expansion (FASE 8) |
| Modulos | Placas hija conectables via headers |

### 1.4 Stack de Desarrollo

| Componente | Tecnologia |
|------------|------------|
| Plataforma | ESP32 / Arduino framework |
| Build system | PlatformIO |
| Lenguaje | C/C++ (C++11) |
| IDE | Visual Studio Code + PlatformIO |
| Prefijo | smrt_ (todas las funciones, macros, archivos) |
| Tests | Unity framework (nativos en PC) |
| Analisis | cppcheck |
| QA | smrt_qa.py (automatizado) |

---

## 2. Requisitos CORE de la Plataforma

El CORE es el firmware base compartido por todos los dispositivos HomeNode.
Gestiona la infraestructura comun: conectividad, comunicacion, persistencia,
depuracion, control de pines y utilidades.

### 2.1 WiFi Management (SMRT_CORE_WIFI)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| WIFI-001 | Modo Station con configuracion de IP estatica | MUST |
| WIFI-002 | Persistencia de SSID/password via NVS | MUST |
| WIFI-003 | Actualizacion de credenciales WiFi via WebSocket | MUST |
| WIFI-004 | PIN de acceso configurable para cambios WiFi | MUST |
| WIFI-005 | Credenciales fallback cuando NVS esta vacio | MUST |
| WIFI-006 | Auto-reconexion ante perdida de conexion | SHOULD |
| WIFI-007 | Modo AP fallback para setup inicial (captive portal) | COULD |
| WIFI-008 | Resolucion mDNS (<hostname>.local) | SHOULD |
| WIFI-009 | Configuracion de gateway, subnet y DNS | MUST |
| WIFI-010 | Hostname configurable por dispositivo | MUST |

### 2.2 WebSocket Server (SMRT_CORE_WS)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| WS-001 | Servidor AsyncWebSocket en endpoint /ws | MUST |
| WS-002 | Protocolo de comandos/respuestas en JSON | MUST |
| WS-003 | Broadcast de telemetria a intervalo configurable | MUST |
| WS-004 | Logging de eventos connect/disconnect de clientes | MUST |
| WS-005 | Sistema de dispatch extensible con registro de handlers | MUST |
| WS-006 | Endpoint de status (RSSI, uptime, IP, clients, SSID, modulo) | MUST |
| WS-007 | Routing de comandos por modulo (prefijo cmd por modulo) | MUST |
| WS-008 | Limpieza automatica de clientes desconectados | MUST |
| WS-009 | Soporte para multiples clientes simultaneos | MUST |

### 2.3 HTTP Server (SMRT_CORE_HTTP)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| HTTP-001 | Servidor HTTP asincrono en puerto 80 | MUST |
| HTTP-002 | Servir Web UI desde PROGMEM (GET /) | MUST |
| HTTP-003 | Endpoint de upload OTA (GET/POST /update) | MUST |
| HTTP-004 | API REST basica para integraciones | SHOULD |
| HTTP-005 | Endpoints: /api/status, /api/sensors, /api/config | SHOULD |

### 2.4 OTA Updates (SMRT_CORE_OTA)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| OTA-001 | Servicio ArduinoOTA (upload via CLI/IDE) | MUST |
| OTA-002 | Upload HTTP via web (/update) | MUST |
| OTA-003 | Reporte de progreso de upload | MUST |
| OTA-004 | Hostname OTA configurable | MUST |
| OTA-005 | Password/auth OTA configurable | SHOULD |
| OTA-006 | Reporte de version de firmware | MUST |
| OTA-007 | Auto-restart tras flash exitoso | MUST |

### 2.5 NVS Persistence (SMRT_CORE_NVS)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| NVS-001 | Wrapper sobre Preferences API con namespace "smrt_cfg" | MUST |
| NVS-002 | Almacenamiento de credenciales WiFi (SSID, password) | MUST |
| NVS-003 | Almacenamiento de PIN de configuracion | MUST |
| NVS-004 | Almacenamiento de configuracion por modulo (namespace propio) | MUST |
| NVS-005 | Factory reset (borrar todo NVS) via WebSocket | SHOULD |
| NVS-006 | Interfaz generica get/set key-value (string, int, bool) | MUST |

### 2.6 Serial Communication (SMRT_CORE_SERIAL)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| SER-001 | Inicializacion UART (115200/8N1) | MUST |
| SER-002 | Lectura serial no-bloqueante | MUST |
| SER-003 | Sistema de niveles de log (ERROR, WARN, INFO, DEBUG) | SHOULD |
| SER-004 | Definiciones de constantes ASCII | MUST |
| SER-005 | Timeout y buffer configurable | MUST |

### 2.7 GPIO Abstraction (SMRT_CORE_GPIO)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| GPIO-001 | Inicializacion de pines (modo input/output) | MUST |
| GPIO-002 | Set/clear/toggle de salida digital | MUST |
| GPIO-003 | Lectura de entrada digital | MUST |
| GPIO-004 | Lectura de entrada analogica (ADC) | SHOULD |
| GPIO-005 | Salida PWM | COULD |
| GPIO-006 | Definicion de pin map universal para placa base | MUST |
| GPIO-007 | Header de asignacion de pines por modulo | MUST |
| GPIO-008 | Constantes para pines reservados del sistema | MUST |

### 2.8 Data Type Utilities (SMRT_CORE_FORMAT)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FMT-001 | Tipos numericos custom (uint8, uint16, uint32, int8, int16, int32, bit) | MUST |
| FMT-002 | Macros de byte swap (16-bit, 32-bit) | MUST |
| FMT-003 | Conversion de caracteres (upper/lower case) | MUST |
| FMT-004 | Manipulacion de strings (copy, compare) | MUST |
| FMT-005 | Conversion numerica (string-to-number, number-to-string) | MUST |
| FMT-006 | Conversion booleana (ON/OFF, TRUE/FALSE) | MUST |
| FMT-007 | Parsing y formato de IP address | MUST |
| FMT-008 | Operaciones de memoria (fill16, fill32) | MUST |
| FMT-009 | Conversion hexadecimal | MUST |
| FMT-010 | Conversion de punto fijo (fixed-point) | MUST |

### 2.9 Web UI Framework (SMRT_CORE_WEBUI)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| UI-001 | Framework HTML/CSS/JS base en PROGMEM | MUST |
| UI-002 | Sistema de tarjetas (cards) por modulo en dashboard | MUST |
| UI-003 | Panel de status/telemetria del sistema | MUST |
| UI-004 | Panel de configuracion WiFi (protegido por PIN) | MUST |
| UI-005 | Pagina de update OTA | MUST |
| UI-006 | Diseno responsive (mobile/tablet/desktop) | MUST |
| UI-007 | Gestion de conexion WebSocket (auto-reconnect) | MUST |
| UI-008 | Punto de inyeccion de UI por modulo | SHOULD |
| UI-009 | Indicador visual de estado de conexion (verde/naranja/rojo) | MUST |
| UI-010 | Tema visual consistente y estetica domestica | SHOULD |

---

## 3. Sistema de Modulos

### 3.1 Arquitectura de Modulos (SMRT_MOD_ARCH)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| MOD-001 | Cada modulo = minimo 1 header (.h) + 1 source (.cpp) | MUST |
| MOD-002 | Interfaz de registro de modulo (init, loop, ws_handler) | MUST |
| MOD-003 | Identificacion de modulo (nombre, version, type ID) | MUST |
| MOD-004 | Compilacion condicional por modulo (#ifdef SMRT_MOD_xxx) | MUST |
| MOD-005 | Namespace de comandos WebSocket por modulo | MUST |
| MOD-006 | Namespace NVS propio por modulo | MUST |
| MOD-007 | Configuracion de pines especifica por modulo | MUST |
| MOD-008 | Tarjeta Web UI propia por modulo | SHOULD |
| MOD-009 | Deteccion automatica de modulo conectado | COULD |
| MOD-010 | Hot-swap de modulo sin recompilacion (futuro) | COULD |

### 3.2 Modulo: Control Ambiental (SMRT_MOD_ENV)

**Proposito:** Monitorizacion de condiciones ambientales interiores.

**Sensores objetivo:**
- Temperatura + humedad: DHT22 o BME280
- Presion atmosferica: BME280
- Luz ambiental: LDR analogico o BH1750 (I2C)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| ENV-001 | Lectura de sensor de temperatura (DHT22 / BME280) | MUST |
| ENV-002 | Lectura de sensor de humedad | MUST |
| ENV-003 | Lectura de sensor de presion atmosferica (BME280) | COULD |
| ENV-004 | Lectura de sensor de luz ambiental (LDR / BH1750) | COULD |
| ENV-005 | Broadcast de telemetria periodico (intervalo configurable) | MUST |
| ENV-006 | Alertas por umbrales (temp alta, humedad baja, etc.) | SHOULD |
| ENV-007 | Calibracion de sensores via WebSocket | COULD |

**Interfaces:**
- I2C (BME280, BH1750)
- GPIO digital (DHT22)
- ADC analogico (LDR)

### 3.3 Modulo: Seguridad (SMRT_MOD_SEC)

**Proposito:** Deteccion de intrusiones y alertas de seguridad.

**Sensores objetivo:**
- PIR (movimiento)
- Reed switch (puerta/ventana abierta/cerrada)
- Sensor de vibracion

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| SEC-001 | Deteccion de movimiento via PIR | MUST |
| SEC-002 | Deteccion de apertura/cierre via reed switch | MUST |
| SEC-003 | Deteccion de vibracion | COULD |
| SEC-004 | Gestion de estado de alarma (armed/disarmed/triggered) | MUST |
| SEC-005 | Notificacion de alerta via WebSocket | MUST |
| SEC-006 | Logging de eventos con timestamps | SHOULD |
| SEC-007 | Sirena/buzzer de alerta | COULD |
| SEC-008 | Armado/desarmado via WebSocket y Web UI | MUST |
| SEC-009 | Delay de entrada/salida configurable | SHOULD |

**Interfaces:**
- GPIO digital input (PIR, reed switch, vibracion)
- GPIO digital output (sirena/buzzer)

### 3.4 Modulo: Control de Reles (SMRT_MOD_RELAY)

**Proposito:** Control de dispositivos electricos via reles.

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| RLY-001 | Toggle de rele individual/multiple via WebSocket | MUST |
| RLY-002 | Soporte configurable de 1 a 4 reles | MUST |
| RLY-003 | Modo pulso temporizado (duracion configurable) | MUST |
| RLY-004 | Persistencia de estado de reles via NVS | SHOULD |
| RLY-005 | Telemetria de estado de reles | MUST |
| RLY-006 | Nombres configurables por rele (ej: "Luz salon") | SHOULD |
| RLY-007 | Control independiente por rele | MUST |

**Interfaces:**
- GPIO digital output (1-4 pines de rele)

### 3.5 Modulo: Enchufe Inteligente (SMRT_MOD_PLUG)

**Proposito:** Control remoto de enchufe con medicion de consumo.

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| PLG-001 | Control on/off de rele via WebSocket | MUST |
| PLG-002 | Medicion de corriente (ACS712 / SCT-013) | MUST |
| PLG-003 | Medicion de voltaje (ZMPT101B / divisor resistivo) | MUST |
| PLG-004 | Calculo de potencia (W) | MUST |
| PLG-005 | Acumulacion de consumo energetico (kWh) | SHOULD |
| PLG-006 | Proteccion contra sobrecarga (limite corriente config.) | SHOULD |
| PLG-007 | Soporte de temporizador/programacion | COULD |
| PLG-008 | Telemetria de consumo a intervalo configurable | MUST |

**Interfaces:**
- GPIO digital output (rele)
- ADC analogico (sensor corriente, sensor voltaje)

### 3.6 Modulo: Control de Acceso (SMRT_MOD_ACCESS)

**Proposito:** Cerradura inteligente con autenticacion NFC/RFID.

> **Nota:** Este modulo es la evolucion directa de Smart_Lock v1.3.0.

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| ACC-001 | Lectura de tarjetas NFC/RFID (MFRC522 via SPI) | MUST |
| ACC-002 | Gestion de UIDs autorizados (add/remove via WebSocket) | MUST |
| ACC-003 | Control de rele de cerradura (modo pulso) | MUST |
| ACC-004 | Logging de eventos de acceso | SHOULD |
| ACC-005 | Persistencia de UIDs autorizados via NVS | MUST |
| ACC-006 | Estado de puerta (locked/unlocked) via WebSocket | MUST |
| ACC-007 | Duracion de pulso de apertura configurable | MUST |
| ACC-008 | Indicacion visual/sonora de acceso concedido/denegado | COULD |

**Interfaces:**
- SPI (MFRC522: SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23, SS=GPIO5, RST=GPIO27)
- GPIO digital output (rele cerradura)

### 3.7 Modulo: Monitorizacion Energetica (SMRT_MOD_ENERGY)

**Proposito:** Monitorizacion del consumo electrico del hogar.

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| NRG-001 | Medicion de voltaje AC (ZMPT101B) | MUST |
| NRG-002 | Medicion de corriente (pinza CT / ACS712) | MUST |
| NRG-003 | Calculo de potencia (W, VA, factor de potencia) | MUST |
| NRG-004 | Acumulacion de energia (kWh) | SHOULD |
| NRG-005 | Soporte multi-canal (hasta 4 canales) | COULD |
| NRG-006 | Broadcast de telemetria a intervalo configurable | MUST |
| NRG-007 | Alertas de consumo excesivo | SHOULD |
| NRG-008 | Historial de consumo (ultimas 24h minimo) | COULD |

**Interfaces:**
- ADC analogico (sensores de corriente y voltaje)

---

## 4. Pin Map Universal — Placa Base ESP32-WROOM

### 4.1 Pines Reservados (CORE — No disponibles para modulos)

| GPIO | Funcion | Asignacion |
|------|---------|------------|
| 0 | Boot mode | Strapping pin (no usar en produccion) |
| 1 | TXD0 | UART TX (serial debug) |
| 3 | RXD0 | UART RX (serial debug) |
| 6-11 | SPI Flash | Flash interna (NO USAR NUNCA) |
| 21 | SDA | Bus I2C data (compartido por sensores) |
| 22 | SCL | Bus I2C clock (compartido por sensores) |

### 4.2 Pines de Expansion para Modulos

| GPIO | Capacidad | Uso sugerido |
|------|-----------|--------------|
| 2 | Digital I/O, ADC | Rele 1 / LED de estado |
| 4 | Digital I/O, ADC, Touch | Entrada modulo 1 |
| 5 | Digital I/O, SPI SS | SPI chip select 0 (NFC/otro) |
| 12 | Digital I/O, ADC, Touch | Entrada modulo 2 |
| 13 | Digital I/O, ADC, Touch | Entrada modulo 3 |
| 14 | Digital I/O, ADC, Touch | Entrada modulo 4 |
| 15 | Digital I/O, ADC, Touch | Entrada modulo 5 |
| 16 | Digital I/O | Salida modulo 1 |
| 17 | Digital I/O | Salida modulo 2 |
| 18 | Digital I/O, SPI SCK | SPI clock (bus compartido) |
| 19 | Digital I/O, SPI MISO | SPI data in (bus compartido) |
| 23 | Digital I/O, SPI MOSI | SPI data out (bus compartido) |
| 25 | Digital I/O, DAC | Salida modulo 3 / DAC |
| 26 | Digital I/O, DAC | Salida modulo 4 / DAC |
| 27 | Digital I/O, ADC, Touch | Entrada modulo 6 / SPI RST |
| 32 | Digital I/O, ADC, Touch | Entrada modulo 7 |
| 33 | Digital I/O, ADC, Touch | Entrada modulo 8 |
| 34 | Input only, ADC | Sensor analogico 1 |
| 35 | Input only, ADC | Sensor analogico 2 |
| 36 (VP) | Input only, ADC | Sensor analogico 3 |
| 39 (VN) | Input only, ADC | Sensor analogico 4 |

### 4.3 Buses de Comunicacion

| Bus | Pines | Max dispositivos |
|-----|-------|------------------|
| I2C | GPIO21 (SDA), GPIO22 (SCL) | Hasta 127 dispositivos |
| SPI | GPIO18 (SCK), GPIO19 (MISO), GPIO23 (MOSI) | 2 dispositivos (SS0=GPIO5, SS1=GPIO27) |
| UART0 | GPIO1 (TX), GPIO3 (RX) | Serial debug |

### 4.4 Asignacion de Pines por Modulo

| Modulo | Pines principales | Bus |
|--------|-------------------|-----|
| ENV (ambiental) | GPIO4 (DHT22), I2C (BME280, BH1750), GPIO34 (LDR) | I2C, ADC |
| SEC (seguridad) | GPIO12 (PIR), GPIO13 (reed), GPIO14 (vibracion), GPIO25 (buzzer) | GPIO |
| RELAY | GPIO2, GPIO16, GPIO17, GPIO25 (reles 1-4) | GPIO |
| PLUG | GPIO2 (rele), GPIO34 (corriente), GPIO35 (voltaje) | GPIO, ADC |
| ACCESS | SPI (MFRC522), GPIO2 (rele cerradura) | SPI, GPIO |
| ENERGY | GPIO34-39 (sensores analogicos, hasta 4 canales) | ADC |

---

## 5. Protocolo de Comunicacion

### 5.1 Protocolo WebSocket JSON

Todos los mensajes WebSocket usan formato JSON:

**Comando (cliente -> servidor):**
```json
{
    "cmd": "<nombre_comando>",
    "module": "<module_id>"
}
```

**Respuesta/telemetria (servidor -> cliente):**
```json
{
    "type": "status",
    "module": "<module_id>"
}
```

### 5.2 Comandos CORE

| Comando | Descripcion | Campos |
|---------|-------------|--------|
| status | Solicitar estado completo del sistema | cmd |
| wifi | Actualizar credenciales WiFi | cmd, pin, ssid, pass, new_pin |
| reboot | Reiniciar ESP32 | cmd |
| factory_reset | Borrar NVS y reiniciar | cmd, pin |
| get_info | Info plataforma (version, modulos, uptime) | cmd |

### 5.3 Namespacing de Comandos por Modulo

Los comandos de modulo se prefijan con el ID del modulo:

| Modulo | Prefijo | Ejemplo de comando |
|--------|---------|-------------------|
| ENV | env_ | env_read, env_set_interval |
| SEC | sec_ | sec_arm, sec_disarm, sec_status |
| RELAY | rly_ | rly_toggle, rly_set, rly_status |
| PLUG | plg_ | plg_toggle, plg_read_power |
| ACCESS | acc_ | acc_add_uid, acc_remove_uid, acc_toggle |
| ENERGY | nrg_ | nrg_read, nrg_set_interval |

### 5.4 Telemetria de Status (broadcast periodico)

```json
{
    "type": "status",
    "rssi": -45,
    "uptime": 123456,
    "ip": "192.168.1.100",
    "clients": 2,
    "ssid": "MiRed",
    "version": "0.1.0",
    "module": "env",
    "module_data": {}
}
```

### 5.5 MQTT (Opcional — Futuro)

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| MQTT-001 | Cliente MQTT para publicacion de telemetria | COULD |
| MQTT-002 | Suscripcion a comandos via MQTT | COULD |
| MQTT-003 | Integracion con Home Assistant via MQTT discovery | COULD |
| MQTT-004 | Integracion con Node-RED | COULD |

---

## 6. Requisitos No Funcionales

| ID | Requisito | Objetivo |
|----|-----------|----------|
| NFR-001 | Complejidad ciclomatica por funcion | <= 10 |
| NFR-002 | Lineas por funcion | <= 50 |
| NFR-003 | Densidad de comentarios | >= 15% |
| NFR-004 | Cobertura de tests (funciones) | >= 100% (1 test/funcion) |
| NFR-005 | Errores de analisis estatico (cppcheck) | 0 |
| NFR-006 | Tasa de paso de tests unitarios | 100% |
| NFR-007 | Tamano de binario firmware | < 1.5 MB |
| NFR-008 | Heap libre en runtime | > 50 KB |
| NFR-009 | Latencia de respuesta WebSocket | < 100 ms |
| NFR-010 | Tiempo de reconexion WiFi | < 10 s |
| NFR-011 | Documentacion Doxygen en todas las funciones publicas | MUST |
| NFR-012 | Compatibilidad con guardas UNIT_TEST | MUST |

---

## 7. Roadmap de Fases

| Fase | Nombre | Estado | Descripcion |
|------|--------|--------|-------------|
| FASE 1 | Definicion de plataforma | **ACTUAL** | Este documento |
| FASE 2 | Arquitectura firmware | PLANIFICADO | Diseno modular del firmware |
| FASE 3 | Sistema de modulos | PLANIFICADO | Interfaz de modulos, registro, deteccion |
| FASE 4 | Interfaces del sistema | PLANIFICADO | Web UI, API REST, WebSocket, MQTT |
| FASE 5 | Automatizacion | PLANIFICADO | Motor de reglas (si X entonces Y) |
| FASE 6 | Firmware base | PLANIFICADO | Implementacion del CORE |
| FASE 7 | QA y estabilidad | PLANIFICADO | Tests, analisis, benchmarks |
| FASE 8 | PCB base | PLANIFICADO | Diseno de placa controlador central |
| FASE 9 | Modulos hardware | PLANIFICADO | Diseno de placas modulares |
| FASE 10 | Carcasas 3D | PLANIFICADO | Diseno mecanico e impresion 3D |

---

## 8. Glosario

| Termino | Definicion |
|---------|-----------|
| CORE | Firmware base de la plataforma (WiFi, WS, OTA, NVS, GPIO, serial, format, WebUI) |
| MODULE | Firmware plugin para funcionalidad especifica de dispositivo |
| NVS | Non-Volatile Storage (almacenamiento flash key-value del ESP32) |
| OTA | Over-The-Air firmware update (actualizacion inalambrica) |
| WS | WebSocket |
| CC | Cyclomatic Complexity (complejidad ciclomatica) |
| PROGMEM | Flash memory del ESP32 (para almacenar HTML/CSS/JS) |
| SPI | Serial Peripheral Interface (bus de comunicacion) |
| I2C | Inter-Integrated Circuit (bus de comunicacion) |
| ADC | Analog to Digital Converter |
| DAC | Digital to Analog Converter |
| PCB | Printed Circuit Board (placa de circuito impreso) |
| MCU | Microcontroller Unit |

---

## Historial del Documento

| Version | Fecha | Autor | Cambios |
|---------|-------|-------|---------|
| 0.1.0 | 2026-03-13 | CESARVO99 + Claude | Requisitos iniciales — FASE 1 |
