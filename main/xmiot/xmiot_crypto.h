/*
 * xmiot
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Encode a buffer into base64 format
 *
 * \param input    source buffer
 * \param ilen     amount of data to be encoded
 * \param output   destination buffer
 * \param max_out_len     size of the destination buffer
 *
 * \return         0 if successful
 */
int xmiot_crypto_base64_encode(const uint8_t* input, size_t ilen, char* output, size_t max_out_len);

/**
 * \brief          Decode a base64-formatted buffer
 *
 * \param input    source buffer(string)
 * \param output   destination buffer (can be NULL for checking size)
 * \param max_out_len     size of the destination buffer
 * \param olen     number of bytes written
 *
 * \return         0 if successful
 */
int xmiot_crypto_base64_decode(const char* input, uint8_t* output, size_t max_out_len, size_t* olen);

/**
 * @brief random
 *
 * @param random      a destination memory
 * @param random_len  the length of the random
 * @return int  \c 0 on success.

 */
int xmiot_crypto_rand(uint8_t* random, size_t random_len);

/**
 * @brief calculates the message-digest of a buffer
 *
 * \param md_name  the message-digest algorithm name(sha1,sha256)
 * \param input    The buffer holding the data.
 * \param ilen     The length of the input data.
 * \param output   The generic message-digest checksum result.
 * @return int  \c 0 on success.
 *
 * @note mbedtls_md.
 */
int xmiot_crypto_md(const char* md_name, const uint8_t* input, size_t ilen, uint8_t* output);

/**
 * @brief calculates the message-digest of two string(base64)
 *
 * \param md_name  the message-digest algorithm name(sha1,sha256)
 * \param input1   The string(base64) holding the data.
 * \param input2   option.
 * \param output   The generic message-digest checksum result, to base64.
 * \param max_out_size     The max length of the output buffer.
 * @return int  \c 0 on success.
 *
 * @note mbedtls_md.
 */
int xmiot_crypto_md_base64(const char* md_name, const char* input1, const char* input2, char* output, size_t max_out_size);

/**
 * \brief          This function calculates the full generic HMAC
 *                 on the input buffer with the provided key.
 *
 * \param md_name  the message-digest algorithm name(sha1,sha256)
 * \param key      The HMAC secret key.
 * \param keylen   The length of the HMAC secret key in Bytes.
 * \param input    The buffer holding the input data.
 * \param ilen     The length of the input data.
 * \param output   The generic HMAC result.
 *
 * \return         \c 0 on success.
 *  * @note mbedtls_md_hmac.
 */
int xmiot_crypto_md_hmac(const char* md_name, const uint8_t* key, size_t keylen, const uint8_t* input, size_t ilen, uint8_t* output);

#ifdef __cplusplus
}
#endif
