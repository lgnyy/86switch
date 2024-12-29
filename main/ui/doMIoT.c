#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs_cfg.h"
#include "xmiot_account.h"
#include "xmiot_service.h"
#include "doMIoT.h"


typedef struct _miot_login_arg_t{
    const char* username;
    const char* password;
}miot_login_arg_t;

static int miot_login_save_config(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg){
    miot_login_arg_t* login_arg = (miot_login_arg_t*)ctx;
    return xmiot_account_login_auth(NULL, login_arg->username, login_arg->password, write_cb, arg);
}


int miot_login(const char* username, const char* password)
{
    miot_login_arg_t login_arg = {.username = username, .password = password};
    int ret = nvs_cfg_save(NVS_CFG_XMIOT_INFO_NAMESPACE, miot_login_save_config, &login_arg);
    printf("miot_login miot_login_save_config:%d\n", ret);
    if (ret == 0){
        void* ctx = xmiot_service_context_create();
        ret = nvs_cfg_load(NVS_CFG_XMIOT_INFO_NAMESPACE, xmiot_service_load_config, ctx);
        if (ret == 0){
            ret = nvs_cfg_save(NVS_CFG_XMIOT_INFO_NAMESPACE, xmiot_service_get_speaker_did, ctx);
        }
        xmiot_service_context_destory(ctx);
        printf("miot_login miot_get_speaker_did_save_config:%d\n", ret);
    }
    return ret;
}

int miot_relogin(void)
{
    return -1;
}

int miot_send_cmd(const char* cmd)
{
    void* ctx = xmiot_service_context_create();
    if (ctx == NULL) {
        return XMIOT_SERVICE_ERR_NO_MEM;
    }

    int ret = nvs_cfg_load(NVS_CFG_XMIOT_INFO_NAMESPACE, xmiot_service_load_config, ctx);
    if (ret == 0) {
        ret = xmiot_service_send_speaker_cmd(ctx, cmd);
    }

    xmiot_service_context_destory(ctx);
    return ret;
}

