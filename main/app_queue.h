#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "app_event.h"

// Queue sizes
#define UI_QUEUE_SIZE 32
#define STORAGE_QUEUE_SIZE 8
#define INPUT_QUEUE_SIZE 64

// Global queues
extern QueueHandle_t g_ui_queue;
extern QueueHandle_t g_storage_queue;
extern QueueHandle_t g_input_queue;

// Initialize all queues
void init_queues(void);
