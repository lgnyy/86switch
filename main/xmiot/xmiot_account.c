#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "yos_uri.h"
#include "yos_http.h"
#include "xmiot_cacert.h"
#include "xmiot_crypto.h"
#include "xmiot_account.h"

#define LOGD printf

static const char* _miio_url = "https://account.xiaomi.com";

static char cfg_deviceId[20];

int xmiot_account_config(int (*read_cb)(void* arg, const char* key, char* value, size_t vsize), void* arg) {
	int ret = read_cb(arg, "deviceId", cfg_deviceId, sizeof(cfg_deviceId));
	if (ret != 0) {
		uint8_t r[12];
		xmiot_crypto_rand(r, 12);
		ret = xmiot_crypto_base64_encode(r, 12, cfg_deviceId, 20);
	}
	return ret;
}

static int miio_password_hash(const char* password, char* hash) {
	uint8_t md[16];
	xmiot_crypto_md("MD5", (const uint8_t*)password, strlen(password), md);

	int i;
	for (i = 0; i < 16; i++) {
		sprintf(hash + (i << 1), "%02X", md[i]);
	}
	return 0;
}

static int miio_request_base(const char* uri, const char* data, char** resp) {
	int ret = XMIOT_ACCOUNT_ERR_INVALID_ARG;
	char* url = (char*)malloc(16 + strlen(_miio_url) + strlen(uri));
	char* headers = (char*)malloc(256);

	if ((url == NULL) || (headers == NULL)) {
		goto end;
	}

	strcpy(url, _miio_url);
	strcat(url, uri);
	sprintf(headers, "Cookie: sdkVersion=\"3.9\"; deviceId=\"%s\"\r\nContent-Type: application/x-www-form-urlencoded", cfg_deviceId);

	size_t data_len = strlen(data);
	if (data_len > 0) {
		sprintf(headers + strlen(headers), "\r\nContent-Length: %d", (int)data_len);
	}

	LOGD("url=%s, headers=%s\n", url, headers);
	LOGD("data=%s\n", data);
	ret = yos_http_static_request(url, xmiot_cacert_go_daddy(), headers, (const uint8_t*)data, data_len, (uint8_t**)resp, NULL);
	if (ret != 0) {
		goto end;
	}
	if ((*resp) == NULL){
		ret = XMIOT_ACCOUNT_ERR_NO_MEM;
		goto end;
	}
	LOGD("resp=%s\n", *resp);

	ret = (memcmp(*resp, "&&&START&&&", 11) == 0) ? 0 : XMIOT_ACCOUNT_ERR_NO_START;
end:
	free(url);
	free(headers);
	return ret;
}

static int miio_check_code(const cJSON* resp) {
	if (resp == NULL) {
		return XMIOT_ACCOUNT_ERR_INVALID_ARG;
	}
	cJSON* code = cJSON_GetObjectItemCaseSensitive(resp, "code");
	if (!cJSON_IsNumber(code)) {
		return XMIOT_ACCOUNT_ERR_NO_CODE;
	}
	return (int)cJSON_GetNumberValue(code);
}

static int miio_login(const char* username, const char* password_hash, cJSON** json_result)
{
	int ret = XMIOT_ACCOUNT_ERR_INVALID_ARG;
	char* data2 = NULL;
	char* resp = NULL;
	cJSON* resp_json = NULL;

	ret = miio_request_base("/pass/serviceLogin?sid=xiaomiio&_json=true", "", &resp);
	if (ret != 0) {
		goto end;
	}

	resp_json = cJSON_Parse(resp + 11);
	if (miio_check_code(resp_json) != 0) {
		char* qs = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "qs"));
		char* sid = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "sid"));
		char* _sign = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "_sign"));
		char* callback = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "callback"));

		data2 = (char*)malloc(256 + strlen(_sign));
		if (!qs || !sid || !_sign || !callback || !data2) {
			ret = XMIOT_ACCOUNT_ERR_NO_MEM;
			goto end;
		}

		strcpy(data2, "_json=true");
		strcat(data2, "&qs=");
		strcat(data2, qs);
		strcat(data2, "&sid=");
		strcat(data2, sid);
		strcat(data2, "&_sign=");
		strcat(data2, _sign);
		strcat(data2, "&callback=");
		strcat(data2, callback);
		strcat(data2, "&user=");
		strcat(data2, username);
		strcat(data2, "&hash=");
		strcat(data2, password_hash);
	
		free(resp);
		resp = NULL;
		ret = miio_request_base("/pass/serviceLoginAuth2", data2, &resp);
		if (ret != 0) {
			goto end;
		}

		cJSON_Delete(resp_json);
		resp_json = cJSON_Parse(resp + 11);
		ret = miio_check_code(resp_json);
		if (ret != 0) {
			goto end;
		}
	}

	if (json_result != NULL) {
		cJSON* nonce_json = cJSON_GetObjectItemCaseSensitive(resp_json, "nonce");
		if (nonce_json != NULL) {
			// Json standard does not support int64, https://zhuanlan.zhihu.com/p/519361799
			char* pos1 = strstr(resp, "\"nonce\":") + 8;
			char* pos2 = (pos1 && (pos1 > resp))? strchr(pos1, ',') : NULL;
			if (pos2 != NULL) {
				*pos2 = '\0';
				cJSON_ReplaceItemInObject(resp_json, "nonce", cJSON_CreateString(pos1));
			}
		}
		*json_result = resp_json;
		resp_json = NULL;
	}
