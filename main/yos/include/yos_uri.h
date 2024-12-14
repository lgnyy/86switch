/*
 * Utility functions for protocol examples
 *
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Encode an URI
 *
 * @param src        the source string
 * @param dest       a destination memory location
 *
 * @note Please allocate the destination buffer keeping in mind that encoding a
 *       special character will take up 3 bytes (for '%' and two hex digits).
 *       In the worst-case scenario, the destination buffer will have to be 3 times
 *       that of the source string.
 */
void yos_uri_encode(const char* src, char* dest);

/**
 * @brief Decode an URI
 *
 * @param src   the source string
 * @param dest  a destination memory location
 *
 * @note Please allocate the destination buffer keeping in mind that a decoded
 *       special character will take up 2 less bytes than its encoded form.
 *       In the worst-case scenario, the destination buffer will have to be
 *       the same size that of the source string.
 */
void yos_uri_decode(const char* src, char* dest);

#ifdef __cplusplus
}
#endif
