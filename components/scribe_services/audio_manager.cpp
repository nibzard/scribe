#include "audio_manager.h"
#include "../scribe_hw/tab5_i2c.h"
#include "../scribe_hw/tab5_io_expander.h"
#include <driver/i2s_std.h>
#include <driver/i2s_tdm.h>
#include <esp_codec_dev_defaults.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <es7210_adc.h>
#include <es8388_codec.h>

namespace {
static const char* TAG = "SCRIBE_AUDIO";

constexpr uint32_t kSampleRateHz = 48000;
constexpr uint32_t kBitsPerSample = 16;
constexpr uint8_t kPlaybackChannels = 2;
constexpr uint8_t kRecordChannels = 4;

// I2S pins (Tab5)
constexpr gpio_num_t kI2sBclk = GPIO_NUM_27;
constexpr gpio_num_t kI2sMclk = GPIO_NUM_30;
constexpr gpio_num_t kI2sLrck = GPIO_NUM_29;
constexpr gpio_num_t kI2sDout = GPIO_NUM_26;
constexpr gpio_num_t kI2sDin = GPIO_NUM_28;

static i2s_chan_handle_t s_tx_chan = nullptr;
static i2s_chan_handle_t s_rx_chan = nullptr;
static const audio_codec_data_if_t* s_i2s_data_if = nullptr;
static esp_codec_dev_handle_t s_play_dev = nullptr;
static esp_codec_dev_handle_t s_record_dev = nullptr;
}  // namespace

AudioManager& AudioManager::getInstance() {
    static AudioManager instance;
    return instance;
}

esp_err_t AudioManager::init() {
    if (initialized_.load()) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing audio codec...");

    tab5::I2CBus& bus = tab5::I2CBus::getInstance();
    esp_err_t ret = bus.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    tab5::IOExpander::getInstance().init();
    tab5::IOExpander::getInstance().setSpeakerEnable(true);

    if (!s_tx_chan || !s_rx_chan) {
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true;
        ret = i2s_new_channel(&chan_cfg, &s_tx_chan, &s_rx_chan);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S channel init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        i2s_std_config_t tx_cfg = {};
        tx_cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kSampleRateHz);
        tx_cfg.slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                            I2S_SLOT_MODE_STEREO);
        tx_cfg.gpio_cfg = {
            .mclk = kI2sMclk,
            .bclk = kI2sBclk,
            .ws = kI2sLrck,
            .dout = kI2sDout,
            .din = kI2sDin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        };

        ret = i2s_channel_init_std_mode(s_tx_chan, &tx_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S TX init failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = i2s_channel_enable(s_tx_chan);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S TX enable failed: %s", esp_err_to_name(ret));
            return ret;
        }

        i2s_tdm_config_t rx_cfg = {};
        rx_cfg.clk_cfg.sample_rate_hz = kSampleRateHz;
        rx_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        rx_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
        rx_cfg.clk_cfg.bclk_div = 8;
        rx_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
        rx_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
        rx_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
        rx_cfg.slot_cfg.slot_mask = static_cast<i2s_tdm_slot_mask_t>(
            I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3);
        rx_cfg.slot_cfg.ws_width = I2S_TDM_AUTO_WS_WIDTH;
        rx_cfg.slot_cfg.ws_pol = false;
        rx_cfg.slot_cfg.bit_shift = true;
        rx_cfg.slot_cfg.left_align = false;
        rx_cfg.slot_cfg.big_endian = false;
        rx_cfg.slot_cfg.bit_order_lsb = false;
        rx_cfg.slot_cfg.skip_mask = false;
        rx_cfg.slot_cfg.total_slot = I2S_TDM_AUTO_SLOT_NUM;
        rx_cfg.gpio_cfg.mclk = kI2sMclk;
        rx_cfg.gpio_cfg.bclk = kI2sBclk;
        rx_cfg.gpio_cfg.ws = kI2sLrck;
        rx_cfg.gpio_cfg.dout = kI2sDout;
        rx_cfg.gpio_cfg.din = kI2sDin;
        rx_cfg.gpio_cfg.invert_flags.mclk_inv = false;
        rx_cfg.gpio_cfg.invert_flags.bclk_inv = false;
        rx_cfg.gpio_cfg.invert_flags.ws_inv = false;

        ret = i2s_channel_init_tdm_mode(s_rx_chan, &rx_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S RX init failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = i2s_channel_enable(s_rx_chan);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S RX enable failed: %s", esp_err_to_name(ret));
            return ret;
        }

        audio_codec_i2s_cfg_t i2s_cfg = {};
        i2s_cfg.port = I2S_NUM_0;
        i2s_cfg.rx_handle = s_rx_chan;
        i2s_cfg.tx_handle = s_tx_chan;
        i2s_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        s_i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
        if (!s_i2s_data_if) {
            ESP_LOGE(TAG, "Failed to create I2S data interface");
            return ESP_ERR_NO_MEM;
        }
    }

    if (!s_play_dev) {
        audio_codec_i2c_cfg_t i2c_cfg = {
            .port = I2C_NUM_1,
            .addr = ES8388_CODEC_DEFAULT_ADDR,
            .bus_handle = bus.handle(),
        };
        const audio_codec_ctrl_if_t* i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
        if (!i2c_ctrl_if) {
            ESP_LOGE(TAG, "Failed to create ES8388 I2C control");
            return ESP_ERR_NO_MEM;
        }

        es8388_codec_cfg_t es8388_cfg = {};
        es8388_cfg.ctrl_if = i2c_ctrl_if;
        es8388_cfg.gpio_if = audio_codec_new_gpio();
        es8388_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
        es8388_cfg.master_mode = false;
        es8388_cfg.pa_pin = -1;
        const audio_codec_if_t* es8388_dev = es8388_codec_new(&es8388_cfg);
        if (!es8388_dev) {
            ESP_LOGE(TAG, "Failed to create ES8388 codec");
            return ESP_ERR_NO_MEM;
        }

        esp_codec_dev_cfg_t dev_cfg = {
            .dev_type = ESP_CODEC_DEV_TYPE_OUT,
            .codec_if = es8388_dev,
            .data_if = s_i2s_data_if,
        };
        s_play_dev = esp_codec_dev_new(&dev_cfg);
        if (!s_play_dev) {
            ESP_LOGE(TAG, "Failed to create speaker device");
            return ESP_ERR_NO_MEM;
        }
    }

    if (!s_record_dev) {
        audio_codec_i2c_cfg_t i2c_cfg = {
            .port = I2C_NUM_1,
            .addr = ES7210_CODEC_DEFAULT_ADDR,
            .bus_handle = bus.handle(),
        };
        const audio_codec_ctrl_if_t* i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
        if (!i2c_ctrl_if) {
            ESP_LOGE(TAG, "Failed to create ES7210 I2C control");
            return ESP_ERR_NO_MEM;
        }

        es7210_codec_cfg_t es7210_cfg = {};
        es7210_cfg.ctrl_if = i2c_ctrl_if;
        es7210_cfg.master_mode = false;
        es7210_cfg.mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2 |
                                  ES7210_SEL_MIC3 | ES7210_SEL_MIC4;
        es7210_cfg.mclk_src = ES7210_MCLK_FROM_PAD;
        es7210_cfg.mclk_div = 256;

        const audio_codec_if_t* es7210_dev = es7210_codec_new(&es7210_cfg);
        if (!es7210_dev) {
            ESP_LOGE(TAG, "Failed to create ES7210 codec");
            return ESP_ERR_NO_MEM;
        }

        esp_codec_dev_cfg_t dev_cfg = {
            .dev_type = ESP_CODEC_DEV_TYPE_IN,
            .codec_if = es7210_dev,
            .data_if = s_i2s_data_if,
        };
        s_record_dev = esp_codec_dev_new(&dev_cfg);
        if (!s_record_dev) {
            ESP_LOGE(TAG, "Failed to create mic device");
            return ESP_ERR_NO_MEM;
        }
    }

    esp_codec_dev_sample_info_t spk_info = {};
    spk_info.bits_per_sample = kBitsPerSample;
    spk_info.channel = kPlaybackChannels;
    spk_info.sample_rate = kSampleRateHz;
    ret = esp_codec_dev_open(s_play_dev, &spk_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open speaker codec: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_codec_dev_sample_info_t mic_info = {};
    mic_info.bits_per_sample = kBitsPerSample;
    mic_info.channel = kRecordChannels;
    mic_info.sample_rate = kSampleRateHz;
    ret = esp_codec_dev_open(s_record_dev, &mic_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open mic codec: %s", esp_err_to_name(ret));
        return ret;
    }

    speaker_ready_.store(true);
    mic_ready_.store(true);
    setSpeakerVolume(speaker_volume_);
    initialized_.store(true);

    ESP_LOGI(TAG, "Audio codec initialized");
    return ESP_OK;
}

