/**
 * @file    smrt_core_backup.h
 * @brief   Configuration backup/export and restore/import via WebSocket
 * @project HOMENODE
 * @version 0.8.0
 */

#ifndef SMRT_CORE_BACKUP_H
#define SMRT_CORE_BACKUP_H

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates a namespace string for backup
 * @param  ns  Namespace string
 * @return 1 if valid (non-null, non-empty, max 15 chars), 0 otherwise
 */
int smrt_backup_validate_namespace(const char *ns);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/** @brief Handles backup WebSocket commands (cfg_export, cfg_import) */
void smrt_backup_ws_handler(const char *cmd, void *json_doc, uint32_t client_id);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_BACKUP_H
