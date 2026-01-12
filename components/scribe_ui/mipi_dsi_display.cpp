#include "mipi_dsi_display.h"
#include "display_driver.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cstring>
#include <algorithm>

static const char* TAG = "SCRIBE_MIPI_DSI";

// Display state
static struct {
    DisplayConfig config;
    lv_display_t* lvgl_display;
    uint8_t* draw_buffer;
    size_t buffer_size;
    bool initialized;
    Orientation current_orientation;

    // SPI device handle (if using SPI)
    spi_device_handle_t spi_handle;

    // Flush semaphore
    SemaphoreHandle_t flush_sem;
} state = {
    .config = {},
    .lvgl_display = nullptr,
    .draw_buffer = nullptr,
    .buffer_size = 0,
    .initialized = false,
    .current_orientation = Orientation::PORTRAIT,
    .spi_handle = nullptr,
    .flush_sem = nullptr
};

// Panel-specific commands
namespace PanelCommands {

// ST7789 commands
constexpr uint8_t ST7789_SWRESET = 0x01;
constexpr uint8_t ST7789_SLPOUT = 0x11;
constexpr uint8_t ST7789_NORON = 0x13;
constexpr uint8_t ST7789_INVOFF = 0x20;
constexpr uint8_t ST7789_INVON = 0x21;
constexpr uint8_t ST7789_DISPOFF = 0x28;
constexpr uint8_t ST7789_DISPON = 0x29;
constexpr uint8_t ST7789_CASET = 0x2A;
constexpr uint8_t ST7789_RASET = 0x2B;
constexpr uint8_t ST7789_RAMWR = 0x2C;
constexpr uint8_t ST7789_MADCTL = 0x36;
constexpr uint8_t ST7789_COLMOD = 0x3A;

// ILI9341 commands
constexpr uint8_t ILI9341_SWRESET = 0x01;
constexpr uint8_t ILI9341_SLPOUT = 0x11;
constexpr uint8_t ILI9341_DISPOFF = 0x28;
constexpr uint8_t ILI9341_DISPON = 0x29;
constexpr uint8_t ILI9341_CASET = 0x2A;
constexpr uint8_t ILI9341_RASET = 0x2B;
constexpr uint8_t ILI9341_RAMWR = 0x2C;
constexpr uint8_t ILI9341_MADCTL = 0x36;
constexpr uint8_t ILI9341_COLMOD = 0x3A;

} // namespace PanelCommands

