# HomeNode v0.4.0 — Auditoria de Seguridad y Funcionalidad

> **Revision estatica de codigo | Analisis de superficie de ataque**
> Fecha: 2026-03-14 | Version auditada: 0.4.0
> Auditor: Claude (asistido por IA)

---

## Tabla de Contenidos

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Metodologia](#2-metodologia)
3. [Vulnerabilidades de Seguridad](#3-vulnerabilidades-de-seguridad)
4. [Gaps de Funcionalidad por Modulo](#4-gaps-de-funcionalidad-por-modulo)
5. [Gaps de Funcionalidad del Core](#5-gaps-de-funcionalidad-del-core)
6. [Recomendaciones Priorizadas](#6-recomendaciones-priorizadas)
7. [Matriz de Riesgo](#7-matriz-de-riesgo)
8. [Conclusiones](#8-conclusiones)

---

## 1. Resumen Ejecutivo

### Estado general

| Metrica | Valor |
|---------|-------|
| Modulos implementados | 6/6 |
| Tests unitarios | 331/331 PASS (100%) |
| Build ESP32 | OK (RAM 15.1%, Flash 70.9%) |
| Analisis estatico | 0 errores, 0 warnings, 2 style |
| LOC totales | 10,933 |
| Funciones | 194 |

### Hallazgos

| Severidad | Cantidad | Descripcion |
|-----------|----------|------------|
| **CRITICA** | 4 | Pueden comprometer totalmente el dispositivo |
| **ALTA** | 4 | Facilitan ataques significativos |
| **MEDIA** | 4 | Debilidades explotables en escenarios especificos |
| **BAJA/INFO** | 2 | Riesgo minimo, buenas practicas |
| **Gaps funcionales** | 23 | Funcionalidades faltantes o incompletas |

### Veredicto

La plataforma tiene una arquitectura modular solida con buena cobertura de tests. Sin embargo, **carece de mecanismos de seguridad basicos** para un dispositivo IoT conectado a red. Las 4 vulnerabilidades criticas permiten a cualquier dispositivo en la misma red local tomar control completo del dispositivo, incluyendo flashear firmware malicioso.

**Riesgo actual: ALTO** — El dispositivo NO debe exponerse a redes no confiables en su estado actual.

---

## 2. Metodologia

### Alcance

- Revision manual del codigo fuente completo (10,933 LOC)
- Analisis de todos los puntos de entrada de red (HTTP, WebSocket, OTA)
- Evaluacion de almacenamiento de credenciales (NVS)
- Revision de la logica de cada modulo
- Verificacion de gaps funcionales vs requisitos implicitos de un sistema IoT domestico

### Archivos analizados

| Categoria | Archivos | LOC aprox. |
|-----------|----------|-----------|
| Core | 7 archivos (ws, http, wifi, nvs, ota, module, gpio) | ~600 |
| Modules | 18 archivos (6 x config + header + source) | ~3,400 |
| Tests | 8 archivos de tests | ~2,100 |
| Config | platformio.ini, smrt_core_config.h | ~230 |
| WebUI | smrt_core_webui.h (HTML/CSS/JS) | ~300 |

### Criterios de evaluacion

- **CVSS v3.1** para puntuacion de vulnerabilidades
- Contexto: dispositivo IoT en red local domestica (no expuesto a internet directamente)
- Amenaza principal: atacante con acceso a la red local (WiFi comprometido, dispositivo malicioso en LAN)

---

## 3. Vulnerabilidades de Seguridad

### CRITICAS (CVSS >= 9.0)

---

#### V-01: Contrasena OTA vacia

| Campo | Valor |
|-------|-------|
| **Severidad** | CRITICA |
| **CVSS** | 9.8 |
| **Archivo** | `include/core/smrt_core_config.h:54` |
| **Codigo** | `#define SMRT_OTA_PASSWORD ""` |

**Descripcion:** La contrasena de ArduinoOTA esta definida como cadena vacia. Cualquier dispositivo en la red local puede flashear firmware arbitrario al ESP32 usando el protocolo OTA sin autenticacion.

**Impacto:** Control total del dispositivo. Un atacante puede subir firmware malicioso que robe credenciales WiFi, actue como pivot para atacar otros dispositivos en la red o cause dano fisico (activacion de reles).

**Archivo afectado:** `src/core/smrt_core_ota.cpp:151-153`
```c
if (strlen(SMRT_OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(SMRT_OTA_PASSWORD);
}
```

**Remediacion:**
- Establecer una contrasena OTA fuerte (minimo 12 caracteres)
- Hacerla configurable via NVS (no hardcoded)
- Considerar deshabilitar OTA en produccion

---

#### V-02: HTTP OTA sin autenticacion

| Campo | Valor |
|-------|-------|
| **Severidad** | CRITICA |
| **CVSS** | 9.8 |
| **Archivo** | `src/core/smrt_core_ota.cpp:192-231` |

**Descripcion:** El endpoint `POST /update` acepta uploads de firmware sin ningun tipo de autenticacion. Cualquier cliente HTTP puede subir un archivo `.bin` y reflashear el dispositivo.

**Impacto:** Identico a V-01 — control total del dispositivo.

**Remediacion:**
- Agregar autenticacion basica HTTP al endpoint `/update`
- Requerir PIN o token antes de aceptar firmware
- Validar el firmware antes de aplicarlo (checksum, firma)

---

#### V-03: WebSocket sin autenticacion

| Campo | Valor |
|-------|-------|
| **Severidad** | CRITICA |
| **CVSS** | 9.0 |
| **Archivo** | `src/core/smrt_core_ws.cpp:181-199` |

**Descripcion:** El servidor WebSocket acepta conexiones de cualquier cliente sin autenticacion. Cualquier dispositivo en la red puede conectarse a `ws://IP/ws` y enviar comandos que controlan todos los modulos.

**Impacto:**
- Activar/desactivar reles (riesgo fisico)
- Desarmar sistema de seguridad
- Abrir cerraduras NFC
- Cambiar configuracion WiFi (si conoce el PIN)
- Modificar UIDs autorizados de acceso

**Remediacion:**
- Implementar autenticacion WS (token en primer mensaje, o header de autenticacion)
- Separar comandos de lectura (sin auth) de escritura (con auth)
- Limitar acciones criticas a sesiones autenticadas

---

#### V-04: Sin verificacion de firma de firmware

| Campo | Valor |
|-------|-------|
| **Severidad** | CRITICA |
| **CVSS** | 9.0 |
| **Archivos** | `src/core/smrt_core_ota.cpp` |

**Descripcion:** Ni ArduinoOTA ni el HTTP upload verifican la autenticidad o integridad del firmware. No hay firma digital, checksum ni validacion del binario antes de flashearlo.

**Impacto:** Un atacante que logre subir firmware (via V-01 o V-02) no enfrenta ninguna barrera adicional. Firmware malicioso se ejecuta sin cuestionamiento.

**Remediacion:**
- Implementar Secure Boot (ESP32 soporta firmado de firmware)
- Como minimo, validar un hash SHA256 del firmware antes de aplicar
- ESP-IDF ofrece `esp_secure_boot_verify_signature()` para esto

---

### ALTAS (CVSS 7.0-8.9)

---

#### V-05: PIN WiFi sin rate limiting

| Campo | Valor |
|-------|-------|
| **Severidad** | ALTA |
| **CVSS** | 8.8 |
| **Archivo** | `src/core/smrt_core_ws.cpp:48-58` |

**Descripcion:** El comando `wifi` verifica el PIN pero no implementa rate limiting. Un atacante puede enviar miles de intentos de PIN por segundo via WebSocket para fuerza bruta.

**Impacto:** Con PIN default "1234" (solo 10,000 combinaciones), un ataque de fuerza bruta tarda segundos. Una vez obtenido el PIN, el atacante puede cambiar las credenciales WiFi y dejar el dispositivo inaccesible.

**Remediacion:**
- Implementar rate limiting: max 3 intentos por minuto
- Bloqueo temporal tras N intentos fallidos (lockout 5 minutos)
- Incrementar complejidad del PIN (alfanumerico, 8+ chars)

---

#### V-06: Credenciales WiFi en texto plano en NVS

| Campo | Valor |
|-------|-------|
| **Severidad** | ALTA |
| **CVSS** | 8.2 |
| **Archivo** | `src/core/smrt_core_wifi.cpp:87-90` |

**Descripcion:** Las credenciales WiFi (SSID y password) se almacenan en NVS sin cifrado. Si un atacante obtiene acceso fisico al ESP32, puede extraer las credenciales de la flash.

**Impacto:** Compromiso de la red WiFi domestica. Acceso a todos los dispositivos conectados.

**Remediacion:**
- Habilitar NVS Encryption (ESP32 lo soporta via ESP-IDF)
- Usar `nvs_flash_init_partition()` con partition encriptada
- Proteger fisicamente el dispositivo

---

#### V-07: Sin HTTPS/TLS

| Campo | Valor |
|-------|-------|
| **Severidad** | ALTA |
| **CVSS** | 7.5 |
| **Archivos** | `src/core/smrt_core_http.cpp`, `src/core/smrt_core_ws.cpp` |

**Descripcion:** Todo el trafico HTTP y WebSocket viaja en texto plano. Esto incluye el PIN de configuracion, credenciales WiFi, comandos de control y telemetria.

**Impacto:** Un atacante en la misma red puede interceptar (sniff) todo el trafico, incluyendo el PIN, credenciales WiFi y comandos de control.

**Remediacion:**
- Implementar HTTPS con certificado auto-firmado
- WS sobre WSS (WebSocket Secure)
- ESP32 soporta TLS via mbedTLS (incluido en ESP-IDF)
- Considerar trade-off de rendimiento (TLS consume ~40KB RAM extra)

---

#### V-08: PIN default debil

| Campo | Valor |
|-------|-------|
| **Severidad** | ALTA |
| **CVSS** | 7.0 |
| **Archivo** | `include/core/smrt_core_config.h:42` |
| **Codigo** | `#define SMRT_WIFI_PIN_DEFAULT "1234"` |

**Descripcion:** El PIN de configuracion por defecto es "1234", un valor extremadamente predecible. Solo permite 4 digitos numericos (max 10,000 combinaciones).

**Impacto:** Combinado con V-05 (sin rate limiting), el PIN puede ser adivinado trivialmente.

**Remediacion:**
- Generar un PIN unico aleatorio en el primer arranque (almacenar en NVS)
- Mostrar el PIN inicial solo por Serial (requiere acceso fisico)
- Permitir PIN alfanumerico de 8+ caracteres

---

### MEDIAS (CVSS 4.0-6.9)

---

#### V-09: Sin timeout de sesion WebSocket

| Campo | Valor |
|-------|-------|
| **Severidad** | MEDIA |
| **CVSS** | 5.3 |
| **Archivo** | `src/core/smrt_core_ws.cpp` |

**Descripcion:** Las conexiones WebSocket persisten indefinidamente. No hay mecanismo de timeout por inactividad ni expiracion de sesion.

**Impacto:** Conexiones zombie que consumen recursos. En escenarios de ataque, un atacante mantiene conexion permanente sin renovar credenciales.

**Remediacion:**
- Implementar timeout de inactividad (ej. 5 minutos sin mensajes)
- Enviar pings periodicos y desconectar si no hay pong

---

#### V-10: Sin limite de clientes WebSocket concurrentes

| Campo | Valor |
|-------|-------|
| **Severidad** | MEDIA |
| **CVSS** | 5.3 |
| **Archivo** | `src/core/smrt_core_ws.cpp` |

**Descripcion:** No hay limite maximo de clientes WebSocket simultaneos. El ESP32 tiene RAM limitada (~520KB) y cada conexion WS consume memoria.

**Impacto:** Un atacante puede abrir multiples conexiones hasta agotar la memoria RAM, causando un crash (DoS — Denial of Service).

**Remediacion:**
- Limitar conexiones WS a un maximo (ej. 4-8 clientes)
- Rechazar nuevas conexiones cuando se alcanza el limite
- AsyncWebSocket soporta `cleanupClients()` pero no limita conexiones nuevas

---

#### V-11: Sin validacion de origen WebSocket

| Campo | Valor |
|-------|-------|
| **Severidad** | MEDIA |
| **CVSS** | 4.3 |
| **Archivo** | `src/core/smrt_core_ws.cpp:181-199` |

**Descripcion:** No se verifica el header `Origin` en las conexiones WebSocket. Esto permite ataques CSRF (Cross-Site Request Forgery) donde una pagina web maliciosa abierta en el navegador del usuario envia comandos al ESP32.

**Impacto:** Si el usuario visita una pagina web maliciosa, esta podria enviar comandos al ESP32 (toggle reles, desarmar alarma, etc.) a traves del navegador del usuario.

**Remediacion:**
- Verificar el header Origin en `WS_EVT_CONNECT`
- Solo aceptar conexiones de origenes confiables (IP del ESP32)

---

#### V-12: Hostname mDNS fijo

| Campo | Valor |
|-------|-------|
| **Severidad** | MEDIA |
| **CVSS** | 4.0 |
| **Archivo** | `include/core/smrt_core_config.h:52` |
| **Codigo** | `#define SMRT_OTA_HOSTNAME "homenode"` |

**Descripcion:** El hostname mDNS es siempre "homenode", facilitando el descubrimiento del dispositivo en la red.

**Impacto:** Trivial para localizar el dispositivo. `ping homenode.local` o escaneo mDNS lo encuentra inmediatamente.

**Remediacion:**
- Hacer el hostname configurable via NVS
- Incluir un sufijo aleatorio (ej. "homenode-a3f2")

---

### BAJAS / INFORMATIVAS

---

#### V-13: Serial debug activo en produccion

| Campo | Valor |
|-------|-------|
| **Severidad** | BAJA |
| **CVSS** | 2.0 |
| **Archivos** | Multiples `Serial.println()` en todos los archivos core y modulos |

**Descripcion:** Los mensajes de debug por Serial estan siempre activos, incluyendo credenciales WiFi cargadas y PIN de configuracion.

**Impacto:** Si alguien tiene acceso fisico al puerto USB, puede leer informacion sensible via Serial monitor.

**Remediacion:**
- Agregar un flag de compilacion `-D SMRT_DEBUG` para habilitar/deshabilitar logs
- Nunca imprimir credenciales completas por Serial

---

#### V-14: Version expuesta en telemetria

| Campo | Valor |
|-------|-------|
| **Severidad** | INFO |
| **CVSS** | 1.0 |
| **Archivo** | `include/core/smrt_core_webui.h:190` |

**Descripcion:** La version del firmware se muestra en el footer del WebUI y en los descriptores de modulo. Facilita fingerprinting.

**Impacto:** Minimo. Permite identificar la version exacta del firmware para buscar vulnerabilidades conocidas.

**Remediacion:**
- Considerar mover la version a un endpoint protegido
- No es critico para un dispositivo en red local

---

## 4. Gaps de Funcionalidad por Modulo

### ENV — Ambiente

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-01 | Sin reintento en error de sensor | Media | Si `dht.readTemperature()` falla, se reporta error pero no hay logica de reintento. Fallos transitorios se exponen al usuario |
| F-02 | Sin alertas de rango | Baja | No hay mecanismo para alertar si la temperatura o humedad sale de un rango definido (ej. >40C o <10C) |
| F-03 | Sin historial de lecturas | Baja | No se almacena historico de lecturas. Util para graficas y tendencias |

### RLY — Reles

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-04 | Sin confirmacion de estado real | Baja | El estado del rele es solo software. No hay feedback del hardware para confirmar que el rele realmente conmuto |
| F-05 | Sin proteccion de pulsos simultaneos | Baja | Si se envian multiples comandos `pulse` al mismo rele, los timers se sobreescriben sin control |
| F-06 | Sin schedule/timer | Media | No hay manera de programar activaciones automaticas (ej. encender riego a las 7am). Requeriria un modulo de scheduling |

### SEC — Seguridad

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-07 | **Vibracion no polleada** | **Alta** | El pin GPIO14 (vibracion) esta declarado en config pero **no se lee en el loop del modulo**. El sensor de vibracion no funciona |
| F-08 | Event log no persistente | Media | Los eventos se pierden al reiniciar el dispositivo. Deberian guardarse en NVS |
| F-09 | Sin notificacion externa | Media | Al disparar la alarma, solo se notifica via WebSocket a clientes conectados. No hay push notification, email, ni sirena IP |
| F-10 | Sin codigo de desarme | Baja | Solo se puede desarmar via comando WS. Un panel fisico con PIN o uso de NFC (integracion con ACC) seria mas robusto |

### PLG — Enchufe Inteligente

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-11 | Sin factor de potencia | Media | `calc_power` usa `V * I` (potencia aparente). No calcula potencia real con desfase, lo que puede dar lecturas inexactas para cargas inductivas |
| F-12 | **Escritura NVS excesiva** | **Alta** | La energia acumulada (`kwh`) se escribe a NVS en cada ciclo de lectura (cada 2s por defecto). La flash NVS tiene vida limitada (~100,000 ciclos). A 2s/ciclo = ~2.3 dias para agotar la flash |
| F-13 | Sin historial de consumo | Baja | No hay registro historico de consumo (diario, semanal) |
| F-14 | Sin schedule | Baja | No hay programacion horaria ON/OFF |

### NRG — Monitor de Energia

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-15 | Potencia aparente = potencia real | Media | `calc_apparent_power` y `calc_power` usan la misma formula (`V * I`). Para calcular potencia aparente real se necesita muestreo simultaneo de V e I |
| F-16 | **Escritura NVS excesiva** | **Alta** | Identico a F-12. kWh por canal se escribe a NVS cada ciclo de lectura |
| F-17 | Sin alerta por canal | Baja | La alerta de potencia es global, no por canal individual |
| F-18 | Canales 2-3 sin documentar en WebUI | Baja | Los canales 2 y 3 comparten voltaje del canal 0. Esta limitacion no se comunica al usuario en el dashboard |

### ACC — Control de Acceso

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-19 | Event log no persistente | Media | Los eventos de acceso se pierden al reiniciar. Critico para trazabilidad de accesos |
| F-20 | Sin modo "aprender tarjeta" | Baja | No hay un boton fisico ni comando WS para entrar en modo aprendizaje donde la proxima tarjeta se agrega automaticamente |
| F-21 | Sin bloqueo por intentos fallidos | Media | No hay cooldown ni bloqueo tras N intentos de acceso denegado. Un atacante puede probar tarjetas indefinidamente |
| F-22 | Sin backup/export de UIDs | Baja | No hay manera de exportar la lista de UIDs autorizados ni restaurarla en otro dispositivo |

---

## 5. Gaps de Funcionalidad del Core

| # | Gap | Severidad | Descripcion |
|---|-----|-----------|------------|
| F-23 | **WebUI incompleta** | **Alta** | El dashboard solo tiene tarjeta para el modulo ENV. Faltan tarjetas UI para RLY, SEC, PLG, NRG y ACC. Los modulos son funcionales via WS pero no tienen interfaz grafica |
| F-24 | Sin modo AP fallback | Media | Si el ESP32 no puede conectar al WiFi configurado, se queda en un loop infinito de reconexion. Deberia crear un AP propio para permitir reconfiguracion |
| F-25 | Sin watchdog software | Media | No hay watchdog que reinicie el dispositivo si el loop principal se cuelga |
| F-26 | Sin logging persistente | Baja | No hay sistema de log a flash/SD. Solo Serial debug |
| F-27 | Sin backup/restore de config | Baja | No hay mecanismo para exportar/importar toda la configuracion NVS |
| F-28 | Sin indicador LED de estado | Baja | El LED integrado (GPIO2) no se usa para indicar estado del sistema (conectando, conectado, error, alarma) |

---

## 6. Recomendaciones Priorizadas

### P0 — Inmediato (antes de cualquier despliegue)

| # | Accion | Esfuerzo | Vulnerabilidades |
|---|--------|----------|-----------------|
| 1 | **Establecer contrasena OTA** | 5 min | V-01 |
| 2 | **Agregar autenticacion HTTP a `/update`** | 2h | V-02 |
| 3 | **Agregar autenticacion WebSocket** (token en primer mensaje) | 4h | V-03 |
| 4 | **Rate limiting en PIN WiFi** (max 3 intentos/min con lockout) | 2h | V-05 |
| 5 | **Corregir escritura NVS excesiva** en PLG y NRG (escribir solo cada N minutos o al cambiar significativamente) | 1h | F-12, F-16 |

### P1 — Corto plazo (1-2 semanas)

| # | Accion | Esfuerzo | Gaps |
|---|--------|----------|------|
| 6 | **Tarjetas WebUI para 5 modulos** (RLY, SEC, PLG, NRG, ACC) | 8h | F-23 |
| 7 | **Polling del sensor de vibracion** en SEC loop | 30 min | F-07 |
| 8 | **Persistir event logs** (SEC y ACC) en NVS | 2h | F-08, F-19 |
| 9 | **Modo AP fallback** si WiFi falla tras N intentos | 3h | F-24 |
| 10 | **PIN default aleatorio** generado en primer arranque | 1h | V-08 |

### P2 — Medio plazo (1-2 meses)

| # | Accion | Esfuerzo | Gaps |
|---|--------|----------|------|
| 11 | **HTTPS/TLS** con certificado auto-firmado | 8h | V-07 |
| 12 | **Schedule/timer** para modulos RLY y PLG | 8h | F-06, F-14 |
| 13 | **Watchdog software** | 1h | F-25 |
| 14 | **Bloqueo por intentos fallidos** en ACC | 2h | F-21 |
| 15 | **Limite de clientes WS** + timeout de inactividad | 2h | V-09, V-10 |
| 16 | **Validacion Origin** en WS | 1h | V-11 |

### P3 — Largo plazo (3+ meses)

| # | Accion | Esfuerzo | Gaps |
|---|--------|----------|------|
| 17 | **NVS Encryption** | 4h | V-06 |
| 18 | **Secure Boot** (firma de firmware) | 8h | V-04 |
| 19 | **Notificaciones push** (MQTT, Telegram, email) | 12h | F-09 |
| 20 | **Historial de lecturas** con almacenamiento circular | 8h | F-03, F-13 |
| 21 | **Factor de potencia real** en PLG y NRG | 4h | F-11, F-15 |
| 22 | **Flag de compilacion DEBUG** para logs Serial | 1h | V-13 |

---

## 7. Matriz de Riesgo

```
IMPACTO
  ^
  │
  │ CRITICO │  V-06       │  V-01, V-02  │
  │         │             │  V-03, V-04  │
  ├─────────┼─────────────┼──────────────┤
  │ ALTO    │  V-09, V-10 │  V-05, V-08  │
  │         │  F-12, F-16 │  F-07, F-23  │
  ├─────────┼─────────────┼──────────────┤
  │ MEDIO   │  F-03, F-13 │  V-11, V-12  │
  │         │  F-17, F-18 │  F-08, F-19  │
  ├─────────┼─────────────┼──────────────┤
  │ BAJO    │  V-14       │  V-13        │
  │         │  F-04, F-22 │  F-05, F-20  │
  └─────────┴─────────────┴──────────────┘
              BAJA          MEDIA/ALTA
                    PROBABILIDAD →
```

### Leyenda

- **Esquina superior derecha** (alta probabilidad + alto impacto): Accion inmediata requerida
- **Esquina superior izquierda** (baja probabilidad + alto impacto): Plan de mitigacion necesario
- **Esquina inferior derecha** (alta probabilidad + bajo impacto): Mejora continua
- **Esquina inferior izquierda** (baja probabilidad + bajo impacto): Aceptar o diferir

---

## 8. Conclusiones

### Fortalezas

1. **Arquitectura modular bien disenada**: El sistema de registro de modulos con interface uniforme (`smrt_module_t`) facilita la extension y el mantenimiento
2. **Excelente cobertura de tests**: 331 tests unitarios con 100% PASS, cubriendo validacion de parametros, logica de negocio y edge cases
3. **Separacion test/produccion**: El patron `#ifdef UNIT_TEST` permite testear logica pura sin dependencias de hardware
4. **Codigo limpio**: 0 errores y 0 warnings en analisis estatico (cppcheck)
5. **Persistencia NVS por modulo**: Cada modulo tiene su namespace NVS, evitando colisiones

### Debilidades criticas

1. **Seguridad de red inexistente**: Sin autenticacion en ninguna capa (WS, HTTP OTA, ArduinoOTA)
2. **Credenciales en texto plano**: WiFi password y PIN almacenados sin cifrado
3. **WebUI incompleta**: Solo 1 de 6 modulos tiene interfaz grafica
4. **Desgaste de flash**: PLG y NRG escriben a NVS con frecuencia excesiva

### Recomendacion final

Antes de desplegar en un entorno real, es **imprescindible** implementar al menos las acciones P0:

1. Contrasena OTA
2. Autenticacion HTTP para OTA web
3. Autenticacion WebSocket
4. Rate limiting en PIN
5. Reducir escrituras NVS

Estas 5 acciones elevan la seguridad de **CRITICA** a **MEDIA**, convirtiendo el dispositivo en utilizable para redes locales domesticas de confianza.

---

> **Nota:** Esta auditoria es una revision estatica de codigo. Una auditoria completa requeriria adicionalmente: penetration testing con hardware real, fuzzing de protocolos, analisis de side-channels y revision de cadena de suministro de dependencias.

---

> **HomeNode v0.4.0** — Auditoria de Seguridad y Funcionalidad
> Generada: 2026-03-14
