#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yos_nvs.h"
#include "xmiot_account.h"
#include "xmiot_service.h"
#include "doMIoT.h"


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

