/**
 * @file    smrt_core_ota.h
 * @brief   OTA (Over-The-Air) update — ArduinoOTA + HTTP upload
 * @project HOMENODE
 * @version 0.2.0
 */

#ifndef SMRT_CORE_OTA_H
#define SMRT_CORE_OTA_H

#ifndef UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes ArduinoOTA service with hostname and callbacks.
 *         Call once in setup() after WiFi is connected.
 * @return void
 */
void smrt_ota_init(void);

/**
 * @brief  Registers the HTTP OTA upload endpoint on the async web server.
 *         GET  /update -> serves the upload page
 *         POST /update -> receives the firmware binary and applies it
 * @return void
 */
void smrt_ota_web_init(void);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST

#endif // SMRT_CORE_OTA_H
