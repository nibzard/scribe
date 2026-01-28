#pragma once

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool keyboard_host_is_running(void);
esp_err_t keyboard_host_start(void);
esp_err_t keyboard_host_stop(void);

#ifdef __cplusplus
}
#endif
