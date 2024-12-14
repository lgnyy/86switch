#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "yos_uri.h"
#include "yos_http.h"
#include "xmiot_cacert.h"
#include "xmiot_crypto.h"
#include "xmiot_service.h"

#define LOGD printf

static const char* _miio_url = "https://api.io.mi.com";

static char cfg_username[20];
static char cfg_deviceId[24];
static char cfg_ssecurity[32];
static char cfg_serviceToken[256];


int xmiot_service_config(int (*read_cb)(void* arg, const char* key, char* value, size_t vsize), void* arg) {
	read_cb(arg, "username", cfg_username, sizeof(cfg_username));
	read_cb(arg, "deviceId", cfg_deviceId, sizeof(cfg_deviceId));
	read_cb(arg, "ssecurity", cfg_ssecurity, sizeof(cfg_ssecurity));
	return read_cb(arg, "serviceToken", cfg_serviceToken, sizeof(cfg_serviceToken));
}

static int miio_request_base(const char* uri, const char* data, char** resp) {
	int ret = XMIOT_SERVICE_ERR_INVALID_ARG;
	char* url = (char*)malloc(16 + strlen(_miio_url) + strlen(uri));
	char* headers = (char*)malloc(256 + strlen(cfg_serviceToken));

	if ((url == NULL) || (headers == NULL)) {
		goto end;
	}

	strcpy(url, _miio_url);
	strcat(url, uri);
	sprintf(headers, "x-xiaomi-protocal-flag-cli: PROTOCAL-HTTP2\r\nCookie: PassportDeviceId=%s; serviceToken=\"%s\"; userId=%s\r\nContent-Type: application/x-www-form-urlencoded", cfg_deviceId, cfg_serviceToken, cfg_username);

	ret = yos_http_static_request(url, xmiot_cacert_usertrust(), headers, (const uint8_t*)data, strlen(data), (uint8_t**)resp, NULL);

end:
	free(url);
	free(headers);
	return ret;
}


#define miio_sign_nonce(ssecurity, nonce, output, max_out_size) xmiot_crypto_md_base64("SHA256", ssecurity, nonce, output, max_out_size)

static int miio_sign_data_ue(const cJSON* obj, cJSON_bool is_uri_encode, char** output) {
	char* _nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, "_nonce"));
	char* data = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, "data"));
	char* signature = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, "signature"));
	
	char* msg = (char*)malloc(256 + strlen(data));
	if (msg == NULL) {
		return XMIOT_SERVICE_ERR_NO_MEM;
	}
	strcpy(msg, "_nonce=");
	yos_uri_encode(_nonce, msg + strlen(msg));
	strcat(msg, "&data=");
	yos_uri_encode(data, msg + strlen(msg));
	strcat(msg, "&signature=");
	yos_uri_encode(signature, msg + strlen(msg));

	*output = msg;
	return 0;
}

static int miio_sign_data(const char* uri, const cJSON* data_, cJSON* resp_, const char* ssecurity) {
	int ret = XMIOT_SERVICE_ERR_NO_MEM;
	char* msg = NULL;
	char* data = cJSON_PrintUnformatted(data_);
	if (data == NULL) {
		goto end;
	}
	LOGD("data=%s\n", data);

	uint8_t nonce_bytes[12];
	char nonce[20];
	ret = xmiot_crypto_rand(nonce_bytes, 12);
	if (ret != 0) {
		goto end;
	}
	ret = xmiot_crypto_base64_encode(nonce_bytes, 12, nonce, sizeof(nonce));
	if (ret != 0) {
		goto end;
	}
	LOGD("nonce=%s\n", nonce);

	char snonce[48];
	ret = miio_sign_nonce(ssecurity, nonce, snonce, sizeof(snonce));
	if (ret != 0) {
		goto end;
	}
	LOGD("snonce=%s\n", snonce);

	msg = (char*)malloc(128 + strlen(data));
	if (msg == NULL) {
		ret = XMIOT_SERVICE_ERR_NO_MEM;
		goto end;
	}
	strcpy(msg, uri);
	strcat(msg, "&");
	strcat(msg, snonce);
	strcat(msg, "&");
	strcat(msg, nonce);
	strcat(msg, "&");
	strcat(msg, "data=");
	strcat(msg, data);
	LOGD("msg=%s\n", msg);

	uint8_t snonce_bytes[32], sign_md[32];
	size_t snonce_len = 0;
	ret = xmiot_crypto_base64_decode(snonce, snonce_bytes, sizeof(snonce_bytes), &snonce_len);
	if (ret != 0) {
		goto end;
	}
	ret = xmiot_crypto_md_hmac("SHA256", snonce_bytes, snonce_len, (uint8_t*)msg, strlen(msg), sign_md);
	if (ret != 0) {
		goto end;
	}	
	char sign[48];
	ret = xmiot_crypto_base64_encode(sign_md, 32, sign, sizeof(sign));
	if (ret != 0) {
		goto end;
	}
	LOGD("sign=%s\n", sign);

	ret = XMIOT_SERVICE_ERR_NO_MEM;
	if (!cJSON_AddItemToObject(resp_, "_nonce", cJSON_CreateString(nonce))) {
		goto end;
	}
	if (!cJSON_AddItemToObject(resp_, "data", cJSON_CreateString(data))) {
		goto end;
	}
	if (!cJSON_AddItemToObject(resp_, "signature", cJSON_CreateString(sign))) {
		goto end;
	}
	ret = 0;

end:
	free(msg);
	cJSON_free(data);
	return ret;
}

