
const char** miot_get_ui_config_keys(void);

int miot_login(const char* username, const char* password);

int miot_relogin(void);

int miot_query_speaker_did(void);

int miot_send_cmd(const char* cmd);