// Low-level SPI write
static esp_err_t spiWrite(const uint8_t* data, size_t len, bool is_data) {
    if (!state.spi_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    // Set DC pin
    gpio_set_level(state.config.dc_pin, is_data ? 1 : 0);

    spi_transaction_t t = {
        .flags = 0,
        .length = len * 8,  // Length in bits
        .tx_buffer = data,
        .rx_buffer = nullptr
    };

    return spi_device_polling_transmit(state.spi_handle, &t);
}

// Write command byte
static esp_err_t writeCommand(uint8_t cmd) {
    return spiWrite(&cmd, 1, false);
}

// Write data bytes
static esp_err_t writeData(const uint8_t* data, size_t len) {
    return spiWrite(data, len, true);
}

// Write single data byte
static esp_err_t writeData8(uint8_t data) {
    return writeData(&data, 1);
}

// Write 16-bit data
static esp_err_t writeData16(uint16_t data) {
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    return writeData(buf, 2);
}

// Initialize ST7789 panel
static esp_err_t initST7789() {
    ESP_LOGI(TAG, "Initializing ST7789 panel");

    // Software reset
    writeCommand(PanelCommands::ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    writeCommand(PanelCommands::ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Color mode: RGB565
    writeCommand(PanelCommands::ST7789_COLMOD);
    writeData8(0x55);  // 16-bit color
    vTaskDelay(pdMS_TO_TICKS(10));

    // Memory Access Control (orientation)
    writeCommand(PanelCommands::ST7789_MADCTL);
    uint8_t madctl = 0x00;
    switch (state.current_orientation) {
        case Orientation::PORTRAIT:
            madctl = 0x00;
            break;
        case Orientation::LANDSCAPE:
            madctl = 0x60;
            break;
        case Orientation::PORTRAIT_INVERTED:
            madctl = 0xC0;
            break;
        case Orientation::LANDSCAPE_INVERTED:
            madctl = 0xA0;
            break;
    }
    writeData8(madctl);

    // Normal display mode
    writeCommand(PanelCommands::ST7789_NORON);

    // Turn on display
    writeCommand(PanelCommands::ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "ST7789 initialization complete");
    return ESP_OK;
}

// Initialize ILI9341 panel
static esp_err_t initILI9341() {
    ESP_LOGI(TAG, "Initializing ILI9341 panel");

    // Software reset
    writeCommand(PanelCommands::ILI9341_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    writeCommand(PanelCommands::ILI9341_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Color mode: RGB565
    writeCommand(PanelCommands::ILI9341_COLMOD);
    writeData8(0x55);  // 16-bit color
    vTaskDelay(pdMS_TO_TICKS(10));

    // Memory Access Control
    writeCommand(PanelCommands::ILI9341_MADCTL);
    uint8_t madctl = 0x08;  // BGR order
    switch (state.current_orientation) {
        case Orientation::PORTRAIT:
            madctl |= 0x00;
            break;
        case Orientation::LANDSCAPE:
            madctl |= 0x60;
            break;
        case Orientation::PORTRAIT_INVERTED:
            madctl |= 0xC0;
            break;
        case Orientation::LANDSCAPE_INVERTED:
            madctl |= 0xA0;
            break;
    }
    writeData8(madctl);

    // Turn on display
    writeCommand(PanelCommands::ILI9341_DISPON);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "ILI9341 initialization complete");
    return ESP_OK;
}

// Set drawing window
static esp_err_t setWindow(int x1, int y1, int x2, int y2) {
    uint8_t buf[4];

    // Column address set
    writeCommand(PanelCommands::ST7789_CASET);
    buf[0] = x1 >> 8;
    buf[1] = x1 & 0xFF;
    buf[2] = x2 >> 8;
    buf[3] = x2 & 0xFF;
    writeData(buf, 4);

    // Row address set
    writeCommand(PanelCommands::ST7789_RASET);
    buf[0] = y1 >> 8;
    buf[1] = y1 & 0xFF;
    buf[2] = y2 >> 8;
    buf[3] = y2 & 0xFF;
    writeData(buf, 4);

    // Memory write
    writeCommand(PanelCommands::ST7789_RAMWR);

    return ESP_OK;
}

// LVGL flush callback
static void lvglFlushCallback(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    // Set drawing window
    setWindow(x1, y1, x2, y2);

    // Write pixels
    const uint16_t* pixels = (const uint16_t*)px_map;
    size_t pixel_count = w * h;

    // Swap bytes if needed
    if (state.config.rgb565_byte_swap) {
        for (size_t i = 0; i < pixel_count; i++) {
            uint16_t color = pixels[i];
            uint8_t buf[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
            writeData(buf, 2);
        }
    } else {
        writeData((const uint8_t*)pixels, pixel_count * 2);
    }

    // Notify LVGL that flush is complete
    lv_display_flush_ready(display);
}

// Initialize SPI interface
static esp_err_t initSPI() {
    ESP_LOGI(TAG, "Initializing SPI interface at %d MHz", state.config.spi_freq_mhz);

    // Configure GPIO pins
    gpio_set_direction(state.config.dc_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(state.config.reset_pin, GPIO_MODE_OUTPUT);
    if (state.config.cs_pin != GPIO_NUM_NC) {
        gpio_set_direction(state.config.cs_pin, GPIO_MODE_OUTPUT);
    }

    // Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_NUM_23,      // Default MOSI (adjust for hardware)
        .miso_io_num = GPIO_NUM_NC,       // Not used for display only
        .sclk_io_num = GPIO_NUM_18,       // Default SCLK (adjust for hardware)
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = state.buffer_size,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Add device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = state.config.spi_freq_mhz * 1000 * 1000,
        .mode = 0,  // SPI mode 0
        .spics_io_num = state.config.cs_pin,
        .queue_size = 1,
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &state.spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    ESP_LOGI(TAG, "SPI interface initialized");
    return ESP_OK;
}

esp_err_t MIPIDSI::init(const DisplayConfig& config) {
    if (state.initialized) {
        ESP_LOGW(TAG, "Display already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing MIPI-DSI display: %dx%d, panel=%d",
             config.width, config.height, (int)config.panel);

    state.config = config;
    state.flush_sem = xSemaphoreCreateBinary();

    // Initialize SPI (or MIPI-DSI in the future)
    if (config.use_spi) {
        esp_err_t ret = initSPI();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI initialization failed: %s", esp_err_to_name(ret));
            vSemaphoreDelete(state.flush_sem);
            return ret;
        }
    }

    // Hardware reset
    if (config.reset_pin != GPIO_NUM_NC) {
        gpio_set_level(config.reset_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(config.reset_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    // Initialize panel
    switch (config.panel) {
        case PanelType::ST7789:
            initST7789();
            break;
        case PanelType::ILI9341:
            initILI9341();
            break;
        default:
            ESP_LOGE(TAG, "Unsupported panel type");
            return ESP_ERR_NOT_SUPPORTED;
    }

    // Calculate buffer size
    int hor_res = config.width;
    int ver_res = config.height;
    state.buffer_size = hor_res * 40 * 2;  // 40 lines buffer

    // Allocate draw buffer
    state.draw_buffer = (uint8_t*)heap_caps_malloc(state.buffer_size, MALLOC_CAP_DMA);
    if (!state.draw_buffer) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer");
        return ESP_ERR_NO_MEM;
    }

    // Create LVGL display
    state.lvgl_display = lv_display_create(hor_res, ver_res);
    if (!state.lvgl_display) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        heap_caps_free(state.draw_buffer);
        return ESP_ERR_NO_MEM;
    }

    // Set flush callback
    lv_display_set_flush_cb(state.lvgl_display, lvglFlushCallback);

    // Set draw buffer
    lv_display_set_draw_buffers(state.lvgl_display, state.draw_buffer, nullptr,
                                state.buffer_size, 0,
                                LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set color format
    lv_display_set_color_format(state.lvgl_display, LV_COLOR_FORMAT_RGB565);

    // Clear display
    clear(0x0000);

    state.initialized = true;
    ESP_LOGI(TAG, "Display initialization complete");
    return ESP_OK;
}

esp_err_t MIPIDSI::setOrientation(Orientation orientation) {
    state.current_orientation = orientation;

    // Re-initialize panel with new orientation
    switch (state.config.panel) {
        case PanelType::ST7789:
            writeCommand(PanelCommands::ST7789_MADCTL);
            break;
        case PanelType::ILI9341:
            writeCommand(PanelCommands::ILI9341_MADCTL);
            break;
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }

    uint8_t madctl = 0x00;
    switch (orientation) {
        case Orientation::PORTRAIT:
            madctl = 0x00;
            break;
        case Orientation::LANDSCAPE:
            madctl = 0x60;
            break;
        case Orientation::PORTRAIT_INVERTED:
            madctl = 0xC0;
            break;
        case Orientation::LANDSCAPE_INVERTED:
            madctl = 0xA0;
            break;
    }
    writeData8(madctl);

    return ESP_OK;
}

void MIPIDSI::writePixels(const uint16_t* pixels, size_t len) {
    writeData((const uint8_t*)pixels, len * 2);
}

void MIPIDSI::fillRect(int x1, int y1, int x2, int y2, uint16_t color) {
    setWindow(x1, y1, x2, y2);

    size_t pixel_count = (x2 - x1 + 1) * (y2 - y1 + 1);
    for (size_t i = 0; i < pixel_count; i++) {
        writeData16(color);
    }
}

void MIPIDSI::clear(uint16_t color) {
    fillRect(0, 0, state.config.width - 1, state.config.height - 1, color);
}

esp_err_t MIPIDSI::reset() {
    if (state.config.reset_pin != GPIO_NUM_NC) {
        gpio_set_level(state.config.reset_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(state.config.reset_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(150));

        // Re-initialize panel
        switch (state.config.panel) {
            case PanelType::ST7789:
                return initST7789();
            case PanelType::ILI9341:
                return initILI9341();
            default:
                break;
        }
    }
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t MIPIDSI::sleep(bool enter) {
    switch (state.config.panel) {
        case PanelType::ST7789:
            if (enter) {
                writeCommand(PanelCommands::ST7789_DISPOFF);
            } else {
                writeCommand(PanelCommands::ST7789_SLPOUT);
                vTaskDelay(pdMS_TO_TICKS(10));
                writeCommand(PanelCommands::ST7789_DISPON);
            }
            return ESP_OK;
        case PanelType::ILI9341:
            if (enter) {
                writeCommand(PanelCommands::ILI9341_DISPOFF);
            } else {
                writeCommand(PanelCommands::ILI9341_SLPOUT);
                vTaskDelay(pdMS_TO_TICKS(10));
                writeCommand(PanelCommands::ILI9341_DISPON);
            }
            return ESP_OK;
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t MIPIDSI::setBacklight(int percent) {
    if (state.config.backlight_pin != GPIO_NUM_NC) {
        // For PWM control, use LEDC peripheral
        // For simple on/off, use GPIO
        gpio_set_level(state.config.backlight_pin, percent > 0 ? 1 : 0);
        return ESP_OK;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

lv_display_t* MIPIDSI::getLVGLDisplay() {
    return state.lvgl_display;
}

const DisplayConfig& MIPIDSI::getConfig() {
    return state.config;
}

void MIPIDSI::deinit() {
    if (!state.initialized) {
        return;
    }

    if (state.lvgl_display) {
        lv_display_delete(state.lvgl_display);
        state.lvgl_display = nullptr;
    }

    if (state.draw_buffer) {
        heap_caps_free(state.draw_buffer);
        state.draw_buffer = nullptr;
    }

    if (state.spi_handle) {
        spi_bus_remove_device(state.spi_handle);
        spi_bus_free(SPI2_HOST);
        state.spi_handle = nullptr;
    }

    if (state.flush_sem) {
        vSemaphoreDelete(state.flush_sem);
        state.flush_sem = nullptr;
    }

    state.initialized = false;
    ESP_LOGI(TAG, "Display deinitialized");
}
