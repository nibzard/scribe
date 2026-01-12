#include "app_queue.h"

QueueHandle_t g_ui_queue = nullptr;
QueueHandle_t g_storage_queue = nullptr;
QueueHandle_t g_input_queue = nullptr;

void init_queues(void) {
    g_ui_queue = xQueueCreate(UI_QUEUE_SIZE, sizeof(Event));
    g_storage_queue = xQueueCreate(STORAGE_QUEUE_SIZE, sizeof(Event));
    g_input_queue = xQueueCreate(INPUT_QUEUE_SIZE, sizeof(Event));
}
