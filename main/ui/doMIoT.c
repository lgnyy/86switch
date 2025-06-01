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
#include "cJSON.h"
#include "yos_wifi.h"
#include "yos_httpd.h"
#include "yos_mqtt.h"
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
        yos_httpd_register_uri_handler(_server, _redirect_uri, _httpd_uri_handler, NULL);
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


static void (*_set_light_status_cb)(int32_t index, bool on);
void miot_mips_sub_set_light_status_cb(void (*set_light_status_cb)(int32_t index, bool on))
{
    _set_light_status_cb = set_light_status_cb;
}

static int32_t _mqtt_event_handler(void* ev) {
    if (yos_mqtt_event_is_connected(ev)) {
        char device_lights[1024];
        yos_nvs_item_t items[1] = { {"device_lights",device_lights,sizeof(device_lights)} };
        if (yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 1) == 0) {
            char topic_bufs[4][256];
            yos_mqtt_topic_t topics[4];
            
            char* light_list[4], * val_list[3]; 
            _string_split(device_lights, ';', light_list, 4);
            for (int i = 0; i < 4; i++) {
                _string_split(light_list[i], ',', val_list, 3);
                sprintf(topic_bufs[i], "device/%s/up/properties_changed/%s/%s", val_list[0], val_list[1], val_list[2]);
                topics[i].filter = topic_bufs[i];
                topics[i].qos = 1;                
            }
            yos_mqtt_subscribe_multiple(yos_mqtt_event_get_instance(ev), topics, 4);
        }
    }
    else if (yos_mqtt_event_is_msg(ev)) {
        int data_len, topic_len;
        char* data = yos_mqtt_event_get_data(ev, &data_len);
        char* topic = yos_mqtt_event_get_topic(ev, &topic_len);
        printf("mqtt msg: %.*s, topic:  %.*s\n", data_len, data, topic_len, topic);
    
        if (_set_light_status_cb != NULL) {
            if (data[data_len]) {
                data[data_len] = '\0';
            }
            cJSON* resp = cJSON_Parse(data);
            if (resp != NULL) {
                cJSON* params = cJSON_GetObjectItemCaseSensitive(resp, "params");
                cJSON* value = cJSON_GetObjectItemCaseSensitive(params, "value");
                char* did = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(params, "did"));
                cJSON* siid = cJSON_GetObjectItemCaseSensitive(params, "siid");
                cJSON* piid = cJSON_GetObjectItemCaseSensitive(params, "piid");
                if ((value != NULL) && (did != NULL) && (siid != NULL) && (piid != NULL)) {
                    char changed_light[256], device_lights[1024];
                    snprintf(changed_light, sizeof(changed_light), "%s,%d,%d", did, siid->valueint, piid->valueint);
                    yos_nvs_item_t items[1] = { {"device_lights",device_lights,sizeof(device_lights)} };
                    if (yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 1) == 0) {
                        char* light_list[4];
                        _string_split(device_lights, ';', light_list, 4);
                        for (int t = 0; t < 4; t++) {
                            if (strcmp(light_list[t], changed_light) == 0) {
                                _set_light_status_cb(1+t, value->valueint);
                                break;
                            }
                        }
                    }
                }
                cJSON_Delete(resp);
            }
        }
    }
    return 0;
}
static yos_mqtt_handle_t s_mqtt;
static const char* _digi_ca_cert = "\x30\x82\x03\x8E\x30\x82\x02\x76\xA0\x03\x02\x01\x02\x02\x10\x03\x3A\xF1\xE6\xA7\x11\xA9\xA0\xBB\x28\x64\xB1\x1D\x09\xFA\xE5\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B\x05\x00\x30\x61\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x15\x30\x13\x06\x03\x55\x04\x0A\x13\x0C\x44\x69\x67\x69\x43\x65\x72\x74\x20\x49\x6E\x63\x31\x19\x30\x17\x06\x03\x55\x04\x0B\x13\x10\x77\x77\x77\x2E\x64\x69\x67\x69\x63\x65\x72\x74\x2E\x63\x6F\x6D\x31\x20\x30\x1E\x06\x03\x55\x04\x03\x13\x17\x44\x69\x67\x69\x43\x65\x72\x74\x20\x47\x6C\x6F\x62\x61\x6C\x20\x52\x6F\x6F\x74\x20\x47\x32\x30\x1E\x17\x0D\x31\x33\x30\x38\x30\x31\x31\x32\x30\x30\x30\x30\x5A\x17\x0D\x33\x38\x30\x31\x31\x35\x31\x32\x30\x30\x30\x30\x5A\x30\x61\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x15\x30\x13\x06\x03\x55\x04\x0A\x13\x0C\x44\x69\x67\x69\x43\x65\x72\x74\x20\x49\x6E\x63\x31\x19\x30\x17\x06\x03\x55\x04\x0B\x13\x10\x77\x77\x77\x2E\x64\x69\x67\x69\x63\x65\x72\x74\x2E\x63\x6F\x6D\x31\x20\x30\x1E\x06\x03\x55\x04\x03\x13\x17\x44\x69\x67\x69\x43\x65\x72\x74\x20\x47\x6C\x6F\x62\x61\x6C\x20\x52\x6F\x6F\x74\x20\x47\x32\x30\x82\x01\x22\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01\x05\x00\x03\x82\x01\x0F\x00\x30\x82\x01\x0A\x02\x82\x01\x01\x00\xBB\x37\xCD\x34\xDC\x7B\x6B\xC9\xB2\x68\x90\xAD\x4A\x75\xFF\x46\xBA\x21\x0A\x08\x8D\xF5\x19\x54\xC9\xFB\x88\xDB\xF3\xAE\xF2\x3A\x89\x91\x3C\x7A\xE6\xAB\x06\x1A\x6B\xCF\xAC\x2D\xE8\x5E\x09\x24\x44\xBA\x62\x9A\x7E\xD6\xA3\xA8\x7E\xE0\x54\x75\x20\x05\xAC\x50\xB7\x9C\x63\x1A\x6C\x30\xDC\xDA\x1F\x19\xB1\xD7\x1E\xDE\xFD\xD7\xE0\xCB\x94\x83\x37\xAE\xEC\x1F\x43\x4E\xDD\x7B\x2C\xD2\xBD\x2E\xA5\x2F\xE4\xA9\xB8\xAD\x3A\xD4\x99\xA4\xB6\x25\xE9\x9B\x6B\x00\x60\x92\x60\xFF\x4F\x21\x49\x18\xF7\x67\x90\xAB\x61\x06\x9C\x8F\xF2\xBA\xE9\xB4\xE9\x92\x32\x6B\xB5\xF3\x57\xE8\x5D\x1B\xCD\x8C\x1D\xAB\x95\x04\x95\x49\xF3\x35\x2D\x96\xE3\x49\x6D\xDD\x77\xE3\xFB\x49\x4B\xB4\xAC\x55\x07\xA9\x8F\x95\xB3\xB4\x23\xBB\x4C\x6D\x45\xF0\xF6\xA9\xB2\x95\x30\xB4\xFD\x4C\x55\x8C\x27\x4A\x57\x14\x7C\x82\x9D\xCD\x73\x92\xD3\x16\x4A\x06\x0C\x8C\x50\xD1\x8F\x1E\x09\xBE\x17\xA1\xE6\x21\xCA\xFD\x83\xE5\x10\xBC\x83\xA5\x0A\xC4\x67\x28\xF6\x73\x14\x14\x3D\x46\x76\xC3\x87\x14\x89\x21\x34\x4D\xAF\x0F\x45\x0C\xA6\x49\xA1\xBA\xBB\x9C\xC5\xB1\x33\x83\x29\x85\x02\x03\x01\x00\x01\xA3\x42\x30\x40\x30\x0F\x06\x03\x55\x1D\x13\x01\x01\xFF\x04\x05\x30\x03\x01\x01\xFF\x30\x0E\x06\x03\x55\x1D\x0F\x01\x01\xFF\x04\x04\x03\x02\x01\x86\x30\x1D\x06\x03\x55\x1D\x0E\x04\x16\x04\x14\x4E\x22\x54\x20\x18\x95\xE6\xE3\x6E\xE6\x0F\xFA\xFA\xB9\x12\xED\x06\x17\x8F\x39\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B\x05\x00\x03\x82\x01\x01\x00\x60\x67\x28\x94\x6F\x0E\x48\x63\xEB\x31\xDD\xEA\x67\x18\xD5\x89\x7D\x3C\xC5\x8B\x4A\x7F\xE9\xBE\xDB\x2B\x17\xDF\xB0\x5F\x73\x77\x2A\x32\x13\x39\x81\x67\x42\x84\x23\xF2\x45\x67\x35\xEC\x88\xBF\xF8\x8F\xB0\x61\x0C\x34\xA4\xAE\x20\x4C\x84\xC6\xDB\xF8\x35\xE1\x76\xD9\xDF\xA6\x42\xBB\xC7\x44\x08\x86\x7F\x36\x74\x24\x5A\xDA\x6C\x0D\x14\x59\x35\xBD\xF2\x49\xDD\xB6\x1F\xC9\xB3\x0D\x47\x2A\x3D\x99\x2F\xBB\x5C\xBB\xB5\xD4\x20\xE1\x99\x5F\x53\x46\x15\xDB\x68\x9B\xF0\xF3\x30\xD5\x3E\x31\xE2\x8D\x84\x9E\xE3\x8A\xDA\xDA\x96\x3E\x35\x13\xA5\x5F\xF0\xF9\x70\x50\x70\x47\x41\x11\x57\x19\x4E\xC0\x8F\xAE\x06\xC4\x95\x13\x17\x2F\x1B\x25\x9F\x75\xF2\xB1\x8E\x99\xA1\x6F\x13\xB1\x41\x71\xFE\x88\x2A\xC8\x4F\x10\x20\x55\xD7\xF3\x14\x45\xE5\xE0\x44\xF4\xEA\x87\x95\x32\x93\x0E\xFE\x53\x46\xFA\x2C\x9D\xFF\x8B\x22\xB9\x4B\xD9\x09\x45\xA4\xDE\xA4\xB8\x9A\x58\xDD\x1B\x7D\x52\x9F\x8E\x59\x43\x88\x81\xA4\x9E\x26\xD5\x6F\xAD\xDD\x0D\xC6\x37\x7D\xED\x03\x92\x1B\xE5\x77\x5F\x76\xEE\x3C\x8D\xC4\x5D\x56\x5B\xA2\xD9\x66\x6E\xB3\x35\x37\xE5\x32\xB6";
// esp_crt_bundle_attach -> enable global cacert

int miot_mips_sub_start(void)
{
    if (s_mqtt != NULL) {
        return -1;
    }
    const char* client_id = "ha.e65ab9b3b56c758786f198972f51f199";
    const char* username = "2882303761520251711"; // OAUTH2_CLIENT_ID

    char access_token[256];
    yos_nvs_item_t items[1] = { {"access_token",access_token,sizeof(access_token)} };
    int ret = yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 1);
    if (ret == 0) {
        s_mqtt = yos_mqtt_connect("mqtts://cn-ha.mqtt.io.mi.com:8883", _digi_ca_cert, client_id, username, access_token);
        yos_mqtt_register_event_handler(s_mqtt, _mqtt_event_handler);
    }
    return 0;
}

int miot_mips_sub_stop(void)
{
    if (s_mqtt != NULL) {
        yos_mqtt_disconnect(s_mqtt);
         s_mqtt = NULL;
    }
    return 0;
}

#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