esp_err_t AudioManager::setSpeakerVolume(uint8_t volume) {
    speaker_volume_ = volume > 100 ? 100 : volume;
    if (!s_play_dev) {
        return ESP_ERR_INVALID_STATE;
    }

    if (speaker_volume_ == 0) {
        return esp_codec_dev_set_out_mute(s_play_dev, true);
    }

    esp_err_t ret = esp_codec_dev_set_out_mute(s_play_dev, false);
    ret |= esp_codec_dev_set_out_vol(s_play_dev, speaker_volume_);
    return ret;
}

esp_err_t AudioManager::record(std::vector<int16_t>& data, uint32_t duration_ms, float gain) {
    if (!mic_ready_.load() || !s_record_dev) {
        return ESP_ERR_INVALID_STATE;
    }

    const size_t total_samples =
        static_cast<size_t>(kSampleRateHz) * kRecordChannels * duration_ms / 1000;
    data.resize(total_samples);

    esp_codec_dev_set_in_gain(s_record_dev, gain);
    return esp_codec_dev_read(s_record_dev, data.data(), data.size() * sizeof(int16_t));
}

esp_err_t AudioManager::playBlocking(const std::vector<int16_t>& data) {
    if (!speaker_ready_.load() || !s_play_dev) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data.empty()) {
        return ESP_OK;
    }

    std::vector<int16_t> scratch = data;
    return playBlockingMutable(scratch);
}

esp_err_t AudioManager::playBlockingMutable(std::vector<int16_t>& data) {
    if (!speaker_ready_.load() || !s_play_dev) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_codec_dev_write(s_play_dev, data.data(), data.size() * sizeof(int16_t));
}

void AudioManager::playTask(void* arg) {
    AudioManager* manager = static_cast<AudioManager*>(arg);
    std::vector<int16_t> local;

    {
        std::lock_guard<std::mutex> lock(manager->play_mutex_);
        local = manager->play_buffer_;
    }

    manager->playBlockingMutable(local);
    manager->playing_.store(false);

    vTaskDelete(nullptr);
}

esp_err_t AudioManager::play(const std::vector<int16_t>& data, bool async) {
    if (!speaker_ready_.load() || !s_play_dev) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!async) {
        return playBlocking(data);
    }

    if (playing_.load()) {
        return ESP_ERR_INVALID_STATE;
    }

    {
        std::lock_guard<std::mutex> lock(play_mutex_);
        play_buffer_ = data;
    }

    playing_.store(true);
    if (xTaskCreate(playTask, "audio_play", 4096, this, 5, nullptr) != pdPASS) {
        playing_.store(false);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
