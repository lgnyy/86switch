
typedef int (*weather_display_cb_t)(void* arg, int index, const char* value);

int weather_query(weather_display_cb_t display_cb, void* arg);

