#include "keybinding.h"

bool KeybindingDispatcher::processEvent(const KeyEvent& event) {
    if (!event.pressed) return false;  // Only handle key press events

    // Check global shortcuts first (highest priority)
    if (handleGlobalShortcut(event)) {
        return true;
    }

    // Then editor shortcuts
    if (handleEditorShortcut(event)) {
        return true;
    }

    // Finally text input
    if (handleTextInput(event)) {
        return true;
    }

    return false;
}

bool KeybindingDispatcher::handleGlobalShortcut(const KeyEvent& event) {
    // Esc - Open/close Menu
    if (event.key == KeyEvent::Key::ESC) {
        if (menu_cb_) menu_cb_();
        return true;
    }

    // F1 or Space hold - Show HUD (handled by separate timer logic for Space hold)
    if (event.key == KeyEvent::Key::F1) {
        if (hud_cb_) hud_cb_();
        return true;
    }

    // Ctrl+N - New project
    if (event.ctrl && event.key == KeyEvent::Key::N) {
        if (new_project_cb_) new_project_cb_();
        return true;
    }

    // Ctrl+O - Switch project
    if (event.ctrl && event.key == KeyEvent::Key::O) {
        if (switch_project_cb_) switch_project_cb_();
        return true;
    }

    // Ctrl+E - Export
    if (event.ctrl && event.key == KeyEvent::Key::E) {
        if (export_cb_) export_cb_();
        return true;
    }

    // Ctrl+, - Settings
    if (event.ctrl && event.key == KeyEvent::Key::COMMA) {
        if (settings_cb_) settings_cb_();
        return true;
    }

    // Ctrl+Q - Power off
    if (event.ctrl && event.key == KeyEvent::Key::Q) {
        if (power_off_cb_) power_off_cb_();
        return true;
    }

    return false;
}

bool KeybindingDispatcher::handleEditorShortcut(const KeyEvent& event) {
    // Arrow keys - Navigation
    if (event.key == KeyEvent::Key::LEFT) {
        if (event.shift && select_left_cb_) {
            select_left_cb_(-1);
        } else if (event.ctrl && move_word_left_cb_) {
            move_word_left_cb_();
        } else if (move_left_cb_) {
            move_left_cb_(-1);
        }
        return true;
    }

    if (event.key == KeyEvent::Key::RIGHT) {
        if (event.shift && select_right_cb_) {
            select_right_cb_(1);
        } else if (event.ctrl && move_word_right_cb_) {
            move_word_right_cb_();
        } else if (move_right_cb_) {
            move_right_cb_(1);
        }
        return true;
    }

    if (event.key == KeyEvent::Key::UP) {
        if (event.shift && select_up_cb_) {
            select_up_cb_(-1);
        } else if (move_up_cb_) {
            move_up_cb_(-1);
        }
        return true;
    }

    if (event.key == KeyEvent::Key::DOWN) {
        if (event.shift && select_down_cb_) {
            select_down_cb_(1);
        } else if (move_down_cb_) {
            move_down_cb_(1);
        }
        return true;
    }

    // Home/End - Line navigation
    if (event.key == KeyEvent::Key::HOME) {
        if (event.ctrl && move_doc_start_cb_) {
            move_doc_start_cb_();
        } else if (move_home_cb_) {
            move_home_cb_();
        }
        return true;
    }

    if (event.key == KeyEvent::Key::END) {
        if (event.ctrl && move_doc_end_cb_) {
            move_doc_end_cb_();
        } else if (move_end_cb_) {
            move_end_cb_();
        }
        return true;
    }

    // Page Up/Down
    if (event.key == KeyEvent::Key::PAGE_UP) {
        if (move_page_up_cb_) move_page_up_cb_();
        return true;
    }

    if (event.key == KeyEvent::Key::PAGE_DOWN) {
        if (move_page_down_cb_) move_page_down_cb_();
        return true;
    }

    // Backspace/Delete
    if (event.key == KeyEvent::Key::BACKSPACE) {
        if (event.ctrl && delete_word_cb_) {
            delete_word_cb_(-1);  // Ctrl+Backspace
        } else if (delete_char_cb_) {
            delete_char_cb_(-1);  // Backspace
        }
        return true;
    }

    if (event.key == KeyEvent::Key::DELETE) {
        if (event.ctrl && delete_word_cb_) {
            delete_word_cb_(1);  // Ctrl+Delete
        } else if (delete_char_cb_) {
            delete_char_cb_(1);  // Delete
        }
        return true;
    }

    // Ctrl+Z / Ctrl+Y - Undo/Redo
    if (event.ctrl && event.key == KeyEvent::Key::Z) {
        if (undo_cb_) undo_cb_();
        return true;
    }

    if (event.ctrl && event.key == KeyEvent::Key::Y) {
        if (redo_cb_) redo_cb_();
        return true;
    }

    // Ctrl+F - Find
    if (event.ctrl && event.key == KeyEvent::Key::F) {
        if (find_cb_) find_cb_();
        return true;
    }

    // Ctrl+S - Save (reassurance)
    if (event.ctrl && event.key == KeyEvent::Key::S) {
        if (save_cb_) save_cb_();
        return true;
    }

    // Ctrl+A - Select all
    if (event.ctrl && event.key == KeyEvent::Key::A) {
        if (select_all_cb_) select_all_cb_();
        return true;
    }

    // Ctrl+M - Toggle Draft/Revise mode
    if (event.ctrl && event.key == KeyEvent::Key::M) {
        if (toggle_mode_cb_) toggle_mode_cb_();
        return true;
    }

    // Ctrl+K - AI Magic Bar
    if (event.ctrl && event.key == KeyEvent::Key::K) {
        if (ai_magic_cb_) ai_magic_cb_();
        return true;
    }

    return false;
}

bool KeybindingDispatcher::handleTextInput(const KeyEvent& event) {
    // Check if this is a printable character
    if (event.isPrintable() && insert_text_cb_) {
        char str[2] = {event.char_code, 0};
        insert_text_cb_(std::string(str));
        return true;
    }

    // Handle Enter key
    if (event.key == KeyEvent::Key::ENTER && insert_text_cb_) {
        insert_text_cb_("\n");
        return true;
    }

    // Handle Tab
    if (event.key == KeyEvent::Key::TAB && insert_text_cb_) {
        insert_text_cb_("    ");  // 4 spaces
        return true;
    }

    return false;
}
