/**
 * @file    smrt_core_http.h
 * @brief   HTTP server initialization and global server/websocket instances
 * @project HOMENODE
 * @version 0.2.0
 */

#ifndef SMRT_CORE_HTTP_H
#define SMRT_CORE_HTTP_H

#ifndef UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes the HTTP server: registers routes, WS, OTA, and starts listening.
 * @return void
 */
void smrt_http_init(void);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST

#endif // SMRT_CORE_HTTP_H
