#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yos_nvs.h"
#if CONFIG_SWITCH86_XMIOT_ENABLE
#include "xmiot_account.h"
#include "xmiot_service.h"
#else
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

static int _load_config_access_token(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "access_token", (char*)ctx_, 256);
}
static int _load_config_refresh_token(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "refresh_token", (char*)ctx_, 256);
}
static int _load_config_expires_ts(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    return read_cb(arg, "expires_ts", (char*)ctx_, 32);
}

static int _save_config(void* ctx_, yos_nvs_write_cb_t write_cb, void* arg) {
    const char* redirect_url = "http://homeassistant.local:8123""/api/webhook/10394765721507444209";
    return miot_cloud_refresh_access_token_w(redirect_url, (char*)ctx_, write_cb, arg);
}

int miot_get_token_expires_ts(char expires_ts[32])
{
    return yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_expires_ts, expires_ts);
}

int miot_refresh_access_token(void) 
{
    char refresh_token[256] = { 0 };
    int ret = yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, _load_config_refresh_token, refresh_token);
    if (ret == 0) {   
        ret = yos_nvs_save(YOS_NVS_MIOT_INFO_NAMESPACE, _save_config, refresh_token);
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

