/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "lvgl_cpp/label.h"
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <memory>
#include <mooncake_log.h>
#include <smooth_ui_toolkit.h>
#include <smooth_lvgl.h>
#include <apps/utils/audio/audio.h>
#include <apps/utils/ui/window.h>
#include <apps/utils/ui/toast.h>
#include <src/widgets/label/lv_label.h>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "panel-sd";

static const ui::Window::KeyFrame_t _kf_sd_card_scan_close = {-46, 300, 75, 75, 0};
static const ui::Window::KeyFrame_t _kf_sd_card_scan_open  = {-40, 43, 566, 411, 255};

class SdCardScanWindow : public ui::Window {
public:
    SdCardScanWindow()
    {
        config.title    = "SD-Card File Scan";
        config.kfClosed = _kf_sd_card_scan_close;
        config.kfOpened = _kf_sd_card_scan_open;
    }

    void onOpen() override
    {
        _window->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

        _panel_file_entries = std::make_unique<Container>(_window->get());
        _panel_file_entries->align(LV_ALIGN_CENTER, 0, 18);
        _panel_file_entries->setSize(535, 345);
        _panel_file_entries->setBorderWidth(0);
        _panel_file_entries->setBgColor(lv_color_hex(0x393939));
        _panel_file_entries->setPadding(12, 12, 24, 24);

        _label_msg = std::make_unique<Label>(_window->get());
        _label_msg->align(LV_ALIGN_CENTER, 0, -24);
        _label_msg->setTextFont(&lv_font_montserrat_24);
        if (GetHAL()->isSdCardMounted()) {
            _label_msg->setText("Scanning SD Card ...");
        } else {
            _label_msg->setTextColor(lv_color_hex(0xFD4444));
            _label_msg->setText("SD Card not mounted.\n\nPlease insert SD Card and reboot.");
        }
    }

    void onUpdate() override
    {
        if (_state != Opened) {
            return;
        }

        if (GetHAL()->isSdCardMounted() && !_is_scanned) {
            _is_scanned       = true;
            auto file_entries = GetHAL()->scanSdCard("/");
            if (file_entries.empty()) {
                _label_msg->setText("No files found on SD Card.");
                return;
            }

            _label_file_entries.clear();

            for (size_t i = 0; i < file_entries.size(); i++) {
                _label_file_entries.push_back(std::make_unique<Label>(_panel_file_entries->get()));
                _label_file_entries.back()->align(LV_ALIGN_TOP_LEFT, 0, i * 42);
                _label_file_entries.back()->setTextFont(&lv_font_montserrat_24);
                if (file_entries[i].isDir) {
                    _label_file_entries.back()->setTextColor(lv_color_hex(0xFDBE1A));
                    lv_label_set_text(_label_file_entries.back()->get(), LV_SYMBOL_DIRECTORY);
                } else {
                    _label_file_entries.back()->setTextColor(lv_color_hex(0x43D2FF));
                    lv_label_set_text(_label_file_entries.back()->get(), LV_SYMBOL_FILE);
                }

                _label_file_entries.push_back(std::make_unique<Label>(_panel_file_entries->get()));
                _label_file_entries.back()->align(LV_ALIGN_TOP_LEFT, 36, i * 42);
                _label_file_entries.back()->setTextFont(&lv_font_montserrat_24);
                _label_file_entries.back()->setTextColor(lv_color_hex(file_entries[i].isDir ? 0xFDBE1A : 0x43D2FF));
                _label_file_entries.back()->setText(file_entries[i].name);
            }

            _label_msg.reset();
        }
    }

    void onClose() override
    {
        audio::play_next_tone_progression();
        _label_msg.reset();
        _label_file_entries.clear();
        _panel_file_entries.reset();
    }

private:
    std::unique_ptr<Label> _label_msg;
    std::unique_ptr<Container> _panel_file_entries;
    std::vector<std::unique_ptr<Label>> _label_file_entries;
    bool _is_scanned = false;
};

void PanelSdCard::init()
{
    _btn_sd_card_scan = std::make_unique<Container>(lv_screen_active());
    _btn_sd_card_scan->align(LV_ALIGN_CENTER, -46, 300);
    _btn_sd_card_scan->setSize(100, 100);
    _btn_sd_card_scan->setOpa(0);
    _btn_sd_card_scan->onClick().connect([&] {
        audio::play_next_tone_progression();

        // Create window
        _window = std::make_unique<SdCardScanWindow>();
        _window->init(lv_screen_active());
        _window->open();
    });
}

void PanelSdCard::update(bool isStacked)
{
    if (_window) {
        _window->update();
        if (_window->getState() == ui::Window::State_t::Closed) {
            _window.reset();
        }
    }
}
