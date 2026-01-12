#include "key_event.h"
#include <sstream>

std::string KeyEvent::toString() const {
    std::stringstream ss;
    if (ctrl) ss << "Ctrl+";
    if (shift) ss << "Shift+";
    if (alt) ss << "Alt+";

    switch (key) {
        case Key::ENTER: ss << "Enter"; break;
        case Key::TAB: ss << "Tab"; break;
        case Key::BACKSPACE: ss << "Backspace"; break;
        case Key::DELETE: ss << "Delete"; break;
        case Key::ESC: ss << "Esc"; break;
        case Key::SPACE: ss << "Space"; break;
        case Key::UP: ss << "Up"; break;
        case Key::DOWN: ss << "Down"; break;
        case Key::LEFT: ss << "Left"; break;
        case Key::RIGHT: ss << "Right"; break;
        case Key::HOME: ss << "Home"; break;
        case Key::END: ss << "End"; break;
        default:
            if (isPrintable()) ss << char_code;
            else ss << "Key(" << (int)key << ")";
            break;
    }
    return ss.str();
}
