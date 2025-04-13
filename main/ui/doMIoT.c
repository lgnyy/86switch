#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "yos_nvs.h"
#include "yos_uri.h"
#if CONFIG_SWITCH86_XMIOT_ENABLE
#include "xmiot_account.h"
#include "xmiot_service.h"
#else
#include "yos_wifi.h"
#include "yos_httpd.h"
#include "miot_cloud.h"
#endif
#include "doMIoT.h"


#if CONFIG_SWITCH86_XMIOT_ENABLE
typedef struct _miot_login_arg_t{
    const char* username;
    const char* password;
}miot_login_arg_t;

const char* ui_config_keys[] = { "username", "speakerDid", NULL };

static int miot_login_save_config(void* ctx, yos_nvs_write_cb_t write_cb, void* arg){
    miot_login_arg_t* login_arg = (miot_login_arg_t*)ctx;
    return xmiot_account_login_auth(NULL, login_arg->username, login_arg->password, write_cb, arg);
}


const char** miot_get_ui_config_keys(void)
{
    return ui_config_keys;
}

int miot_login(const char* username, const char* password)
{
    if (password[0] == '\0') {
        return miot_relogin();
    }
    miot_login_arg_t login_arg = {.username = username, .password = password};
    return yos_nvs_save(YOS_NVS_XMIOT_INFO_NAMESPACE, miot_login_save_config, &login_arg);
}

int miot_relogin(void)
{
    char info[128];
    int ret = yos_nvs_load(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_account_relogin_load_config, info);
    if (ret == 0) {
        ret = yos_nvs_save(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_account_relogin_auth, info);
    }
    return ret;
}

int miot_query_speaker_did(void)
{
    void* ctx = xmiot_service_context_create();
    int ret = yos_nvs_load(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_service_load_config, ctx);
    if (ret == 0) {
        ret = yos_nvs_save(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_service_get_speaker_did, ctx);
    }
    xmiot_service_context_destory(ctx);
    return ret;
}

int miot_send_cmd(const char* cmd)
{
    void* ctx = xmiot_service_context_create();
    if (ctx == NULL) {
        return XMIOT_SERVICE_ERR_NO_MEM;
    }

    int ret = yos_nvs_load(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_service_load_config, ctx);
    if (ret == 0) {
        ret = xmiot_service_send_speaker_cmd(ctx, cmd);
    }

    xmiot_service_context_destory(ctx);
    return ret;
}

#else /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

const char* ui_config_keys[] = { "expires_ts", "device_speaker", NULL };

static const char* _redirect_url = "http://homeassistant.local:8123""/api/webhook/10394765721507444209";
static const uint16_t _redirect_port = 8123;
#define _redirect_uri (_redirect_url + 31)

static int _load_config_access_token(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "access_token", (char*)ctx_, 256);
}
static int _load_config_refresh_token(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "refresh_token", (char*)ctx_, 256);
}
static int _save_config_access_token(void* ctx_, yos_nvs_write_cb_t write_cb, void* arg) {
    return miot_cloud_get_access_token_w(_redirect_url, (char*)ctx_, write_cb, arg);
}
static int _save_config_refresh_token(void* ctx_, yos_nvs_write_cb_t write_cb, void* arg) {
    return miot_cloud_refresh_access_token_w(_redirect_url, (char*)ctx_, write_cb, arg);
}

const char** miot_get_ui_config_keys(void)
{
    return ui_config_keys;
}

int miot_load_config_semicolons(char* lines, int max_len)
{
    yos_nvs_item_t item = {"device_speaker", lines, max_len};
    int ret = yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, &item, 1);
    if (ret == 0) {
        strncat(lines, ";", max_len);
        item.key = "device_lights";
        item.value = lines + strlen(lines);
        item.vsize = max_len - (int)strlen(lines);
        ret = yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, &item, 1);
    }
    return ret;
}

