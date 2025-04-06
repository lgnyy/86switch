#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "yos_nvs.h"
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

const char* ui_config_keys[] = { "expires_ts", "speaker_did", NULL };

static const char* _redirect_url = "http://homeassistant.local:8123""/api/webhook/10394765721507444209";
static const uint16_t _redirect_port = 8123;
#define _redirect_uri (_redirect_url + 31)

static int _load_config_access_token(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "access_token", (char*)ctx_, 256);
}
static int _load_config_refresh_token(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "refresh_token", (char*)ctx_, 256);
}
static int _load_config_expires_ts(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "expires_ts", (char*)ctx_, 32);
}
static int _save_config_speaker_did(void* ctx_, yos_nvs_write_cb_t write_cb, void* arg) {
    return write_cb(arg, "speaker_did", (char*)ctx_);
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

int miot_get_token_expires_ts(char expires_ts[32])
{
    return yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_expires_ts, expires_ts);
}

int miot_set_speaker_did(const char* speaker_did)
{
    return yos_nvs_save(YOS_NVS_MIOT_INFO_NAMESPACE, _save_config_speaker_did, (void*)speaker_did);
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

static int32_t _httpd_uri_handler(void* req) {
    uint32_t uri_len = 0;
    const char* uri = yos_httpd_req_get_uri(req, &uri_len);
    //printf("uri=%.*s\n", uri_len, uri);

    yos_httpd_resp_send(req, "ok", 2);

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

        char* udata = (char*)yos_httpd_req_get_udata(req);
        memcpy(udata, uri + offset, i - offset);
        udata[i - offset] = '\0';

        yos_httpd_set_bits(yos_httpd_req_get_handle(req), 1);
    }
    return 0;
}
static int miot_get_auth_code(char code[256])
{
    yos_httpd_handle_t server = yos_httpd_create(_redirect_port);
    yos_register_uri_handler(server, _redirect_uri, _httpd_uri_handler, code);
    int ret = yos_httpd_wait_bits(server, 1, 60000);
    yos_unregister_uri_handler(server, _redirect_uri);
    yos_httpd_destory(server);
    return ret;
}

int miot_get_access_token(void)
{
    char code[256] = { 0 };
    int ret = miot_get_auth_code(code);
    if (ret == 0) {
        ret = yos_nvs_save(YOS_NVS_MIOT_INFO_NAMESPACE, _save_config_access_token, code);
    }
    return ret;
}

int miot_set_prop(const char* did, int siid, int piid, const char* value)
{
    char access_token[256];
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_access_token, access_token);
    if (ret == 0) {
        ret = miot_cloud_set_prop(access_token, did, siid, piid, value, NULL);
    }
    return ret;
}

int miot_set_props_siid(const char* did, const int siids[4], int piid, const char* value)
{
    int i, count;
    for (count = 0; siids[count]; count++) {
    }
    
    miot_cloud_param_did_t* params = (miot_cloud_param_did_t*)malloc(sizeof(miot_cloud_param_did_t) * count);
    if (params == NULL) {
        return -1;
    }
    
    for (i = 0; i < count; i++) {
        params[i].did = did;
        params[i].siid = siids[i];
        params[i].piid = piid;
        params[i].value = value;
    }

    char access_token[256];
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_access_token, access_token);
    if (ret == 0) {
        ret = miot_cloud_set_props(access_token, params, count, NULL);
    }
    return ret;
}

int miot_set_props_piid(const char* did, int siid, const int piids[4], const char* values[4])
{
    int i, count;
    for (count = 0; piids[count]; count++) {
    }
    miot_cloud_param_did_t* params = (miot_cloud_param_did_t*)malloc(sizeof(miot_cloud_param_did_t) * count);
    if (params == NULL) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        params[i].did = did;
        params[i].siid = siid;
        params[i].piid = piids[i];
        params[i].value = values[i];
    }

    char access_token[256];
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_access_token, access_token);
    if (ret == 0) {
        ret = miot_cloud_set_props(access_token, params, count, NULL);
    }
    return ret;
}

int miot_action(const char* did, int siid, int aiid, const char* value)
{
    char access_token[256];
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_access_token, access_token);
    if (ret == 0) {
        ret = miot_cloud_action(access_token, did, siid, aiid, value, NULL);
    }
    return ret;
}
#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

