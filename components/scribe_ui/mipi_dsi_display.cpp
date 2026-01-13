#include "mipi_dsi_display.h"
#include "../scribe_hw/tab5_io_expander.h"
#include "sdkconfig.h"
#include <driver/ledc.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_ldo_regulator.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using MIPIDSI::DisplayConfig;
using MIPIDSI::Orientation;

static const char* TAG = "SCRIBE_MIPI_DSI";

// Display state
static struct {
    DisplayConfig config;
    lv_display_t* lvgl_display;
    uint8_t* draw_buffer;
    size_t buffer_size;
    bool initialized;
    Orientation current_orientation;
    bool use_spi;

    // SPI device handle (if using SPI)
    spi_device_handle_t spi_handle;
    spi_host_device_t spi_host;

    // DSI handles
    esp_lcd_dsi_bus_handle_t dsi_bus;
    esp_lcd_panel_io_handle_t dsi_io;
    esp_lcd_panel_handle_t dsi_panel;
    esp_ldo_channel_handle_t dsi_ldo;
    void* frame_buffers[2];
    size_t frame_buffer_size;
    int num_frame_buffers;

    // Backlight
    bool backlight_pwm_ready;
} state = {
    .config = {},
    .lvgl_display = nullptr,
    .draw_buffer = nullptr,
    .buffer_size = 0,
    .initialized = false,
    .current_orientation = Orientation::PORTRAIT,
    .use_spi = true,
    .spi_handle = nullptr,
    .spi_host = SPI2_HOST,
    .dsi_bus = nullptr,
    .dsi_io = nullptr,
    .dsi_panel = nullptr,
    .dsi_ldo = nullptr,
    .frame_buffers = {nullptr, nullptr},
    .frame_buffer_size = 0,
    .num_frame_buffers = 0,
    .backlight_pwm_ready = false,
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

// DSI generic commands
constexpr uint8_t DSI_SLPIN = 0x10;
constexpr uint8_t DSI_SLPOUT = 0x11;
constexpr uint8_t DSI_DISPOFF = 0x28;
constexpr uint8_t DSI_DISPON = 0x29;
constexpr uint8_t DSI_MADCTL = 0x36;
constexpr uint8_t DSI_COLMOD = 0x3A;

} // namespace PanelCommands

