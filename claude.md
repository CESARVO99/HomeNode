# CLAUDE.md — Metodologia de Trabajo y Estandares de Desarrollo

> **Archivo generico de metodologia para proyectos IoT embebidos.**
> Cada proyecto debe personalizar la seccion [Descripcion del Proyecto](#descripcion-del-proyecto).

---

## Descripcion del Proyecto

<!-- ============================================================
     INSTRUCCION: Al iniciar una sesion de trabajo, Claude debe
     preguntar al usuario que cambios/features se van a implementar
     y actualizar esta seccion con la descripcion especifica.
     ============================================================ -->

**Proyecto:** HomeNode — Plataforma IoT domestica modular
**Plataforma:** ESP32-WROOM (NodeMCU-32S)
**Framework:** Arduino (PlatformIO)
**Lenguaje:** C/C++ (estandar C++11)
**IDE:** Visual Studio Code + PlatformIO

### Features/Cambios Pendientes

> _Claude debe pedir al usuario una descripcion de los cambios o features
> a implementar al inicio de cada sesion. Documentar aqui:_

- [ ] _(Describir feature o cambio 1)_
- [ ] _(Describir feature o cambio 2)_

---

## Flujo de Trabajo (Workflow)

El flujo de trabajo sigue estos pasos en orden. Claude debe ejecutarlos
secuencialmente y reportar el estado de cada paso.

### Paso 1 — Recopilacion de Requisitos

1. **Preguntar** al usuario que cambios/features se van a realizar.
2. **Ofrecer opciones** cuando haya multiples enfoques posibles.
3. **Documentar** la descripcion en la seccion anterior.
4. **Confirmar** el alcance antes de comenzar a codificar.

> **Regla:** Siempre preguntar ante dudas. Dar opciones y dejar que el
> usuario describa mejor si lo prefiere.

### Paso 2 — Desarrollo (Codificacion)

1. **Seguir los estandares de codigo existentes** del proyecto (ver seccion
   [Estandares de Codigo](#estandares-de-codigo)).
2. **Documentar con Doxygen** todas las funciones, structs y macros nuevas.
3. **Adaptar el codigo para unit tests:**
   - Usar `#ifdef UNIT_TEST` para aislar dependencias de hardware.
   - Usar `extern "C"` en headers para compatibilidad C/C++.
   - Agregar guards `#ifndef exclude_<functionName>` para exclusion
     condicional en builds de produccion.
4. **Mantener la modularidad:** Un modulo = un `.cpp` + un `.h`.

### Paso 3 — Limpieza de Codigo

1. **Eliminar codigo redundante/spaghetti** del directorio `src/`.
2. **Eliminar variables, includes y defines no utilizados.**
3. **Verificar** que no queden `// TODO`, `// FIXME` o codigo comentado
   sin justificacion.

### Paso 4 — Refactorizacion y Analisis Estatico

1. **Ejecutar analisis estatico** para detectar issues:
   ```bash
   cppcheck --enable=all --suppress=missingInclude --suppress=unknownMacro --std=c++11 -I include -I include/core -I include/modules src/ include/
   ```
2. **Corregir errores de cppcheck:**
   - `shiftTooManyBitsSigned` — Reemplazar shifts de bits en signed por
     comparaciones directas (`value < 0` en lugar de `value >> 31`).
   - `cstyleCast` — Reemplazar casts estilo C `(tipo)expr` por variables
     locales tipadas o casts C++ (`static_cast<>`).
   - `constParameterPointer` — Anadir `const` a parametros puntero que
     no se modifican dentro de la funcion.
   - `variableScope` — Mover declaraciones de variables al scope mas
     reducido posible (dentro de bucles si solo se usan ahi).
3. **Reducir complejidad ciclomatica** de funciones con CC > 10:
   - **Extraer subfunciones** para bloques de logica independiente.
   - **Usar early return** en lugar de anidamiento profundo.
   - **Crear funciones helper** `static` para validacion y conversion.
   - Objetivo: todas las funciones con CC <= 10.
4. **Actualizar headers** para que los prototipos coincidan con las
   firmas refactorizadas (especialmente cambios de `char *` a `const char *`).
5. **Bumping de version:** Incrementar `@version` en archivos modificados.

> **Regla:** Cada refactorizacion debe seguirse de compilacion + tests
> para verificar que no se ha roto nada.

### Paso 5 — Compilacion y Correccion

1. **Compilar** el entorno de produccion:
   ```bash
   pio run -e nodemcu-32s
   ```
2. **Compilar** el entorno de tests nativos:
   ```bash
   pio test -e native
   ```
3. **Corregir** todos los errores y warnings de compilacion.
4. **Repetir** hasta que ambos entornos compilen sin errores.

### Paso 6 — QA: Tests, Metricas y Analisis

1. **Ejecutar el script de QA:**
   ```bash
   python smrt_qa.py
   ```
   Esto genera en `reports/<timestamp>/`:
   - `test_results.txt` — Resultados de tests unitarios
   - `static_analysis.txt` — Analisis estatico (cppcheck)
   - `metrics.txt` — Metricas de codigo
   - `summary.txt` — Resumen consolidado
   - `changelog.md` — Registro de cambios realizados

2. **Criterios de aceptacion:**
   - Tests unitarios: **100% PASS** (todos deben pasar)
   - Analisis estatico: **0 errores** (warnings y style son informativos)
   - Complejidad ciclomatica: ver umbrales en [Metricas](#metricas-y-complejidad-ciclomatica)

3. **Si hay fallos:**
   - Corregir -> recompilar -> re-ejecutar QA
   - Iterar hasta cumplir criterios

### Paso 7 — Commit y Push

1. **Agregar** los archivos modificados:
   ```bash
   git add <archivos especificos>
   ```
2. **Crear commit** con descripcion completa:
   ```bash
   git commit -m "$(cat <<'EOF'
   <tipo>: <descripcion corta>

   Cambios realizados:
   - <detalle 1>
   - <detalle 2>

   Tests: XX/XX PASS
   Analisis: X errores, X warnings
   Metricas: XXXX LOC, XX funciones

   Co-Authored-By: Claude <noreply@anthropic.com>
   EOF
   )"
   ```
3. **Push** al repositorio remoto:
   ```bash
   git push origin master
   ```

### Paso 8 — Instalacion de Herramientas (si necesario)

Si alguna herramienta no esta instalada, Claude debe:

1. **Detectar** la ausencia automaticamente.
2. **Informar** al usuario que herramienta falta.
3. **Ofrecer** instalarla (con confirmacion del usuario).
4. **Instalar** usando el metodo mas conveniente:

| Herramienta | Comando de instalacion |
|-------------|----------------------|
| PlatformIO | `pip install platformio` o extension VS Code |
| cppcheck | `winget install cppcheck` o incluido en WinLibs |
| GCC/G++ | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |
| Python 3 | `winget install Python.Python.3` |
| Git | `winget install Git.Git` |

---

## Estandares de Codigo

### Convenciones de Nombres

| Elemento | Formato | Ejemplo |
|----------|---------|---------|
| Funciones | camelCase con prefijo modulo | `smrtTypeSetUnsigned()` |
| Macros/Defines | UPPER_SNAKE con prefijo SMRT_ | `SMRT_TYPE_SWAP_16` |
| Tipos custom | camelCase sin prefijo | `uint8`, `int32`, `bit` |
| Variables locales | camelCase | `strLen`, `pValue` |
| Constantes | UPPER_SNAKE | `NFC_UID_MAX_SIZE` |
| Archivos fuente | snake_case con prefijo smrt_ | `smrt_mc_format.cpp` |
| Archivos header | snake_case con prefijo smrt_ | `smrt_mc_format.h` |
| Archivos test | prefijo test_ | `test_smrt_mc_format.cpp` |

### Estructura de Archivos

```
proyecto/
├── include/
│   ├── core/              # Headers CORE de la plataforma
│   └── modules/           # Headers de modulos especificos
├── src/
│   ├── core/              # Fuentes CORE de la plataforma
│   └── modules/           # Fuentes de modulos especificos
├── test/
│   └── test_native/       # Tests unitarios nativos
├── lib/                   # Librerias externas
├── data/                  # Archivos SPIFFS/LittleFS
├── docs/                  # Documentacion (requisitos, arquitectura)
├── reports/               # Reportes QA (en .gitignore)
├── platformio.ini         # Configuracion PlatformIO
├── smrt_qa.py             # Script de QA automatizado
├── add_gcc_path.py        # Auto-deteccion WinLibs GCC
├── claude.md              # Este archivo (metodologia)
├── claude_rules.xml       # Reglas exportables XML
└── .gitignore
```

### Documentacion Doxygen

Toda funcion publica debe tener documentacion Doxygen completa:

```c
/**
 * @file    nombre_archivo.h
 * @brief   Descripcion breve del modulo
 * @project HOMENODE
 * @version X.Y.Z
 */

/**
 * @brief  Descripcion breve de la funcion
 * @param  paramName  Descripcion del parametro
 * @return Descripcion del valor de retorno
 */
```

### Guardas para Unit Tests

```cpp
// En el header (.h):
#ifndef UNIT_TEST
#define exclude_functionName    // Excluir en produccion
#endif

// En el source (.cpp):
#ifdef UNIT_TEST
    #include "module.h"
#else
    #include "main_header.h"
#endif

// En el test (.cpp):
#include <unity.h>
#include "module.h"
#include "../../src/core/module.cpp"  // Inclusion directa del fuente
```

### Guardas de Header

```cpp
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

// ... declaraciones ...

#ifdef __cplusplus
}
#endif

#endif // MODULE_NAME_H
```

---

## Metricas y Complejidad Ciclomatica

### Indicadores Monitorizados

| Metrica | Descripcion | Umbral Recomendado |
|---------|-------------|--------------------|
| **Complejidad ciclomatica** | Numero de caminos independientes por funcion | <= 10 (simple), <= 20 (moderado), > 20 (refactorizar) |
| **Lineas por funcion** | Promedio de LOC por funcion | <= 50 lineas |
| **Densidad de comentarios** | Ratio comentarios/codigo | >= 15% |
| **Ratio test/funcion** | Tests unitarios por funcion | >= 100% (al menos 1 test por funcion) |
| **Sentencias if** | Decisiones condicionales | Indicador de complejidad |
| **Bucles for/while** | Iteraciones | Indicador de complejidad |
| **Switch statements** | Alternativas multiples | Indicador de complejidad |

### Calculo de Complejidad Ciclomatica

La complejidad ciclomatica (CC) se calcula por funcion como:

```
CC = 1 + numero_de(if) + numero_de(else if) + numero_de(for)
   + numero_de(while) + numero_de(case) + numero_de(&&) + numero_de(||)
   + numero_de(?) + numero_de(catch)
```

### Herramientas de Analisis (Gratuitas)

| Herramienta | Proposito | Comando |
|-------------|-----------|---------|
| **cppcheck** | Analisis estatico (bugs, estilo, rendimiento) | `cppcheck --enable=all src/ include/` |
| **PlatformIO Unity** | Tests unitarios nativos | `pio test -e native -v` |
| **smrt_qa.py** | Automatizacion QA completa | `python smrt_qa.py` |
| **GCC -Wall -Wextra** | Warnings del compilador | Incluido en build flags |
| **Python regex** | Metricas de codigo (LOC, funciones, complejidad) | Integrado en smrt_qa.py |

---

## Automatizacion (Scripts)

### Script Principal: `smrt_qa.py`

```bash
# Ejecutar todo (tests + analisis + metricas + changelog)
python smrt_qa.py

# Solo tests
python smrt_qa.py --tests

# Solo analisis estatico
python smrt_qa.py --analysis

# Solo metricas
python smrt_qa.py --metrics

# Ver ultimo reporte
python smrt_qa.py --summary
```

### Estructura de Reportes

```
reports/
└── YYYY-MM-DD_HH-MM-SS/
    ├── test_results.txt        # Salida de pio test
    ├── static_analysis.txt     # Salida de cppcheck
    ├── metrics.txt             # Metricas de codigo
    ├── summary.txt             # Resumen consolidado
    └── changelog.md            # Registro de cambios
```

### Changelog Automatico

Cada ejecucion de QA debe generar un `changelog.md` en la subcarpeta
del reporte con el siguiente formato:

```markdown
# Changelog — YYYY-MM-DD HH:MM:SS

## Cambios Realizados
- [ detalle de cada cambio ]

## Archivos Modificados
- `path/to/file.cpp` — Descripcion del cambio

## Resultados QA
- Tests: XX/XX PASS
- Analisis: X errores, X warnings, X style
- Metricas: XXXX LOC, XX funciones
```

---

## Reglas Exportables (XML)

Las reglas de trabajo estan tambien definidas en `claude_rules.xml` para
poder ser importadas/exportadas entre proyectos. Ver archivo adjunto.

---

## Instrucciones para Claude

### Al Iniciar una Sesion

1. **Leer** este archivo `claude.md` para conocer la metodologia.
2. **Preguntar** al usuario:
   - _"Que cambios o features vamos a implementar hoy?"_
   - _"Hay algun requisito especifico o restriccion?"_
3. **Documentar** la respuesta en la seccion [Descripcion del Proyecto](#descripcion-del-proyecto).
4. **Ofrecer opciones** si hay multiples enfoques posibles.

### Durante el Desarrollo

1. **Seguir** el flujo de trabajo paso a paso.
2. **Preguntar** ante cualquier duda.
3. **Dar opciones** cuando haya multiples soluciones.
4. **No asumir** — siempre confirmar con el usuario.
5. **Reportar** el progreso de cada paso.

### Ante Errores

1. **Mostrar** el error completo al usuario.
2. **Explicar** la causa probable.
3. **Proponer** soluciones (minimo 2 opciones cuando sea posible).
4. **Implementar** la solucion elegida por el usuario.

### Al Finalizar

1. **Ejecutar** el script de QA completo.
2. **Generar** el changelog con los cambios realizados.
3. **Mostrar** el resumen al usuario.
4. **Preguntar** si hacer commit y push.
5. **Crear** commit con descripcion completa de todo lo realizado.

---

## Configuracion del Entorno

### PlatformIO (platformio.ini)

```ini
[platformio]
default_envs = nodemcu-32s

; Configuracion comun ESP32
[env_common_esp32]
platform = espressif32
board = nodemcu-32s
framework = arduino
monitor_speed = 115200
build_flags =
    -I include
    -I include/core
    -I include/modules
    -D SMRT_PLATFORM_HOMENODE
    -std=c++11
lib_deps =
    wifi
    ESP Async WebServer
    bblanchon/ArduinoJson@^7

; Produccion - ESP32
[env:nodemcu-32s]
extends = env_common_esp32
upload_speed = 115200

; OTA - WiFi
[env:ota]
extends = env_common_esp32
upload_protocol = espota
upload_port = 192.168.1.100
upload_flags = --port=3232

; Tests nativos - PC
[env:native]
platform = native
extra_scripts = pre:add_gcc_path.py
build_flags = -I include -I include/core -I include/modules -D UNIT_TEST -D SMRT_PLATFORM_HOMENODE
build_src_filter = -<*>
lib_deps =
lib_ignore =
    ESP Async WebServer
    ESPAsyncTCP
    AsyncTCP
    wifi
test_framework = unity
```

### Archivos Ignorados (.gitignore)

```
.pio
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json
.vscode/ipch
reports/
```

---

## Historial de Metodologia

| Version | Fecha | Cambios |
|---------|-------|---------|
| 1.0.0 | 2026-03-13 | Version inicial HomeNode — Adaptado de Smart_Lock v1.1.0. Estructura core/modules, proyecto @HOMENODE |
