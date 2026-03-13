# HomeNode — Plataforma IoT Domestica Modular

Plataforma modular de IoT domestico basada en **ESP32-WROOM** (NodeMCU-32S)
con firmware CORE compartido y modulos intercambiables para diferentes
aplicaciones domesticas.

---

## Estado del Proyecto

| Fase | Nombre | Estado |
|------|--------|--------|
| FASE 1 | Definicion de plataforma | **EN PROGRESO** |
| FASE 2 | Arquitectura firmware | PLANIFICADO |
| FASE 3 | Sistema de modulos | PLANIFICADO |
| FASE 4 | Interfaces del sistema | PLANIFICADO |
| FASE 5 | Automatizacion | PLANIFICADO |
| FASE 6 | Firmware base | PLANIFICADO |
| FASE 7 | QA y estabilidad | PLANIFICADO |
| FASE 8 | PCB base | PLANIFICADO |
| FASE 9 | Modulos hardware | PLANIFICADO |
| FASE 10 | Carcasas 3D | PLANIFICADO |

---

## Arquitectura

```
HomeNode/
├── include/
│   ├── core/              # Headers CORE (WiFi, WS, OTA, NVS, GPIO, serial, types)
│   └── modules/           # Headers de modulos (env, sec, relay, plug, access, energy)
├── src/
│   ├── core/              # Fuentes CORE
│   └── modules/           # Fuentes de modulos
├── test/
│   └── test_native/       # Tests unitarios (Unity framework)
├── docs/                  # Documentacion de requisitos y arquitectura
├── lib/                   # Librerias externas
├── data/                  # Archivos SPIFFS/LittleFS
├── reports/               # Reportes QA (auto-generados, en .gitignore)
├── platformio.ini         # Configuracion PlatformIO
├── smrt_qa.py             # Script de QA automatizado
├── claude.md              # Metodologia de trabajo
└── claude_rules.xml       # Reglas exportables XML
```

---

## Modulos Objetivo

| Modulo | Descripcion | Flag de compilacion |
|--------|-------------|---------------------|
| Environmental | Temperatura, humedad, presion, luz | `SMRT_MOD_ENV` |
| Security | PIR, reed switch, vibracion, alarma | `SMRT_MOD_SEC` |
| Relay | Control de dispositivos electricos (1-4 reles) | `SMRT_MOD_RELAY` |
| Smart Plug | Control remoto + medicion de energia | `SMRT_MOD_PLUG` |
| Access Control | NFC/RFID + control de cerradura | `SMRT_MOD_ACCESS` |
| Energy Monitor | Monitorizacion de consumo electrico | `SMRT_MOD_ENERGY` |

---

## Hardware

- **MCU:** ESP32-WROOM-32 (NodeMCU-32S)
- **Conectividad:** WiFi 802.11 b/g/n
- **Buses:** I2C, SPI, UART
- **ADC:** 4 entradas analogicas (GPIO34-39)
- **GPIO:** 20+ pines de expansion para modulos

---

## Compilacion

```bash
# Firmware produccion (ESP32)
pio run -e nodemcu-32s

# Upload OTA via WiFi
pio run -e ota -t upload

# Tests unitarios (PC, sin hardware)
pio test -e native -v

# Suite QA completa (tests + analisis + metricas)
python smrt_qa.py

# Solo metricas
python smrt_qa.py --metrics

# Solo analisis estatico
python smrt_qa.py --analysis
```

---

## Metodologia

Todo el desarrollo sigue las reglas definidas en:

- `claude.md` — Flujo de trabajo y estandares de desarrollo
- `claude_rules.xml` — Reglas exportables en formato XML
- `docs/requirements.md` — Requisitos funcionales de la plataforma

---

## Proyecto de Referencia

**Smart_Lock v1.3.0** (`../Smart_Lock/`) es el proyecto original del cual
se extrae el CORE de HomeNode. Se mantiene como referencia funcional con:

- ~3,600 LOC
- 85 unit tests (100% PASS)
- 0 errores cppcheck
- WiFi, WebSocket, NFC/RFID, OTA, Web UI

---

*HomeNode v0.1.0 — FASE 1: Definicion de Plataforma*
