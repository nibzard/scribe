#pragma once

#include <cstdint>
#include <string>

// Normalized key event (independent of physical keyboard layout)
struct KeyEvent {
    enum class Key : uint16_t {
        UNKNOWN = 0,

        // Letters
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Numbers
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,

        // Symbols
        SPACE, ENTER, TAB, BACKSPACE, DELETE,
        MINUS, EQUAL, LBRACKET, RBRACKET, BACKSLASH,
        SEMICOLON, QUOTE, COMMA, PERIOD, SLASH,
        GRAVE,

        // Navigation
        UP, DOWN, LEFT, RIGHT,
        HOME, END, PAGE_UP, PAGE_DOWN,

        // Modifiers
        LSHIFT, RSHIFT, LCTRL, RCTRL, LALT, RALT,

        // Special
        ESC, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        INSERT,
    };

    Key key = Key::UNKNOWN;
    bool pressed = false;     // true for key down, false for key up
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool meta = false;

    // For printable characters, the resulting character
    char char_code = 0;

    // Check if this is a printable character
    bool isPrintable() const {
        return char_code > 0 && char_code >= 32 && char_code <= 126;
    }

    // Get display string for this key
    std::string toString() const;
};

// Map USB HID Usage ID to KeyEvent::Key
KeyEvent::Key mapHIDUsageToKey(uint8_t usage);

// Map key to character for given shift state
char mapKeyToChar(KeyEvent::Key key, bool shift);
