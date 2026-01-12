#pragma once

#include <lvgl.h>
#include <string>
#include <functional>
#include <vector>

// Export screen - SD card export and Send to Computer
class ScreenExport {
public:
    using ExportCallback = std::function<void(const std::string& type)>;
    using CloseCallback = std::function<void()>;

    ScreenExport();
    ~ScreenExport();

    void init();
    void show();
    void hide();

    void setExportCallback(ExportCallback cb) { export_cb_ = cb; }
    void setCloseCallback(CloseCallback cb) { close_cb_ = cb; }

    void moveSelection(int delta);
    void selectCurrent();
    std::string getSelectedType() const;

    void updateProgress(size_t current, size_t total);
    void showComplete(const char* message);
    void showSendInstructions();
    void showSendRunning();
    void showSendDone();
    bool isSendInstructions() const { return mode_ == Mode::SendInstructions; }
    bool isSendRunning() const { return mode_ == Mode::SendRunning; }

private:
    enum class Mode {
        List,
        SendInstructions,
        SendRunning
    };

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* btn_list_ = nullptr;
    lv_obj_t* progress_label_ = nullptr;
    lv_obj_t* privacy_label_ = nullptr;
    lv_obj_t* instructions_body_ = nullptr;
    lv_obj_t* instructions_confirm_ = nullptr;
    lv_obj_t* instructions_cancel_ = nullptr;

    int selected_index_ = 0;
    std::vector<std::string> types_;
    std::vector<lv_obj_t*> buttons_;
    Mode mode_ = Mode::List;

    ExportCallback export_cb_;
    CloseCallback close_cb_;

    void createWidgets();
    void updateSelection();
    void setMode(Mode mode);
};
