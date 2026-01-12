#pragma once

#include <lvgl.h>
#include <array>
#include <string>
#include <functional>
#include "../scribe_services/ai_assist.h"

// AI suggestion state
enum class AISuggestionState {
    IDLE,
    CONNECTING,
    GENERATING,
    READY,
    ERROR
};

// Magic Bar screen - AI writing assistant
// Shows at bottom of editor when activated (Ctrl+K)
// Displays streaming AI suggestions with insert/discard controls
class ScreenMagicBar {
public:
    using InsertCallback = std::function<void(const std::string& text)>;
    using CancelCallback = std::function<void()>;
    using StyleCallback = std::function<void(AIStyle style)>;

    ScreenMagicBar();
    ~ScreenMagicBar();

    void init();
    void show();
    void hide();

    // Set the text to get suggestions for
    void setContextText(const std::string& text) { context_text_ = text; }

    // Set style for suggestion
    void setStyle(AIStyle style) { style_ = style; }

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

    // Get current style
    AIStyle getStyle() const { return style_; }

    // Callbacks
    void setInsertCallback(InsertCallback cb) { insert_cb_ = cb; }
    void setCancelCallback(CancelCallback cb) { cancel_cb_ = cb; }
    void setStyleCallback(StyleCallback cb) { style_cb_ = cb; }

    // Update status when request starts
    void startGenerating();

    // Accept or discard current suggestion
    void acceptSuggestion();
    void discardSuggestion();

private:
    struct StyleButtonContext {
        ScreenMagicBar* magic = nullptr;
        AIStyle style = AIStyle::CONTINUE;
    };

    lv_obj_t* bar_ = nullptr;
    lv_obj_t* style_selector_ = nullptr;
    lv_obj_t* preview_text_ = nullptr;
    lv_obj_t* insert_btn_ = nullptr;
    lv_obj_t* discard_btn_ = nullptr;
    lv_obj_t* status_label_ = nullptr;

    bool visible_ = false;
    AISuggestionState state_ = AISuggestionState::IDLE;
    AIStyle style_ = AIStyle::CONTINUE;
    std::string context_text_;
    std::string suggestion_;

    InsertCallback insert_cb_;
    CancelCallback cancel_cb_;
    StyleCallback style_cb_;

    std::array<StyleButtonContext, 4> style_button_ctx_{};

    void createWidgets();
    void updateStyleSelector();
    void updateStatus(const char* status);
    void updatePreview();

    static const char* getStyleName(AIStyle style);
};
