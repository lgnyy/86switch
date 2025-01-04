
typedef int (*weather_display_cb_t)(void* arg, int index, const char* value);

const char** weather_get_config_keys(void);
int weather_query_first(const char* pos_key, weather_display_cb_t display_cb, void* arg);
int weather_query(weather_display_cb_t display_cb, void* arg);

