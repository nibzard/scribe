#pragma once

#include "key_event.h"
#include <functional>

// Keybinding dispatcher - maps key combinations to actions
// Based on SPECS.md section 4: Keybinding spec

class KeybindingDispatcher {
public:
    // Action callback type
    using ActionCallback = std::function<void()>;

    // Set callbacks for various actions
    void setMenuCallback(ActionCallback cb) { menu_cb_ = cb; }
    void setHUDCallback(ActionCallback cb) { hud_cb_ = cb; }
    void setNewProjectCallback(ActionCallback cb) { new_project_cb_ = cb; }
    void setSwitchProjectCallback(ActionCallback cb) { switch_project_cb_ = cb; }
    void setExportCallback(ActionCallback cb) { export_cb_ = cb; }
    void setSettingsCallback(ActionCallback cb) { settings_cb_ = cb; }
    void setHelpCallback(ActionCallback cb) { help_cb_ = cb; }
    void setSleepCallback(ActionCallback cb) { sleep_cb_ = cb; }
    void setPowerOffCallback(ActionCallback cb) { power_off_cb_ = cb; }
    void setFindCallback(ActionCallback cb) { find_cb_ = cb; }
    void setSaveCallback(ActionCallback cb) { save_cb_ = cb; }

    // Set callbacks for editor actions
    using MoveCallback = std::function<void(int delta)>;
    void setMoveLeftCallback(MoveCallback cb) { move_left_cb_ = cb; }
    void setMoveRightCallback(MoveCallback cb) { move_right_cb_ = cb; }
    void setMoveUpCallback(MoveCallback cb) { move_up_cb_ = cb; }
    void setMoveDownCallback(MoveCallback cb) { move_down_cb_ = cb; }
    void setSelectLeftCallback(MoveCallback cb) { select_left_cb_ = cb; }
    void setSelectRightCallback(MoveCallback cb) { select_right_cb_ = cb; }
    void setSelectUpCallback(MoveCallback cb) { select_up_cb_ = cb; }
    void setSelectDownCallback(MoveCallback cb) { select_down_cb_ = cb; }
    void setMoveWordLeftCallback(ActionCallback cb) { move_word_left_cb_ = cb; }
    void setMoveWordRightCallback(ActionCallback cb) { move_word_right_cb_ = cb; }
    void setMoveHomeCallback(ActionCallback cb) { move_home_cb_ = cb; }
    void setMoveEndCallback(ActionCallback cb) { move_end_cb_ = cb; }
    void setMovePageUpCallback(ActionCallback cb) { move_page_up_cb_ = cb; }
    void setMovePageDownCallback(ActionCallback cb) { move_page_down_cb_ = cb; }
    void setMoveDocStartCallback(ActionCallback cb) { move_doc_start_cb_ = cb; }
    void setMoveDocEndCallback(ActionCallback cb) { move_doc_end_cb_ = cb; }

    using TextCallback = std::function<void(const std::string&)>;
    void setInsertTextCallback(TextCallback cb) { insert_text_cb_ = cb; }
    void setDeleteCharCallback(std::function<void(int)> cb) { delete_char_cb_ = cb; }
    void setDeleteWordCallback(std::function<void(int)> cb) { delete_word_cb_ = cb; }

    void setUndoCallback(ActionCallback cb) { undo_cb_ = cb; }
    void setRedoCallback(ActionCallback cb) { redo_cb_ = cb; }
    void setSelectAllCallback(ActionCallback cb) { select_all_cb_ = cb; }
    void setToggleModeCallback(ActionCallback cb) { toggle_mode_cb_ = cb; }
    void setAIMagicCallback(ActionCallback cb) { ai_magic_cb_ = cb; }

    // Process a key event - returns true if handled
    bool processEvent(const KeyEvent& event);

private:
    // Global shortcuts
    ActionCallback menu_cb_;
    ActionCallback hud_cb_;
    ActionCallback new_project_cb_;
    ActionCallback switch_project_cb_;
    ActionCallback export_cb_;
    ActionCallback settings_cb_;
    ActionCallback help_cb_;
    ActionCallback sleep_cb_;
    ActionCallback power_off_cb_;
    ActionCallback find_cb_;
    ActionCallback save_cb_;

    // Editor navigation
    MoveCallback move_left_cb_;
    MoveCallback move_right_cb_;
    MoveCallback move_up_cb_;
    MoveCallback move_down_cb_;
    MoveCallback select_left_cb_;
    MoveCallback select_right_cb_;
    MoveCallback select_up_cb_;
    MoveCallback select_down_cb_;
    ActionCallback move_word_left_cb_;
    ActionCallback move_word_right_cb_;
    ActionCallback move_home_cb_;
    ActionCallback move_end_cb_;
    ActionCallback move_page_up_cb_;
    ActionCallback move_page_down_cb_;
    ActionCallback move_doc_start_cb_;
    ActionCallback move_doc_end_cb_;

    // Editor editing
    TextCallback insert_text_cb_;
    std::function<void(int)> delete_char_cb_;
    std::function<void(int)> delete_word_cb_;

    // Editor state
    ActionCallback undo_cb_;
    ActionCallback redo_cb_;
    ActionCallback select_all_cb_;
    ActionCallback toggle_mode_cb_;
    ActionCallback ai_magic_cb_;

    // Helper methods
    bool handleGlobalShortcut(const KeyEvent& event);
    bool handleEditorShortcut(const KeyEvent& event);
    bool handleTextInput(const KeyEvent& event);
};
