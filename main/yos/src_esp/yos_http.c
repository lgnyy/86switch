
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#include "../include/yos_http.h"

static const char *TAG = "YOS_HTTP";


typedef struct yos_http_context
{
	esp_http_client_handle_t client;
    char *output_buffer;  // Buffer to store response of http request from event handler
    int output_len;       // Stores number of bytes read
	int output_alloc_len;
	int32_t (*on_header_set_cookie_cb)(void* arg, const char* value);
	void* on_header_set_cookie_arg;	
}yos_http_context_t;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
 			if (evt->user_data != NULL){
				yos_http_context_t* ctx = (yos_http_context_t*)(evt->user_data);
				if ((ctx->on_header_set_cookie_cb != NULL) && (strcmp(evt->header_key, "Set-Cookie") == 0)) {
					ctx->on_header_set_cookie_cb(ctx->on_header_set_cookie_arg, evt->header_value);
				}
			}
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			if (evt->user_data == NULL){
				break;
			}
			yos_http_context_t* ctx = (yos_http_context_t*)(evt->user_data);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
	            int content_len = esp_http_client_get_content_length(evt->client);
                if ((ctx->output_buffer == NULL) || (content_len > ctx->output_alloc_len)) {
 			        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, content_len=%d", content_len); 
                    // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                    char* new_buffer = (char*)realloc(ctx->output_buffer, content_len);
					if (new_buffer == NULL){
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_ERR_NO_MEM;
					}
					ctx->output_buffer = new_buffer;
					ctx->output_alloc_len = content_len;
                    ctx->output_len = 0;
				}  
                
				if ((ctx->output_len +  evt->data_len) > ctx->output_alloc_len){
                      ESP_LOGE(TAG, "Over output buffer");
                    return ESP_FAIL;
				}
            }else{
	            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, is chunked, len=%d", evt->data_len); 
				if ((ctx->output_len +  evt->data_len) > ctx->output_alloc_len){
					int new_len = (ctx->output_alloc_len + evt->data_len + 0x800) & 0x7FFFF800;
					char* new_buffer = (char*)realloc(ctx->output_buffer, new_len);
					if (new_buffer == NULL){
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_ERR_NO_MEM;
					}
					ctx->output_buffer = new_buffer;
					ctx->output_alloc_len = new_len;
				}
			}
			memcpy(ctx->output_buffer + ctx->output_len, evt->data, evt->data_len);
			ctx->output_len += evt->data_len;
			ctx->output_buffer[ctx->output_len] = '\0';

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}



yos_http_handle_t yos_http_create(const char* url, const char* ca_cert)
{
	yos_http_context_t* ctx = (yos_http_context_t*)calloc(1, sizeof(yos_http_context_t));
	if (ctx != NULL){
	   esp_http_client_config_t config = {
			.url = url,
			.event_handler = _http_event_handler,
			.user_data = ctx,
			//.disable_auto_redirect = true,
			.buffer_size_tx = 2048,
			.cert_pem = ca_cert,
			.cert_len = (ca_cert && (ca_cert[0]==0x30) && (ca_cert[1]==0x82))? (4+((uint8_t)(ca_cert[2]) << 8 | (uint8_t)(ca_cert[3]))) : 0,
		};
		ctx->client = esp_http_client_init(&config);
	}
	return ctx;
}

int32_t yos_http_destory(yos_http_handle_t ctx)
{
	if (ctx != NULL){
		if (ctx->output_buffer != NULL) {
			free(ctx->output_buffer);
		}
		
		esp_http_client_cleanup(ctx->client);
		free(ctx);
	}
    return 0;
}


int32_t yos_http_set_timeout(yos_http_handle_t ctx, uint32_t conn_timeout, uint32_t rsp_timeout)
{
	if ((ctx == NULL)){
		return ESP_ERR_INVALID_ARG;
	}
	return esp_http_client_set_timeout_ms(ctx->client, (int)rsp_timeout);
}