int miot_save_config_semicolons(const char* lines, int len)
{
    char* tmp = (char*)malloc(len + 1);
    if (tmp == NULL) {
        return -1;
    }
    memcpy(tmp, lines, len);
    tmp[len] = '\0';
    
    char* pos = strchr(tmp, ';');
    if (pos != NULL) {
        *pos++ = '\0';
    }

    yos_nvs_item_t items[2] = { {"device_speaker", tmp, 0}, {"device_lights", pos, 0}};
    int ret = yos_nvs_save_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, (pos != NULL)?2:1);
    free(tmp);
    return ret;
}

int miot_get_token_expires_ts(char expires_ts[32])
{
    yos_nvs_item_t item = { "expires_ts", expires_ts, 32 };
    return yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, &item, 1);
}

int miot_set_speaker_did(const char* speaker_did)
{
    if (strchr(speaker_did, ',') == NULL) {
        char device_speaker[256];
        snprintf(device_speaker, sizeof(device_speaker), "%s,5,5", speaker_did);
        yos_nvs_item_t item = { "device_speaker", device_speaker, 0 };
        return yos_nvs_save_ex(YOS_NVS_MIOT_INFO_NAMESPACE, &item, 1);
    }
    else{
        yos_nvs_item_t item = { "device_speaker", (char*)speaker_did, 0 };
        return yos_nvs_save_ex(YOS_NVS_MIOT_INFO_NAMESPACE, &item, 1);
    }
}

int miot_refresh_access_token(void) 
{
    char refresh_token[256] = { 0 };
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_refresh_token, refresh_token);
    if (ret == 0) {   
        ret = yos_nvs_save(YOS_NVS_MIOT_INFO_NAMESPACE, _save_config_refresh_token, refresh_token);
    }
    return ret;
}

int miot_gen_auth_url(char* output, size_t max_out_len)
{
    int i;
    char state[20];
    srand((unsigned int)time(NULL));
    for (i = 0; i < 16; i++) {
        state[i] = 0x30 + (rand() % 10); // getrandom
    }
    state[16] = '\0';

    int ret = miot_cloud_gen_auth_url(_redirect_url, state, NULL, 0, output, max_out_len);
    if (ret == 0) {
        strncat(output, "&source_ip=", max_out_len);
        yos_wifi_station_get_ip4(output + strlen(output));
    }
    return ret;
}

int miot_gen_local_url(int port, char* output, size_t max_out_len)
{
    char ip4[20] = { 0 };
    yos_wifi_station_get_ip4(ip4);
    snprintf(output, max_out_len, "http://%s:%d/api/?redirect_uri=", ip4, port);
    yos_uri_encode(_redirect_url, output + strlen(output));
    strncat(output, "&source_ip=", max_out_len);
    strncat(output, ip4, max_out_len);
    return 0;
}

static int32_t _httpd_uri_get_code(const char* uri, uint32_t uri_len, char code[256]) {
    uint32_t i, offset = 0;
    for (i = 0; i < uri_len; i++) {
        if (((uri[i] == '?') || (uri[i] == '&')) && (memcmp(uri + i + 1, "code=", 5) == 0)) {
            offset = i + 6;
            break;
        }
    }
    if (offset > 0) {
        for (i = offset + 1; (i < uri_len) && (uri[i] != '&'); i++) {
        }

        memcpy(code, uri + offset, i - offset);
        code[i - offset] = '\0';
        return 0;
    }
    return -1;
}

static int32_t _httpd_uri_handler(void* req) {
    uint32_t uri_len = 0;
    const char* uri = yos_httpd_req_get_uri(req, &uri_len);
    //printf("uri=%.*s\n", uri_len, uri);

    yos_httpd_resp_send(req, "ok", 2);

    miot_get_access_token_with_uri(uri, uri_len);
    return 0;
}

static yos_httpd_handle_t _server = NULL;
int miot_get_access_token_start(void) 
{
    if (_server == NULL) {
        _server = yos_httpd_create(_redirect_port);
        yos_register_uri_handler(_server, _redirect_uri, _httpd_uri_handler, NULL);
    }
    return (_server != NULL) ? 0 : -1;
}
int miot_get_access_token_stop(void) {
    if (_server != NULL) {
        yos_httpd_destory(_server);
        _server = NULL;
    }
    return 0;
}

