/**
 * @file    smrt_core_crypto.cpp
 * @brief   AES-128-CBC encryption using mbedTLS + encrypted NVS wrapper
 * @project HOMENODE
 * @version 0.8.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifdef UNIT_TEST
    #include "smrt_core_crypto.h"
    #include <cstring>
    #include <cstdlib>
#else
    #include "smrt_core.h"
    #include <mbedtls/aes.h>
    #include <mbedtls/md.h>
    #include <mbedtls/base64.h>
    #include <esp_system.h>
#endif

//-----------------------------------------------------------------------------
// Testable utility functions (always compiled)
//-----------------------------------------------------------------------------

/**
 * @brief  Validates an AES key length
 * @param  key_len  Key length in bytes
 * @return 1 if valid (16, 24, or 32), 0 otherwise
 */
int smrt_crypto_validate_key_len(int key_len) {
    return (key_len == 16 || key_len == 24 || key_len == 32) ? 1 : 0;
}

/**
 * @brief  Checks if a string has the "ENC:" prefix
 * @param  value  String to check
 * @return 1 if encrypted, 0 if plaintext or NULL
 */
int smrt_crypto_is_encrypted(const char *value) {
    if (!value) return 0;
    if (strlen(value) < SMRT_CRYPTO_ENC_PREFIX_LEN) return 0;
    return (strncmp(value, SMRT_CRYPTO_ENC_PREFIX, SMRT_CRYPTO_ENC_PREFIX_LEN) == 0) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Base64 — portable implementation for testability
//-----------------------------------------------------------------------------

static const char b64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief  Base64 encode
 */
int smrt_crypto_base64_encode(const unsigned char *input, int input_len,
                               char *output, int output_max) {
    int encoded_len = 4 * ((input_len + 2) / 3);
    if (encoded_len + 1 > output_max) return 0;

    int i = 0, j = 0;
    while (i < input_len) {
        unsigned int a = (i < input_len) ? input[i++] : 0;
        unsigned int b = (i < input_len) ? input[i++] : 0;
        unsigned int c = (i < input_len) ? input[i++] : 0;
        unsigned int triple = (a << 16) | (b << 8) | c;

        output[j++] = b64_chars[(triple >> 18) & 0x3F];
        output[j++] = b64_chars[(triple >> 12) & 0x3F];
        output[j++] = (i > input_len + 1) ? '=' : b64_chars[(triple >> 6) & 0x3F];
        output[j++] = (i > input_len)     ? '=' : b64_chars[triple & 0x3F];
    }
    output[j] = '\0';
    return j;
}

/**
 * @brief  Base64 decode helper — char to index
 */
static int b64_char_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

/**
 * @brief  Base64 decode
 */
int smrt_crypto_base64_decode(const char *input, unsigned char *output,
                               int output_max) {
    int input_len = (int)strlen(input);
    if (input_len % 4 != 0) return 0;

    int out_len = (input_len / 4) * 3;
    if (input_len > 0 && input[input_len - 1] == '=') out_len--;
    if (input_len > 1 && input[input_len - 2] == '=') out_len--;
    if (out_len > output_max) return 0;

    int i = 0, j = 0;
    while (i < input_len) {
        int a = b64_char_index(input[i++]);
        int b = b64_char_index(input[i++]);
        int c = (input[i] == '=') ? 0 : b64_char_index(input[i]); i++;
        int d = (input[i] == '=') ? 0 : b64_char_index(input[i]); i++;

        if (a < 0 || b < 0) return 0;

        unsigned int triple = ((unsigned int)a << 18) | ((unsigned int)b << 12) |
                              ((unsigned int)c << 6)  | (unsigned int)d;

        if (j < out_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < out_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < out_len) output[j++] = triple & 0xFF;
    }
    return out_len;
}

//-----------------------------------------------------------------------------
// Hardware-dependent functions (ESP32 only)
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST

//-----------------------------------------------------------------------------
// Static state — derived AES key
//-----------------------------------------------------------------------------
static unsigned char smrt_crypto_key[SMRT_CRYPTO_KEY_LEN];
static bool smrt_crypto_initialized = false;

/**
 * @brief  Initializes the crypto subsystem — derives key from chip MAC
 * @return void
 */
void smrt_crypto_init(void) {
    /* Get unique chip MAC */
    uint64_t mac = ESP.getEfuseMac();
    uint8_t mac_bytes[8];
    memcpy(mac_bytes, &mac, 8);

    /* HMAC-SHA256(mac, salt) → truncate to 16 bytes */
    const char *salt = "HomeNode-NVS-v1";
    unsigned char hmac_out[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, mac_bytes, 8);
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)salt, strlen(salt));
    mbedtls_md_hmac_finish(&ctx, hmac_out);
    mbedtls_md_free(&ctx);

    memcpy(smrt_crypto_key, hmac_out, SMRT_CRYPTO_KEY_LEN);
    smrt_crypto_initialized = true;
    SMRT_DEBUG_LOG("Crypto subsystem initialized");
}

