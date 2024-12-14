/*
 * xmiot
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define XMIOT_SERVICE_ERR_INVALID_ARG -1
#define XMIOT_SERVICE_ERR_NO_MEM      -2
#define XMIOT_SERVICE_ERR_NO_CODE     -4

/**
 * @brief config
 *
 * @param read_cb
 * @param arg
 * @return int  \c 0 on success.

 */
int xmiot_service_config(int (*read_cb)(void* arg, const char* key, char* value, size_t vsize), void* arg);

/**
 * @brief Get did of wifispeaker
 *
 * @param did      a destination did
 * @return int  \c 0 on success.

 */
int xmiot_service_get_speaker_did(char did[10]);

/**
 * @brief Send wifispeaker commands
 *
 * @param did      a speaker did
 * @param cmd      a speaker command
 * @return int  \c 0 on success.

 */
int xmiot_service_send_speaker_cmd(const char* did, const char* cmd);

#ifdef __cplusplus
}
#endif