static int miio_request(const char* uri, const cJSON* data, 
	int (*cb)(void* arg, cJSON* resp), void* arg) {
	char* qdata = NULL;
	char* qresp = NULL;
	cJSON* tmp = cJSON_CreateObject();
	cJSON* resp = NULL;
	
	// skip /app
	int ret = miio_sign_data(uri+4, data, tmp, cfg_ssecurity);
	if (ret != 0) {
		goto end;
	}

	ret = miio_sign_data_ue(tmp, 1, &qdata);
	if (ret != 0) {
		goto end;
	}
	LOGD("qdata=%s\n", qdata);

	ret = miio_request_base(uri, qdata, &qresp);
	if (ret != 0) {
		goto end;
	}
	LOGD("qresp=%s\n", qresp);

	resp = cJSON_Parse(qresp);
	if (resp == NULL) {
		ret = XMIOT_SERVICE_ERR_NO_MEM;
		goto end;
	}

	cJSON* code = cJSON_GetObjectItemCaseSensitive(resp, "code");
	if (!cJSON_IsNumber(code)){
		ret = XMIOT_SERVICE_ERR_NO_CODE;
		goto end;
	}

	ret = (int)cJSON_GetNumberValue(code);
	if (ret != 0) {
		goto end;
	}

	if (cb != NULL) {
		ret = cb(arg, resp);
	}

end:
	cJSON_Delete(tmp);
	cJSON_Delete(resp);
	yos_http_static_free(qresp);
	free(qdata);
	return 0;
}


static int miio_find_speaker_did(void* arg, cJSON* resp) {
	cJSON* result = cJSON_GetObjectItemCaseSensitive(resp, "result");
	cJSON* list = cJSON_GetObjectItemCaseSensitive(result, "list");
	int n = cJSON_GetArraySize(list);
	for (int i = 0; i < n; i++) {
		cJSON* item = cJSON_GetArrayItem(list, i);
		char* model = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(item, "model"));
		if (memcmp(model, "xiaomi.wifispeaker.", 19) == 0) {
			char* did = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(item, "did"));
			if ((did != NULL) && (strlen(did) < 10)) {
				strcpy((char*)arg, did);
				return 0;
			}
		}
	}
	return -1;
}
int xmiot_service_get_speaker_did(char did[10]) {
	int ret = XMIOT_SERVICE_ERR_NO_MEM;
	cJSON* data = cJSON_CreateObject();
	if (data == NULL) {
		goto end;
	}

	// {getVirtualModel:false, getHuamiDevices:0}
	if (!cJSON_AddItemToObject(data, "getVirtualModel", cJSON_CreateFalse())) {
		goto end;
	}
	if (!cJSON_AddItemToObject(data, "getHuamiDevices", cJSON_CreateNumber(0))) {
		goto end;
	}

	ret = miio_request("/app/home/device_list", data, miio_find_speaker_did, did);

end:
	cJSON_Delete(data);
	return ret;
}

int xmiot_service_send_speaker_cmd(const char* did, const char* cmd) {
	int ret = XMIOT_SERVICE_ERR_NO_MEM;
	cJSON* data = cJSON_CreateObject();
	if (data == NULL) {
		goto end;
	}
	// {params: {did:"XX", siid:5, aiid:5, in:[cmd,1]}}
	cJSON* params = cJSON_CreateObject();
	cJSON_AddItemToObject(data, "params", params);

	if (!cJSON_AddItemToObject(params, "did", cJSON_CreateString(did))) {
		goto end;
	}
	if (!cJSON_AddItemToObject(params, "siid", cJSON_CreateNumber(5))) {
		goto end;
	}
	if (!cJSON_AddItemToObject(params, "aiid", cJSON_CreateNumber(5))) {
		goto end;
	}

	cJSON* in = cJSON_CreateArray();
	cJSON_AddItemToObject(params, "in", in);
	if (!cJSON_AddItemToArray(in, cJSON_CreateString(cmd))) {
		goto end;
	}
	if (!cJSON_AddItemToArray(in, cJSON_CreateNumber(1))) {
		goto end;
	}

	ret = miio_request("/app/miotspec/action", data, NULL, NULL);

end:
	cJSON_Delete(data);
	return ret;
}