/**
 * @brief  Derives a session key for WS encryption from PIN + chip ID
 * @param  pin       PIN string
 * @param  key_out   Output buffer (16 bytes)
 * @return void
 */
void smrt_crypto_derive_session_key(const char *pin, unsigned char *key_out) {
    uint64_t mac = ESP.getEfuseMac();
    uint8_t mac_bytes[8];
    memcpy(mac_bytes, &mac, 8);

    /* Concatenate PIN + MAC as HMAC key, salt as data */
    unsigned char combined[72];
    int pin_len = strlen(pin);
    memcpy(combined, pin, pin_len);
    memcpy(combined + pin_len, mac_bytes, 8);

    unsigned char hmac_out[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, combined, pin_len + 8);
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)"HomeNode-WS-v1", 14);
    mbedtls_md_hmac_finish(&ctx, hmac_out);
    mbedtls_md_free(&ctx);

    memcpy(key_out, hmac_out, SMRT_CRYPTO_KEY_LEN);
}

/**
 * @brief  Encrypts a plaintext string using AES-128-CBC with PKCS7 padding
 * @param  plaintext  String to encrypt
 * @param  out_buf    Output: "ENC:" + base64(IV + ciphertext)
 * @param  out_max    Max output buffer size
 * @return 1 on success, 0 on error
 */
int smrt_crypto_encrypt_string(const char *plaintext, char *out_buf, int out_max) {
    if (!smrt_crypto_initialized || !plaintext || !out_buf) return 0;

    int pt_len = strlen(plaintext);
    /* PKCS7 padding: pad to 16-byte boundary */
    int pad_len = SMRT_CRYPTO_KEY_LEN - (pt_len % SMRT_CRYPTO_KEY_LEN);
    int ct_len = pt_len + pad_len;

    /* Allocate padded plaintext */
    unsigned char *padded = (unsigned char *)malloc(ct_len);
    if (!padded) return 0;
    memcpy(padded, plaintext, pt_len);
    memset(padded + pt_len, pad_len, pad_len);

    /* Generate random IV */
    unsigned char iv[SMRT_CRYPTO_IV_LEN];
    esp_fill_random(iv, SMRT_CRYPTO_IV_LEN);

    /* Encrypt */
    unsigned char *ciphertext = (unsigned char *)malloc(ct_len);
    if (!ciphertext) { free(padded); return 0; }

    unsigned char iv_copy[SMRT_CRYPTO_IV_LEN];
    memcpy(iv_copy, iv, SMRT_CRYPTO_IV_LEN);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, smrt_crypto_key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, ct_len, iv_copy, padded, ciphertext);
    mbedtls_aes_free(&aes);
    free(padded);

    /* Combine IV + ciphertext */
    int combined_len = SMRT_CRYPTO_IV_LEN + ct_len;
    unsigned char *combined = (unsigned char *)malloc(combined_len);
    if (!combined) { free(ciphertext); return 0; }
    memcpy(combined, iv, SMRT_CRYPTO_IV_LEN);
    memcpy(combined + SMRT_CRYPTO_IV_LEN, ciphertext, ct_len);
    free(ciphertext);

    /* Base64 encode */
    char b64_buf[512];
    int b64_len = smrt_crypto_base64_encode(combined, combined_len, b64_buf, sizeof(b64_buf));
    free(combined);
    if (b64_len == 0) return 0;

    /* Format: "ENC:" + base64 */
    int total_len = SMRT_CRYPTO_ENC_PREFIX_LEN + b64_len;
    if (total_len + 1 > out_max) return 0;

    strcpy(out_buf, SMRT_CRYPTO_ENC_PREFIX);
    strcat(out_buf, b64_buf);
    return 1;
}