namespace {
constexpr int kBacklightTimer = LEDC_TIMER_0;
constexpr int kBacklightChannel = LEDC_CHANNEL_0;
constexpr ledc_mode_t kBacklightSpeedMode = LEDC_LOW_SPEED_MODE;
constexpr uint32_t kBacklightFreqHz = 5000;
constexpr ledc_timer_bit_t kBacklightDutyRes = LEDC_TIMER_13_BIT;
constexpr int kBacklightMaxDuty = (1 << kBacklightDutyRes) - 1;

constexpr int kDsiBusId = 0;
constexpr int kDsiLaneMbps = 960;
constexpr int kDsiNumLanes = 2;
constexpr int kDsiDpiClockMhz = 80;
constexpr int kDsiHsyncBackPorch = 40;
constexpr int kDsiHsyncPulseWidth = 2;
constexpr int kDsiHsyncFrontPorch = 40;
constexpr int kDsiVsyncBackPorch = 8;
constexpr int kDsiVsyncPulseWidth = 2;
constexpr int kDsiVsyncFrontPorch = 220;

static const uint8_t st7123_init_list0[] = {
    4,  0x60,  0x71, 0x23, 0xa2,
    4,  0x60,  0x71, 0x23, 0xa3,
    4,  0x60,  0x71, 0x23, 0xa4,
    2,  0xA4,  0x31,

    7,  0xD7,  0x10, 0x0A, 0x10, 0x2A, 0x80,  0x80,
    8,  0x90,  0x71, 0x23, 0x5A, 0x20, 0x24,  0x09, 0x09,

    40, 0xA3,  0x80, 0x01, 0x88, 0x30, 0x05,  0x00, 0x00, 0x00, 0x00, 0x00,
               0x46, 0x00, 0x00, 0x1E, 0x5C,  0x1E, 0x80, 0x00, 0x4F, 0x05,
               0x00, 0x00, 0x00, 0x00, 0x00,  0x46, 0x00, 0x00, 0x1E, 0x5C,
               0x1E, 0x80, 0x00, 0x6F, 0x58,  0x00, 0x00, 0x00, 0xFF,

    56, 0xA6,  0x03, 0x00, 0x24, 0x55, 0x36,  0x00, 0x39, 0x00, 0x6E, 0x6E,
               0x91, 0xFF, 0x00, 0x24, 0x55,  0x38, 0x00, 0x37, 0x00, 0x6E,
               0x6E, 0x91, 0xFF, 0x00, 0x24,  0x11, 0x00, 0x00, 0x00, 0x00,
               0x6E, 0x6E, 0x91, 0xFF, 0x00,  0xEC, 0x11, 0x00, 0x03, 0x00,
               0x03, 0x6E, 0x6E, 0xFF, 0xFF,  0x00, 0x08, 0x80, 0x08, 0x80,
               0x06, 0x00, 0x00, 0x00, 0x00,

    61, 0xA7,  0x19, 0x19, 0x80, 0x64, 0x40,  0x07, 0x16, 0x40, 0x00, 0x44,
               0x03, 0x6E, 0x6E, 0x91, 0xFF,  0x08, 0x80, 0x64, 0x40, 0x25,
               0x34, 0x40, 0x00, 0x02, 0x01,  0x6E, 0x6E, 0x91, 0xFF, 0x08,
               0x80, 0x64, 0x40, 0x00, 0x00,  0x40, 0x00, 0x00, 0x00, 0x6E,
               0x6E, 0x91, 0xFF, 0x08, 0x80,  0x64, 0x40, 0x00, 0x00, 0x00,
               0x00, 0x20, 0x00, 0x6E, 0x6E,  0x84, 0xFF, 0x08, 0x80, 0x44,

    45, 0xAC,  0x03, 0x19, 0x19, 0x18, 0x18,  0x06, 0x13, 0x13, 0x11, 0x11,
               0x08, 0x08, 0x0A, 0x0A, 0x1C,  0x1C, 0x07, 0x07, 0x00, 0x00,
               0x02, 0x02, 0x01, 0x19, 0x19,  0x18, 0x18, 0x06, 0x12, 0x12,
               0x10, 0x10, 0x09, 0x09, 0x0B,  0x0B, 0x1C, 0x1C, 0x07, 0x07,
               0x03, 0x03, 0x01, 0x01,

    26, 0xAD,  0xF0, 0x00, 0x46, 0x00, 0x03,  0x50, 0x50, 0xFF, 0xFF, 0xF0,
               0x40, 0x06, 0x01, 0x07, 0x42,  0x42, 0xFF, 0xFF, 0x01, 0x00,
               0x00, 0xFF, 0xFF, 0xFF, 0xFF,

     8, 0xAE,  0xFE, 0x3F, 0x3F, 0xFE, 0x3F, 0x3F, 0x00,
    18, 0xB2,  0x15, 0x19, 0x05, 0x23, 0x49,  0xAF, 0x03, 0x2E, 0x5C, 0xD2,
               0xFF, 0x10, 0x20, 0xFD, 0x20,  0xC0, 0x00,

    15, 0xE8,  0x20, 0x6F, 0x04, 0x97, 0x97,  0x3E, 0x04, 0xDC, 0xDC, 0x3E,
               0x06, 0xFA, 0x26, 0x3E,

     3, 0x75,  0x03, 0x04,
    37, 0xE7,  0x3B, 0x00, 0x00, 0x7C, 0xA1,  0x8C, 0x20, 0x1A, 0xF0, 0xB1,
               0x50, 0x00, 0x50, 0xB1, 0x50,  0xB1, 0x50, 0xD8, 0x00, 0x55,
               0x00, 0xB1, 0x00, 0x45, 0xC9,  0x6A, 0xFF, 0x5A, 0xD8, 0x18,
               0x88, 0x15, 0xB1, 0x01, 0x01,  0x77,

     9, 0xEA,  0x13, 0x00, 0x04, 0x00, 0x00,  0x00, 0x00, 0x2C,
     8, 0xB0,  0x22, 0x43, 0x11, 0x61, 0x25,  0x43, 0x43,
     5, 0xB7,  0x00, 0x00, 0x73, 0x73,
     3, 0xBF,  0xA6, 0xAA,
    11, 0xA9,  0x00, 0x00, 0x73, 0xFF, 0x00,  0x00, 0x03, 0x00, 0x00, 0x03,
    38, 0xC8,  0x00, 0x00, 0x10, 0x1F, 0x36,  0x00, 0x5D, 0x04, 0x9D, 0x05,
               0x10, 0xF2, 0x06, 0x60, 0x03,  0x11, 0xAD, 0x00, 0xEF, 0x01,
               0x22, 0x2E, 0x0E, 0x74, 0x08,  0x32, 0xDC, 0x09, 0x33, 0x0F,
               0xF3, 0x77, 0x0D, 0xB0, 0xDC,  0x03, 0xFF,
    38, 0xC9,  0x00, 0x00, 0x10, 0x1F, 0x36,  0x00, 0x5D, 0x04, 0x9D, 0x05,
               0x10, 0xF2, 0x06, 0x60, 0x03,  0x11, 0xAD, 0x00, 0xEF, 0x01,
               0x22, 0x2E, 0x0E, 0x74, 0x08,  0x32, 0xDC, 0x09, 0x33, 0x0F,
               0xF3, 0x77, 0x0D, 0xB0, 0xDC,  0x03, 0xFF,
     0,
};

static const uint8_t st7123_init_list1[] = {
     1,  PanelCommands::DSI_DISPON,
     2,  0x35,  0x00,
     1,  PanelCommands::DSI_SLPOUT,
     0,
};

static esp_err_t initBacklightPwm() {
    if (state.backlight_pwm_ready || state.config.backlight_pin == GPIO_NUM_NC) {
        return ESP_OK;
    }

    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = kBacklightSpeedMode;
    timer_cfg.timer_num = static_cast<ledc_timer_t>(kBacklightTimer);
    timer_cfg.duty_resolution = kBacklightDutyRes;
    timer_cfg.freq_hz = kBacklightFreqHz;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        return ret;
    }

