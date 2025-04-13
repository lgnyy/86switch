
typedef int (*weather_display_cb_t)(void* arg, int index, const char* value);

const char** weather_get_config_keys(void);

int weather_load_config_semicolon(char* pos_key, int max_len);
int weather_save_config_semicolon(const char* pos_key, int len);

int weather_query_first(const char* pos, const char* key, weather_display_cb_t display_cb, void* arg);
int weather_query(weather_display_cb_t display_cb, void* arg);

