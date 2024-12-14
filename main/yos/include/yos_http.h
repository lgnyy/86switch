/*
 * yos_http.h
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HTTP instance handle */
typedef struct yos_http_context * yos_http_handle_t;


/**
 * @brief Create client instance
 *
 * @param [in] url              server address
 * @param [in] ca_cert          ca cert(optional)
 *
 * @return !NULL success/NULL failure
 */
yos_http_handle_t yos_http_create(const char* url, const char* ca_cert);

/**
 * @brief Destroy the created client instance
 *
 * @param [in] handle         client instance
 *
 * @return 0 success/other failure
 */
int32_t yos_http_destory(yos_http_handle_t handle);

/** Set timeout (connection, response) */
int32_t yos_http_set_timeout(yos_http_handle_t handle, uint32_t conn_timeout, uint32_t rsp_timeout);


/** Set url */
int32_t yos_http_set_url(yos_http_handle_t handle, const char* url);

/** Set universal header settings */
int32_t yos_http_set_headers(yos_http_handle_t handle, const char* headers);

/** Set http request header */
int32_t yos_http_set_header(yos_http_handle_t handle, const char* key, const char* value);

/** Get http request header */
int32_t yos_http_get_header(yos_http_handle_t handle, const char* key, char** value);
/** Set http header event(Set-Cookie) */
int32_t yos_http_set_on_header_set_cookie_cb(yos_http_handle_t ctx, int32_t (*cb)(void* arg, const char* value), void* arg);

/** request(GET/POST), caller does not need to release resp */
int32_t yos_http_request(yos_http_handle_t handle, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len);


/** Allocate memory */
void* yos_http_static_malloc(uint32_t size);

/** free memory */
void yos_http_static_free(void* ptr);


/** request(GET/POST), caller releases resp */
int32_t yos_http_static_request(const char* url, const char* ca_cert, const char* header, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len);

#ifdef __cplusplus
}
#endif
