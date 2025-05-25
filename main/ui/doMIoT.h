#include <stdint.h>
#include <stdbool.h>

const char** miot_get_ui_config_keys(void);

#if CONFIG_SWITCH86_XMIOT_ENABLE

int miot_login(const char* username, const char* password);

int miot_relogin(void);

int miot_query_speaker_did(void);

int miot_send_cmd(const char* cmd);

#else /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

int miot_load_config_semicolons(char* lines, int max_len);
int miot_save_config_semicolons(const char* lines, int len);

int miot_get_token_expires_ts(char expires_ts[32]);
int miot_set_speaker_did(const char* speaker_did);
int miot_refresh_access_token(void);
int miot_gen_auth_url(char* output, size_t max_out_len);
int miot_gen_local_url(int port, char* output, size_t max_out_len);
int miot_get_access_token_start(void);
int miot_get_access_token_stop(void);
int miot_get_access_token_with_uri(const char* uri, size_t uri_len);

int miot_api_post(const char* url_path, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len);
void miot_free(void* ptr);

int miot_action_speaker_cmd(const char* value);
int miot_set_props_lights(int offset, int count, int value);
int miot_set_props_light_bt(int offset, int brightness, int temperature);

void miot_mips_sub_set_light_status_cb(void (*set_light_status_cb)(int32_t index, bool on));
int miot_mips_sub_start(void);
int miot_mips_sub_stop(void);

#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

