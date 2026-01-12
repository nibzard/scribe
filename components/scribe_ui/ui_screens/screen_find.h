#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

// Find bar - Search within document
class ScreenFind {
public:
    using CloseCallback = std::function<void()>;

    ScreenFind();
    ~ScreenFind();

    void init();
    void show();
    void hide();

    void setCloseCallback(CloseCallback cb) { close_cb_ = cb; }

    void setQuery(const std::string& query);
    std::string getQuery() const;
    void appendQueryChar(char c);
    void backspaceQuery();

    void showMatch(int current, int total);
    void showNoResults();

private:
    lv_obj_t* overlay_ = nullptr;
    lv_obj_t* label_ = nullptr;
    lv_obj_t* search_input_ = nullptr;
    lv_obj_t* match_label_ = nullptr;
    lv_obj_t* hint_label_ = nullptr;

    CloseCallback close_cb_;

    void createWidgets();
};