int miot_get_access_token_with_uri(const char* uri, size_t uri_len)
{
    char code[256];
    int ret = _httpd_uri_get_code(uri, (uint32_t)uri_len, code);
    if (ret == 0) {
        ret = yos_nvs_save(YOS_NVS_MIOT_INFO_NAMESPACE, _save_config_access_token, code);
    }
    return ret;
}

int miot_api_post(const char* url_path, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len)
{
    char access_token[256];
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_access_token, access_token);
    if (ret == 0) {
        ret = miot_cloud_api_post(access_token, url_path, data, data_len, resp, resp_len);
    }
    return ret;
}

void miot_free(void* ptr)
{
    miot_cloud_free(ptr);
}

static int _string_split(char* str, int ch, char** list, int count) {
    int offset;
    for (offset = 0; offset < count; offset++) {
        list[offset] = "";
    }
    if (str == NULL) {
        return 0;
    }

    offset = 0;
    list[offset++] = str;
    for (; *str; str++) {
        if (*str == ch) {
            *str = '\0';
            if (offset < count) {
                list[offset++] = str + 1;
            }
        }
    }
    return offset;
}

int miot_action_speaker_cmd(const char* cmd)
{
    char access_token[256], device_speaker[256];
    yos_nvs_item_t items[2] = { {"access_token",access_token,sizeof(access_token)}, {"device_speaker",device_speaker,sizeof(device_speaker)}};
    int ret = yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 2);
    if (ret == 0) {
        char value[256], *list[3];
        _string_split(device_speaker, ',', list, 3);
        snprintf(value, sizeof(value), "\"%s\",1", cmd);
        ret = miot_cloud_action(access_token, list[0], atoi(list[1]), atoi(list[2]), value, NULL);
    }
    return ret;
}

int miot_set_props_lights(int offset, int count, int value) {
    if ((offset + count) > 4) {
        return -1;
    }
    char access_token[256], device_lights[1024];
    yos_nvs_item_t items[2] = { {"access_token",access_token,sizeof(access_token)}, {"device_lights",device_lights,sizeof(device_lights)} };
    int ret = yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 2);
    if (ret == 0) {
        char *light_list[4], *val_list[3];
        _string_split(device_lights, ';', light_list, 4);

        miot_cloud_param_did_t params[4];
        for (int i = 0; i < count; i++) {
            _string_split(light_list[offset + i], ',', val_list, 3);
            params[i].did = val_list[0];
            params[i].siid = atoi(val_list[1]);
            params[i].piid = atoi(val_list[2]);
            params[i].value = value? "true" : "false";
        }

        ret = miot_cloud_set_props(access_token, params, count, NULL);
    }
    return ret;
}


// brightness + temperature
int miot_set_props_light_bt(int offset, int brightness, int temperature) {
    if (offset >= 4) {
        return -1;
    }    
    char access_token[256], device_lights[1024];
    yos_nvs_item_t items[2] = { {"access_token",access_token,sizeof(access_token)}, {"device_lights",device_lights,sizeof(device_lights)} };
    int ret = yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 2);
    if (ret == 0) {
        char * light_list[4], * val_list[7];
        _string_split(device_lights, ';', light_list, 4);
        _string_split(light_list[offset], ',', val_list, 7);

        char p[32], c[32];
        const char* values[] = { "true", p, c, NULL };
        snprintf(p, sizeof(p), "%d", brightness);
        snprintf(c, sizeof(c), "%d", 1700 + 48 * temperature);

        miot_cloud_param_did_t params[3];
        for (int i = 0; i < 3; i++) {
            params[i].did = val_list[0];
            params[i].siid = atoi(val_list[1 + i * 2]);
            params[i].piid = atoi(val_list[2 + i * 2]);
            params[i].value = values[i];
        }

        ret = miot_cloud_set_props(access_token, params, 3, NULL);
    }
    return ret;
}
#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

