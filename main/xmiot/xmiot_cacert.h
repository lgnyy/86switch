/*
 * xmiot
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Go Daddy Root Certificate Authority - G2 */
const char* xmiot_cacert_go_daddy(void);

/** USERTrust RSA Certification Authority */
const char* xmiot_cacert_usertrust(void);

#ifdef __cplusplus
}
#endif