end:
	free(data2);
	yos_http_static_free(resp);
	cJSON_Delete(resp_json);
	return ret;
}

int xmiot_account_login(const char* username, const char* password, void** json_result)
{
	if (!username || !password) {
		return XMIOT_ACCOUNT_ERR_INVALID_ARG;
	}
	char password_hash[32 + 4];
	miio_password_hash(password, password_hash);

	return miio_login(username, password_hash, (cJSON**)json_result);
}

static int32_t on_header_set_cookie_cb(void* arg, const char* value) {
	if (memcmp(value, "serviceToken=", 13) == 0) {
		const char* pe = strchr(value + 13, ';');
		if (pe == NULL) {
			pe = value + strlen(value);
		}
		size_t vl = (size_t)(pe - value - 13);
		char* vv = (char*)malloc(vl + 1);
		if (vv != NULL) {
			memcpy(vv, value + 13, vl);
			vv[vl] = '\0';
		}
		*((char**)arg) = vv;
	}
	return 0;
}
static int miio_security_token_service(const char* location, const char* nonce, const char* ssecurity, char** service_token){
	int ret = XMIOT_ACCOUNT_ERR_INVALID_ARG;
	char *nsec = (char*)malloc(32 + strlen(nonce) + strlen(ssecurity));
	char* url = (char*)malloc(strlen(location)+128);
	uint8_t sign_md[20];
	yos_http_handle_t http_client = NULL;
	char* resp = NULL;

	if (!nsec || !url) {
		goto end;
	}

	sprintf(nsec, "nonce=%s&%s", nonce, ssecurity);
	LOGD("nsec=%s\n", nsec);
	xmiot_crypto_md("SHA1", (const uint8_t*)nsec, strlen(nsec), sign_md);
	char* sign = nsec; // tmp
	xmiot_crypto_base64_encode(sign_md, 20, sign, 32);

	strcpy(url, location);
	strcat(url, "&clientSign=");
	yos_uri_encode(sign, url + strlen(url));

	LOGD("url=%s\n", url);
	http_client = yos_http_create(url, xmiot_cacert_usertrust());
	if (http_client == NULL){
		ret = XMIOT_ACCOUNT_ERR_NO_MEM;
		goto end;
	}

	yos_http_set_on_header_set_cookie_cb(http_client, on_header_set_cookie_cb, service_token);

	// auto free resp
	ret = yos_http_request(http_client, NULL, 0, (uint8_t**)&resp, NULL);
	if (ret != 0){
		goto end;
	}
	LOGD("resp=%s\n", resp);

	if ((*service_token) == NULL) {
		ret = XMIOT_ACCOUNT_ERR_NO_TOKEN;
		goto end;
	}

end:
	free(nsec);
	free(url);
	yos_http_destory(http_client);
	return ret;
}

int xmiot_account_login_auth(const char* username, const char* password,
	int (*write_cb)(void* arg, const char* key, const char* value), void* arg)
{
	if (!username || !password || !write_cb) {
		return XMIOT_ACCOUNT_ERR_INVALID_ARG;
	}
	char password_hash[32 + 4];
	miio_password_hash(password, password_hash);

	cJSON* resp_json = NULL;
	char* service_token = NULL;
	int ret = miio_login(username, password_hash, &resp_json);
	if (ret != 0) {
		goto end;
	}
	// passToken
	char* ssecurity = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "ssecurity"));
	char* location = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "location"));
	char* nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(resp_json, "nonce"));
	if (!ssecurity || !location || !nonce) {
		ret = XMIOT_ACCOUNT_ERR_INVALID_ARG;
		goto end;
	}

	ret = miio_security_token_service(location, nonce, ssecurity, &service_token);
	if (ret != 0) {
		goto end;
	}

	write_cb(arg, "username", username);
	write_cb(arg, "password_hash", password_hash);
	write_cb(arg, "deviceId", cfg_deviceId);
	write_cb(arg, "ssecurity", ssecurity);
	ret = write_cb(arg, "serviceToken", service_token);

end:
	cJSON_Delete(resp_json);
	free(service_token);
	return ret;
}