int32_t yos_http_set_url(yos_http_handle_t ctx, const char* url)
{
	if ((ctx == NULL) || (url == NULL)){
		return ESP_ERR_INVALID_ARG;
	}
	return esp_http_client_set_url(ctx->client, url);
}

int32_t yos_http_set_headers(yos_http_handle_t ctx, const char* headers)
{
	if ((ctx == NULL) || (ctx->client == NULL) || (headers == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	char* tmp = (char*)malloc(strlen(headers) + 1);
	if (tmp == NULL) {
		return ESP_ERR_NO_MEM;
	}
	strcpy(tmp, headers);

	char* p1 = tmp;
	char* p2 = p1;
	while (p2 != NULL) {
		p2 = strstr(p1, "\r\n");
		if (p2 != NULL) {
			*p2 = '\0';
		}
		char* p3 = strchr(p1, ':');
		if (p3 != NULL) {
			*p3++ = '\0';

			esp_err_t err = esp_http_client_set_header(ctx->client, p1, p3);
			if (err != ESP_OK){
				ESP_LOGE(TAG, "esp_http_client_set_header(%s): 0x%x", p1, err);
			}
		}
		if (p2 != NULL) {
			p1 = p2 + 2;
		}
	}
	return 0;
}

int32_t yos_http_set_header(yos_http_handle_t ctx, const char* key, const char* value)
{
	if ((ctx == NULL) || (ctx->client == NULL) || (key == NULL) || (value == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	return esp_http_client_set_header(ctx->client, key, value);
}

int32_t yos_http_get_header(yos_http_handle_t ctx, const char* key, char** value)
{
	if ((ctx == NULL) || (ctx->client == NULL) || (key == NULL) || (value == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	return esp_http_client_get_header(ctx->client, key, value);
}

int32_t yos_http_set_on_header_set_cookie_cb(yos_http_handle_t ctx, int32_t(*cb)(void* arg, const char* value), void* arg) 
{
	if ((ctx == NULL)){
		return ESP_ERR_INVALID_ARG;
	}
	ctx->on_header_set_cookie_cb = cb;
	ctx->on_header_set_cookie_arg = arg;
	return 0;
}


int32_t yos_http_request(yos_http_handle_t ctx, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len)
{
	if ((ctx == NULL) || (resp == NULL)){
		return ESP_ERR_INVALID_ARG;
	}

	ctx->output_len = 0;

	if (data_len > 0){
		esp_http_client_set_method(ctx->client, HTTP_METHOD_POST);
		esp_http_client_set_post_field(ctx->client, (const char *)data, (int)data_len);
	}
	else{
		esp_http_client_set_method(ctx->client, HTTP_METHOD_GET);
	}
 
	esp_err_t err = esp_http_client_perform(ctx->client);
	ESP_LOGI(TAG, "esp_http_client_perform:%d", err);
	if (err == ESP_OK){
		if (resp != NULL){
			*resp = (uint8_t*)(ctx->output_buffer);
		}
		if (resp_len != NULL){
			*resp_len = ctx->output_len;
		}
	}
	return err;
}

void* yos_http_static_malloc(uint32_t size)
{
	return malloc(size);
}

void yos_http_static_free(void* ptr)
{
	free(ptr);
}


int32_t yos_http_static_request(const char* url, const char* ca_cert, const char* headers, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len)
{
	yos_http_handle_t handle = yos_http_create(url, ca_cert);
	if (handle == NULL){
		return ESP_ERR_NO_MEM;
	}

	if (headers != NULL){
		yos_http_set_headers(handle, headers);
	}

	int32_t rv = yos_http_request(handle, data, data_len, resp, resp_len);
	if ((rv == 0) && (resp != NULL)){
		handle->output_buffer = NULL; // External release
		handle->output_len = 0;
		handle->output_alloc_len = 0;
	}

	yos_http_destory(handle);
	return rv;
}
