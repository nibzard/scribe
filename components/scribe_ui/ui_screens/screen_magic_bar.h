#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// AI suggestion state
enum class AISuggestionState {
    IDLE,
    CONNECTING,
    GENERATING,
    READY,
    ERROR
};

// Magic Bar screen - AI suggestion preview
// Displays streaming AI suggestions with insert/discard controls
class ScreenMagicBar {
public:
    using InsertCallback = std::function<void(const std::string& text)>;
    using CancelCallback = std::function<void()>;

    ScreenMagicBar();
    ~ScreenMagicBar();

    void init();
    void show();
    void hide();

    // Stream callback for AI suggestions
    void onStreamDelta(const char* delta);

    // Complete callback
    void onComplete(bool success, const std::string& error);

    // Check if currently showing
    bool isVisible() const { return visible_; }

    // Check if suggestion is ready to insert
    bool isReady() const { return state_ == AISuggestionState::READY; }

    // Get current suggestion text
    std::string getSuggestion() const { return suggestion_; }

    // Callbacks
    void setInsertCallback(InsertCallback cb) { insert_cb_ = cb; }
    void setCancelCallback(CancelCallback cb) { cancel_cb_ = cb; }

    // Update status when request starts
    void startGenerating();

    // Accept or discard current suggestion
    void acceptSuggestion();
    void discardSuggestion();

private:
    lv_obj_t* bar_ = nullptr;
    lv_obj_t* preview_box_ = nullptr;
    lv_obj_t* preview_label_ = nullptr;
    lv_obj_t* btn_cont_ = nullptr;
    lv_obj_t* insert_btn_ = nullptr;
    lv_obj_t* discard_btn_ = nullptr;
    lv_obj_t* status_label_ = nullptr;

    bool visible_ = false;
    AISuggestionState state_ = AISuggestionState::IDLE;
    std::string suggestion_;

    InsertCallback insert_cb_;
    CancelCallback cancel_cb_;

    void createWidgets();
    void updateStatus(const char* status);
    void updatePreview();
};
