#include "keyboard_host_control.h"
#include "keyboard_host.h"

bool keyboard_host_is_running(void) {
    return KeyboardHost::getInstance().isRunning();
}

esp_err_t keyboard_host_start(void) {
    return KeyboardHost::getInstance().start();
}

esp_err_t keyboard_host_stop(void) {
    return KeyboardHost::getInstance().stop();
}
