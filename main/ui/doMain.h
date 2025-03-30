
#include "ui.h"

void ui_main(void);


void wifi_scan_task(void *pvParameters);

void wifi_connect_task(void* pvParameters);

#if CONFIG_SWITCH86_XMIOT_ENABLE
void miot_login_task(void* pvParameters);

void miot_query_task(void* pvParameters);
#endif

void weather_query_task(void* pvParameters);
