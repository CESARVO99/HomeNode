/**
 * @file    smrt_core_crypto.h
 * @brief   AES-128-CBC encryption/decryption and encrypted NVS wrapper
 * @project HOMENODE
 * @version 0.8.0
 *
 * Provides application-level encryption for sensitive NVS values
 * and optional WebSocket message encryption. Uses mbedTLS (built
 * into ESP-IDF) for AES-128-CBC with PKCS7 padding.
 *
 * Key derivation: HMAC-SHA256(ESP.getEfuseMac(), "HomeNode-NVS-v1")
 * truncated to 16 bytes.
 *
 * Encrypted strings stored in NVS are prefixed with "ENC:" followed
 * by base64(IV + ciphertext). Migration is transparent: reading a
 * plaintext value auto-encrypts it on next write.
 */

#ifndef SMRT_CORE_CRYPTO_H
#define SMRT_CORE_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define SMRT_CRYPTO_KEY_LEN         16      /**< AES-128 key length (bytes) */
#define SMRT_CRYPTO_IV_LEN          16      /**< AES-CBC IV length (bytes)  */
#define SMRT_CRYPTO_ENC_PREFIX      "ENC:"  /**< Prefix for encrypted NVS values */
#define SMRT_CRYPTO_ENC_PREFIX_LEN  4

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates an AES key length
 * @param  key_len  Key length in bytes
 * @return 1 if valid (16, 24, or 32), 0 otherwise
 */
int smrt_crypto_validate_key_len(int key_len);

/**
 * @brief  Checks if a string is an encrypted NVS value (has "ENC:" prefix)
 * @param  value  String to check
 * @return 1 if encrypted, 0 if plaintext
 */
int smrt_crypto_is_encrypted(const char *value);

/**
 * @brief  Base64 encode a byte buffer
 * @param  input      Input byte buffer
 * @param  input_len  Input length
 * @param  output     Output base64 string buffer
 * @param  output_max Maximum output buffer size
 * @return Length of encoded string, or 0 on error
 */
int smrt_crypto_base64_encode(const unsigned char *input, int input_len,
                               char *output, int output_max);

/**
 * @brief  Base64 decode a string to byte buffer
 * @param  input       Input base64 string
 * @param  output      Output byte buffer
 * @param  output_max  Maximum output buffer size
 * @return Length of decoded bytes, or 0 on error
 */
int smrt_crypto_base64_decode(const char *input, unsigned char *output,
                               int output_max);

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

/**
 * @brief  Initializes the crypto subsystem (derives key from chip MAC)
 * @return void
 */
void smrt_crypto_init(void);

/**
 * @brief  Encrypts a plaintext string using AES-128-CBC
 * @param  plaintext  Null-terminated string to encrypt
 * @param  out_buf    Output buffer for "ENC:" + base64(IV + ciphertext)
 * @param  out_max    Maximum output buffer size
 * @return 1 on success, 0 on error
 */
int smrt_crypto_encrypt_string(const char *plaintext, char *out_buf, int out_max);

/**
 * @brief  Decrypts an encrypted string (expects "ENC:" + base64 format)
 * @param  encrypted  Encrypted string with "ENC:" prefix
 * @param  out_buf    Output buffer for decrypted plaintext
 * @param  out_max    Maximum output buffer size
 * @return 1 on success, 0 on error (including if input is plaintext)
 */
int smrt_crypto_decrypt_string(const char *encrypted, char *out_buf, int out_max);

/**
 * @brief  Saves an encrypted string to NVS
 * @param  ns     Namespace
 * @param  key    Key name
 * @param  value  Plaintext value to encrypt and store
 * @return void
 */
void smrt_nvs_set_string_enc(const char *ns, const char *key, const char *value);

/**
 * @brief  Loads and decrypts a string from NVS (transparent migration)
 * @param  ns       Namespace
 * @param  key      Key name
 * @param  buf      Output buffer for decrypted plaintext
 * @param  buf_len  Maximum buffer size
 * @return true if key found, false otherwise
 */
bool smrt_nvs_get_string_enc(const char *ns, const char *key, char *buf, size_t buf_len);

/**
 * @brief  Returns the session encryption key for WS encryption
 * @param  pin       PIN string used for key derivation
 * @param  key_out   Output buffer (must be SMRT_CRYPTO_KEY_LEN bytes)
 * @return void
 */
void smrt_crypto_derive_session_key(const char *pin, unsigned char *key_out);

#endif // UNIT_TEST

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_CRYPTO_H
