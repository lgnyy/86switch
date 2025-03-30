
#if CONFIG_SWITCH86_XMIOT_ENABLE

const char** miot_get_ui_config_keys(void);

int miot_login(const char* username, const char* password);

int miot_relogin(void);

int miot_query_speaker_did(void);

int miot_send_cmd(const char* cmd);

#else /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

int miot_get_token_expires_ts(char expires_ts[32]);
int miot_refresh_access_token(void);

int miot_set_prop(const char* did, int siid, int piid, const char* value);
int miot_set_props_siid(const char* did, const int siids[4], int piid, const char* value);
int miot_set_props_piid(const char* did, int siid, const int piids[4], const char* values[4]);
int miot_action(const char* did, int siid, int aiid, const char* value);

#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

