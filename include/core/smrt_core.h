/**
 * @file    smrt_core.h
 * @brief   Master umbrella header — includes all core platform headers
 * @project HOMENODE
 * @version 0.4.1
 *
 * All source files in src/core/ and src/modules/ should include this
 * single header instead of individual core headers.
 */

#ifndef SMRT_CORE_H
#define SMRT_CORE_H

//-----------------------------------------------------------------------------
// Arduino / ESP32 framework includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#endif

//-----------------------------------------------------------------------------
// Core platform headers (order matters: config first, then HAL, then services)
//-----------------------------------------------------------------------------
#include "smrt_core_config.h"       /**< Platform configuration defines */
#include "smrt_mc_format.h"         /**< Data types and conversion utilities */
#include "smrt_mc_serial.h"         /**< Serial port management */
#include "smrt_mc_gpio.h"           /**< GPIO control */
#include "smrt_core_module.h"       /**< Module registration system */

#ifndef UNIT_TEST
#include "smrt_core_nvs.h"         /**< NVS persistence API */
#include "smrt_core_auth.h"        /**< Authentication & rate limiting */
#include "smrt_core_wifi.h"        /**< WiFi management */
#include "smrt_core_ws.h"          /**< WebSocket server */
#include "smrt_core_http.h"        /**< HTTP server */
#include "smrt_core_ota.h"         /**< OTA update */
#include "smrt_core_webui.h"       /**< Embedded web interface */
#endif

#endif // SMRT_CORE_H
