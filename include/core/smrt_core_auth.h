/**
 * @file    smrt_core_auth.h
 * @brief   Authentication, rate limiting, session timeout and WS management
 * @project HOMENODE
 * @version 0.6.0
 *
 * Provides:
 *   - WebSocket client authentication tracking (per client ID)
 *   - Session inactivity timeout with auto-logout
 *   - PIN rate limiting with lockout after N failed attempts
 *   - HTTP Basic Auth validation for OTA endpoint
 */

#ifndef SMRT_CORE_AUTH_H
#define SMRT_CORE_AUTH_H

#ifndef UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes the authentication subsystem.
 *         Clears all tracked sessions and resets rate limiter.
 * @return void
 */
void smrt_auth_init(void);

/**
 * @brief  Marks a WebSocket client as authenticated.
 * @param  client_id  AsyncWebSocketClient ID
 * @return 1 on success, 0 if slots full
 */
int smrt_auth_ws_login(uint32_t client_id);

/**
 * @brief  Removes a WebSocket client from authenticated list.
 * @param  client_id  AsyncWebSocketClient ID
 * @return void
 */
void smrt_auth_ws_logout(uint32_t client_id);

/**
 * @brief  Checks if a WebSocket client is authenticated.
 * @param  client_id  AsyncWebSocketClient ID
 * @return 1 if authenticated, 0 otherwise
 */
int smrt_auth_ws_is_authenticated(uint32_t client_id);

/**
 * @brief  Records a failed PIN attempt and checks if lockout is active.
 * @return 1 if locked out (too many attempts), 0 if attempt allowed
 */
int smrt_auth_pin_is_locked(void);

/**
 * @brief  Records a failed PIN attempt.
 * @return void
 */
void smrt_auth_pin_fail(void);

/**
 * @brief  Resets the failed PIN attempt counter (call on success).
 * @return void
 */
void smrt_auth_pin_reset(void);

/**
 * @brief  Returns remaining lockout time in seconds, 0 if not locked.
 * @return Seconds remaining in lockout, or 0
 */
unsigned long smrt_auth_pin_lockout_remaining(void);

/**
 * @brief  Updates last-activity timestamp for an authenticated client.
 * @param  client_id  WebSocket client ID
 * @return void
 */
void smrt_auth_ws_touch(uint32_t client_id);

/**
 * @brief  Logs out clients idle longer than SMRT_AUTH_SESSION_TIMEOUT_MS.
 *         Call from the main loop.
 * @return void
 */
void smrt_auth_ws_cleanup_expired(void);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST

#endif // SMRT_CORE_AUTH_H