    ledc_channel_config_t channel_cfg = {};
    channel_cfg.speed_mode = kBacklightSpeedMode;
    channel_cfg.channel = static_cast<ledc_channel_t>(kBacklightChannel);
    channel_cfg.timer_sel = static_cast<ledc_timer_t>(kBacklightTimer);
    channel_cfg.intr_type = LEDC_INTR_DISABLE;
    channel_cfg.gpio_num = state.config.backlight_pin;
    channel_cfg.duty = 0;
    channel_cfg.hpoint = 0;
    ret = ledc_channel_config(&channel_cfg);
    if (ret == ESP_OK) {
        state.backlight_pwm_ready = true;
    }
    return ret;
}

} // namespace

// Low-level SPI write
static esp_err_t spiWrite(const uint8_t* data, size_t len, bool is_data) {
    if (!state.spi_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    // Set DC pin
    gpio_set_level(state.config.dc_pin, is_data ? 1 : 0);

    spi_transaction_t t = {};
    t.length = len * 8;  // Length in bits
    t.tx_buffer = data;

#if defined(CONFIG_SCRIBE_WOKWI_SIM) && CONFIG_SCRIBE_WOKWI_SIM
    if (state.config.cs_pin != GPIO_NUM_NC) {
        gpio_set_level(state.config.cs_pin, 0);
    }
#endif
    esp_err_t ret = spi_device_polling_transmit(state.spi_handle, &t);
#if defined(CONFIG_SCRIBE_WOKWI_SIM) && CONFIG_SCRIBE_WOKWI_SIM
    if (state.config.cs_pin != GPIO_NUM_NC) {
        gpio_set_level(state.config.cs_pin, 1);
    }
#endif
    return ret;
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

static esp_err_t sendDsiInitList(const uint8_t* list) {
    const uint8_t* params = list;
    while (params && params[0] != 0) {
        size_t len = params[0];
        uint8_t cmd = params[1];
        const uint8_t* data = &params[2];
        esp_err_t ret = esp_lcd_panel_io_tx_param(state.dsi_io, cmd, data, len - 1);
        if (ret != ESP_OK) {
            return ret;
        }
        params += len + 1;
    }
    return ESP_OK;
}

static esp_err_t initDSI() {
    ESP_LOGI(TAG, "Initializing ST7123 panel over MIPI-DSI");

    esp_ldo_channel_config_t ldo_cfg = {};
    ldo_cfg.chan_id = 3;
    ldo_cfg.voltage_mv = 2500;
    ldo_cfg.flags.adjustable = 1;
    esp_err_t ret = esp_ldo_acquire_channel(&ldo_cfg, &state.dsi_ldo);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to acquire DSI LDO: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_dsi_bus_config_t bus_config = {};
    bus_config.bus_id = kDsiBusId;
    bus_config.num_data_lanes = kDsiNumLanes;
    bus_config.phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT;
    bus_config.lane_bit_rate_mbps = kDsiLaneMbps;

    ret = esp_lcd_new_dsi_bus(&bus_config, &state.dsi_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create DSI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_dbi_io_config_t io_config = {};
    io_config.virtual_channel = 0;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    ret = esp_lcd_new_panel_io_dbi(state.dsi_bus, &io_config, &state.dsi_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create DSI IO: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_dpi_panel_config_t panel_config = {};
    panel_config.virtual_channel = 0;
    panel_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    panel_config.dpi_clock_freq_mhz = kDsiDpiClockMhz;
    panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    panel_config.num_fbs = 2;
    panel_config.video_timing.h_size = state.config.width;
    panel_config.video_timing.v_size = state.config.height;
    panel_config.video_timing.hsync_back_porch = kDsiHsyncBackPorch;
    panel_config.video_timing.hsync_pulse_width = kDsiHsyncPulseWidth;
    panel_config.video_timing.hsync_front_porch = kDsiHsyncFrontPorch;
    panel_config.video_timing.vsync_back_porch = kDsiVsyncBackPorch;
    panel_config.video_timing.vsync_pulse_width = kDsiVsyncPulseWidth;
    panel_config.video_timing.vsync_front_porch = kDsiVsyncFrontPorch;
    panel_config.flags.use_dma2d = true;

    ret = esp_lcd_new_panel_dpi(state.dsi_bus, &panel_config, &state.dsi_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create DSI panel: %s", esp_err_to_name(ret));
        return ret;
    }

    state.num_frame_buffers = panel_config.num_fbs ? panel_config.num_fbs : 1;
    if (state.num_frame_buffers > 1) {
        ret = esp_lcd_dpi_panel_get_frame_buffer(state.dsi_panel, 2,
                                                &state.frame_buffers[0],
                                                &state.frame_buffers[1]);
    } else {
        ret = esp_lcd_dpi_panel_get_frame_buffer(state.dsi_panel, 1,
                                                &state.frame_buffers[0]);
        state.frame_buffers[1] = nullptr;
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get DSI frame buffers: %s", esp_err_to_name(ret));
        return ret;
    }
    state.frame_buffer_size = static_cast<size_t>(state.config.width) *
                              static_cast<size_t>(state.config.height) * 2;

    tab5::IOExpander::getInstance().init();
    tab5::IOExpander::getInstance().resetDisplayAndTouch();

    ret = sendDsiInitList(st7123_init_list0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send ST7123 init list0: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = sendDsiInitList(st7123_init_list1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send ST7123 init list1: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    uint8_t madctl = 0x00;
    uint8_t colmod = 0x55;
    esp_lcd_panel_io_tx_param(state.dsi_io, PanelCommands::DSI_MADCTL, &madctl, 1);
    esp_lcd_panel_io_tx_param(state.dsi_io, PanelCommands::DSI_COLMOD, &colmod, 1);

    ret = esp_lcd_panel_init(state.dsi_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init DSI panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ST7123 initialization complete");
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
    if (!state.use_spi && state.dsi_panel) {
        esp_lcd_panel_draw_bitmap(state.dsi_panel,
                                  area->x1, area->y1,
                                  area->x2 + 1, area->y2 + 1,
                                  px_map);
        lv_display_flush_ready(display);
        return;
    }

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
#if defined(CONFIG_SCRIBE_WOKWI_SIM) && CONFIG_SCRIBE_WOKWI_SIM
        gpio_set_level(state.config.cs_pin, 1);
#endif
    }

    // Configure SPI bus
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = GPIO_NUM_23;      // Default MOSI (adjust for hardware)
    bus_cfg.miso_io_num = GPIO_NUM_NC;       // Not used for display only
    bus_cfg.sclk_io_num = GPIO_NUM_18;       // Default SCLK (adjust for hardware)
    bus_cfg.quadwp_io_num = GPIO_NUM_NC;
    bus_cfg.quadhd_io_num = GPIO_NUM_NC;
    bus_cfg.max_transfer_sz = static_cast<int>(state.buffer_size);

    int dma_chan = SPI_DMA_CH_AUTO;
#if defined(CONFIG_SCRIBE_WOKWI_SIM) && CONFIG_SCRIBE_WOKWI_SIM
    dma_chan = SPI_DMA_DISABLED;
#endif
    spi_host_device_t host = SPI2_HOST;
#if defined(CONFIG_SCRIBE_WOKWI_SIM) && CONFIG_SCRIBE_WOKWI_SIM
    host = SPI3_HOST;
#endif
    esp_err_t ret = spi_bus_initialize(host, &bus_cfg, dma_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Add device
    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.clock_speed_hz = state.config.spi_freq_mhz * 1000 * 1000;
    dev_cfg.mode = 0;  // SPI mode 0
    dev_cfg.spics_io_num = state.config.cs_pin;
#if defined(CONFIG_SCRIBE_WOKWI_SIM) && CONFIG_SCRIBE_WOKWI_SIM
    dev_cfg.spics_io_num = GPIO_NUM_NC;
#endif
    dev_cfg.queue_size = 1;
    dev_cfg.flags = SPI_DEVICE_NO_DUMMY;

    ret = spi_bus_add_device(host, &dev_cfg, &state.spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(host);
        return ret;
    }

    state.spi_host = host;
    ESP_LOGI(TAG, "SPI interface initialized");
    return ESP_OK;
}

esp_err_t MIPIDSI::init(const DisplayConfig& config) {
    if (state.initialized) {
        ESP_LOGW(TAG, "Display already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing display: %dx%d, panel=%d", config.width, config.height, (int)config.panel);

    state.config = config;
    state.use_spi = config.use_spi;

    if (state.use_spi) {
        state.buffer_size = static_cast<size_t>(config.width) * 40 * 2;  // 40 lines buffer
        esp_err_t ret = initSPI();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI initialization failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // Hardware reset
        if (config.reset_pin != GPIO_NUM_NC) {
            gpio_set_level(config.reset_pin, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
            gpio_set_level(config.reset_pin, 1);
            vTaskDelay(pdMS_TO_TICKS(150));
        }

        switch (config.panel) {
            case PanelType::ST7789:
                initST7789();
                break;
            case PanelType::ILI9341:
                initILI9341();
                break;
            default:
                ESP_LOGE(TAG, "Unsupported SPI panel type");
                return ESP_ERR_NOT_SUPPORTED;
        }

        state.draw_buffer = (uint8_t*)heap_caps_malloc(state.buffer_size, MALLOC_CAP_DMA);
        if (!state.draw_buffer) {
            ESP_LOGE(TAG, "Failed to allocate draw buffer");
            return ESP_ERR_NO_MEM;
        }
    } else {
        if (config.panel != PanelType::ST7123) {
            ESP_LOGE(TAG, "Unsupported DSI panel type");
            return ESP_ERR_NOT_SUPPORTED;
        }
        esp_err_t ret = initDSI();
        if (ret != ESP_OK) {
            return ret;
        }
    }

    // Create LVGL display
    state.lvgl_display = lv_display_create(config.width, config.height);
    if (!state.lvgl_display) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_ERR_NO_MEM;
    }

    // Set flush callback
    lv_display_set_flush_cb(state.lvgl_display, lvglFlushCallback);

    // Set color format before configuring buffers
    lv_display_set_color_format(state.lvgl_display, LV_COLOR_FORMAT_RGB565);

    if (state.use_spi) {
        lv_display_set_buffers(state.lvgl_display, state.draw_buffer, nullptr,
                               state.buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    } else {
        lv_display_set_buffers(state.lvgl_display,
                               state.frame_buffers[0],
                               state.num_frame_buffers > 1 ? state.frame_buffers[1] : nullptr,
                               state.frame_buffer_size,
                               LV_DISPLAY_RENDER_MODE_DIRECT);
    }

    clear(0x0000);

    state.initialized = true;
    ESP_LOGI(TAG, "Display initialization complete");
    return ESP_OK;
}

esp_err_t MIPIDSI::setOrientation(Orientation orientation) {
    state.current_orientation = orientation;

    if (state.lvgl_display) {
        bool swap = (orientation == Orientation::LANDSCAPE ||
                     orientation == Orientation::LANDSCAPE_INVERTED);
        int width = swap ? state.config.height : state.config.width;
        int height = swap ? state.config.width : state.config.height;
        lv_display_set_resolution(state.lvgl_display, width, height);
    }

    if (!state.use_spi && state.dsi_io) {
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
        return esp_lcd_panel_io_tx_param(state.dsi_io, PanelCommands::DSI_MADCTL, &madctl, 1);
    }

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

MIPIDSI::Orientation MIPIDSI::getOrientation() {
    return state.current_orientation;
}

void MIPIDSI::writePixels(const uint16_t* pixels, size_t len) {
    if (!state.use_spi) {
        return;
    }
    writeData((const uint8_t*)pixels, len * 2);
}

void MIPIDSI::fillRect(int x1, int y1, int x2, int y2, uint16_t color) {
    if (!state.use_spi) {
        return;
    }

    setWindow(x1, y1, x2, y2);

    size_t pixel_count = (x2 - x1 + 1) * (y2 - y1 + 1);
    for (size_t i = 0; i < pixel_count; i++) {
        writeData16(color);
    }
}

void MIPIDSI::clear(uint16_t color) {
    if (!state.use_spi && state.frame_buffers[0]) {
        size_t pixels = static_cast<size_t>(state.config.width) *
                        static_cast<size_t>(state.config.height);
        for (int i = 0; i < state.num_frame_buffers; ++i) {
            if (!state.frame_buffers[i]) {
                continue;
            }
            uint16_t* fb = static_cast<uint16_t*>(state.frame_buffers[i]);
            for (size_t p = 0; p < pixels; ++p) {
                fb[p] = color;
            }
        }
        esp_lcd_panel_draw_bitmap(state.dsi_panel, 0, 0,
                                  state.config.width, state.config.height,
                                  state.frame_buffers[0]);
        return;
    }
    fillRect(0, 0, state.config.width - 1, state.config.height - 1, color);
}

esp_err_t MIPIDSI::reset() {
    if (!state.use_spi) {
        tab5::IOExpander::getInstance().resetDisplayAndTouch();
        sendDsiInitList(st7123_init_list0);
        sendDsiInitList(st7123_init_list1);
        vTaskDelay(pdMS_TO_TICKS(120));
        uint8_t madctl = 0x00;
        uint8_t colmod = 0x55;
        esp_lcd_panel_io_tx_param(state.dsi_io, PanelCommands::DSI_MADCTL, &madctl, 1);
        esp_lcd_panel_io_tx_param(state.dsi_io, PanelCommands::DSI_COLMOD, &colmod, 1);
        return esp_lcd_panel_init(state.dsi_panel);
    }

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
    if (!state.use_spi && state.dsi_io) {
        uint8_t cmd = enter ? PanelCommands::DSI_DISPOFF : PanelCommands::DSI_DISPON;
        esp_err_t ret = esp_lcd_panel_io_tx_param(state.dsi_io, cmd, nullptr, 0);
        if (!enter) {
            uint8_t slpout = PanelCommands::DSI_SLPOUT;
            esp_lcd_panel_io_tx_param(state.dsi_io, slpout, nullptr, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        return ret;
    }

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
    if (state.config.backlight_pin == GPIO_NUM_NC) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    if (initBacklightPwm() == ESP_OK && state.backlight_pwm_ready) {
        int duty = (percent * kBacklightMaxDuty) / 100;
        ledc_set_duty(kBacklightSpeedMode, static_cast<ledc_channel_t>(kBacklightChannel), duty);
        ledc_update_duty(kBacklightSpeedMode, static_cast<ledc_channel_t>(kBacklightChannel));
        return ESP_OK;
    }

    gpio_set_level(state.config.backlight_pin, percent > 0 ? 1 : 0);
    return ESP_OK;
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
        spi_bus_free(state.spi_host);
        state.spi_handle = nullptr;
    }

    if (state.dsi_panel) {
        esp_lcd_panel_del(state.dsi_panel);
        state.dsi_panel = nullptr;
    }
    if (state.dsi_io) {
        esp_lcd_panel_io_del(state.dsi_io);
        state.dsi_io = nullptr;
    }
    if (state.dsi_bus) {
        esp_lcd_del_dsi_bus(state.dsi_bus);
        state.dsi_bus = nullptr;
    }
    if (state.dsi_ldo) {
        esp_ldo_release_channel(state.dsi_ldo);
        state.dsi_ldo = nullptr;
    }

    state.initialized = false;
    ESP_LOGI(TAG, "Display deinitialized");
}