/**
 * @brief  Decrypts an "ENC:" prefixed string
 * @param  encrypted  Encrypted string
 * @param  out_buf    Output plaintext buffer
 * @param  out_max    Max output buffer size
 * @return 1 on success, 0 on error
 */
int smrt_crypto_decrypt_string(const char *encrypted, char *out_buf, int out_max) {
    if (!smrt_crypto_initialized || !encrypted || !out_buf) return 0;
    if (!smrt_crypto_is_encrypted(encrypted)) return 0;

    /* Skip "ENC:" prefix */
    const char *b64_data = encrypted + SMRT_CRYPTO_ENC_PREFIX_LEN;

    /* Base64 decode */
    unsigned char decoded[512];
    int decoded_len = smrt_crypto_base64_decode(b64_data, decoded, sizeof(decoded));
    if (decoded_len < SMRT_CRYPTO_IV_LEN + SMRT_CRYPTO_KEY_LEN) return 0;

    /* Extract IV and ciphertext */
    unsigned char iv[SMRT_CRYPTO_IV_LEN];
    memcpy(iv, decoded, SMRT_CRYPTO_IV_LEN);
    int ct_len = decoded_len - SMRT_CRYPTO_IV_LEN;
    unsigned char *ciphertext = decoded + SMRT_CRYPTO_IV_LEN;

    /* Decrypt */
    unsigned char *plaintext = (unsigned char *)malloc(ct_len + 1);
    if (!plaintext) return 0;

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, smrt_crypto_key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ct_len, iv, ciphertext, plaintext);
    mbedtls_aes_free(&aes);

    /* Remove PKCS7 padding */
    int pad_val = plaintext[ct_len - 1];
    if (pad_val < 1 || pad_val > SMRT_CRYPTO_KEY_LEN) {
        free(plaintext);
        return 0;
    }
    int pt_len = ct_len - pad_val;
    if (pt_len < 0 || pt_len >= out_max) {
        free(plaintext);
        return 0;
    }

    memcpy(out_buf, plaintext, pt_len);
    out_buf[pt_len] = '\0';
    free(plaintext);
    return 1;
}

/**
 * @brief  Saves an encrypted string to NVS
 */
void smrt_nvs_set_string_enc(const char *ns, const char *key, const char *value) {
    #ifdef SMRT_CRYPTO
    if (smrt_crypto_initialized) {
        char enc_buf[512];
        if (smrt_crypto_encrypt_string(value, enc_buf, sizeof(enc_buf))) {
            smrt_nvs_set_string(ns, key, enc_buf);
            return;
        }
    }
    #endif
    /* Fallback: store plaintext */
    smrt_nvs_set_string(ns, key, value);
}

/**
 * @brief  Loads and decrypts a string from NVS (transparent migration)
 */
bool smrt_nvs_get_string_enc(const char *ns, const char *key, char *buf, size_t buf_len) {
    char raw[512];
    if (!smrt_nvs_get_string(ns, key, raw, sizeof(raw))) {
        return false;
    }

    #ifdef SMRT_CRYPTO
    if (smrt_crypto_initialized && smrt_crypto_is_encrypted(raw)) {
        /* Decrypt */
        if (smrt_crypto_decrypt_string(raw, buf, buf_len)) {
            return true;
        }
        /* Decryption failed — return raw as fallback */
        strncpy(buf, raw, buf_len - 1);
        buf[buf_len - 1] = '\0';
        return true;
    }

    /* Plaintext detected — migrate to encrypted on next write opportunity */
    if (smrt_crypto_initialized && !smrt_crypto_is_encrypted(raw)) {
        strncpy(buf, raw, buf_len - 1);
        buf[buf_len - 1] = '\0';
        /* Re-save encrypted (transparent migration) */
        smrt_nvs_set_string_enc(ns, key, raw);
        return true;
    }
    #endif

    /* No crypto: return raw */
    strncpy(buf, raw, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return true;
}

#endif // UNIT_TEST
