
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <crtdbg.h>

#if 0
const uint8_t index_html_gz_start[10];
const uint8_t index_html_gz_end[1];

#define INCBIN(name, file) \
    static const uint8_t* name##_data; \
    static size_t name##_size; \
    static void __load_##name() { \
        HANDLE hFile = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); \
        if (hFile == INVALID_HANDLE_VALUE) { \
            fprintf(stderr, "Failed to open %s\n", file); \
            exit(1); \
        } \
        DWORD size = GetFileSize(hFile, NULL); \
        uint8_t* buffer = (uint8_t*)malloc(size); \
        ReadFile(hFile, buffer, size, NULL, NULL); \
        CloseHandle(hFile); \
        name##_data = buffer; \
        name##_size = size; \
    } \
    static void (*__dummy_##name)() = __load_##name;

INCBIN(index_html_gz, "index.html.gz");
#endif

static uint32_t custom_os_tick(void)
{
	return GetTickCount();
}
static void custom_os_delay(uint32_t ms)
{
	Sleep(ms);
}

void ui_main_entry(void* param)
{
	(void)(param);
	/* lvgl初始化 */
	lv_init();

	lv_tick_set_cb(custom_os_tick);
	lv_delay_set_cb(custom_os_delay);

	lv_display_t* disp = lv_windows_create_display(L"86switch", 480, 480+24, 100, true, false);
	lv_windows_acquire_pointer_indev(disp);

	extern void ui_main(void);
	ui_main();

	//*******************************************************************
	uint32_t time_till_next;
	for (;;)
	{
		time_till_next = lv_timer_handler();

		custom_os_delay(time_till_next);
	}
}

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);
	printf("Hello World!\n");

	ui_main_entry(NULL);

	// memory
	_CrtDumpMemoryLeaks();
	return 0;
}
