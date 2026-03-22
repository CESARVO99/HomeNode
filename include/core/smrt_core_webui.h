/**
 * @file    smrt_core_webui.h
 * @brief   Web interface — served from LittleFS filesystem
 * @project HOMENODE
 * @version 1.3.0
 *
 * The dashboard HTML/CSS/JS lives in data/index.html and is uploaded
 * to the ESP32's LittleFS partition via `pio run -t uploadfs`.
 *
 * This header is kept for backward compatibility with smrt_core.h includes.
 * The actual serving is done in smrt_core_http.cpp via LittleFS.
 */

#ifndef SMRT_CORE_WEBUI_H
#define SMRT_CORE_WEBUI_H

/* No PROGMEM content — WebUI served from LittleFS */

#endif // SMRT_CORE_WEBUI_H
