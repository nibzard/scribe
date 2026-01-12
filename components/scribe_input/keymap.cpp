#include "key_event.h"

// USB HID Usage ID to KeyEvent::Key mapping
// Based on USB HID Usage Tables: Keyboard/Keypad Page (0x07)

static const KeyEvent::Key hid_usage_to_key[] = {
    KeyEvent::Key::UNKNOWN,  // 0x00 - No event
    KeyEvent::Key::UNKNOWN,  // 0x01 - Keyboard ErrorRollOver
    KeyEvent::Key::UNKNOWN,  // 0x02 - Keyboard POSTFail
    KeyEvent::Key::UNKNOWN,  // 0x03 - Keyboard ErrorUndefined
    KeyEvent::Key::A,        // 0x04
    KeyEvent::Key::B,        // 0x05
    KeyEvent::Key::C,        // 0x06
    KeyEvent::Key::D,        // 0x07
    KeyEvent::Key::E,        // 0x08
    KeyEvent::Key::F,        // 0x09
    KeyEvent::Key::G,        // 0x0A
    KeyEvent::Key::H,        // 0x0B
    KeyEvent::Key::I,        // 0x0C
    KeyEvent::Key::J,        // 0x0D
    KeyEvent::Key::K,        // 0x0E
    KeyEvent::Key::L,        // 0x0F
    KeyEvent::Key::M,        // 0x10
    KeyEvent::Key::N,        // 0x11
    KeyEvent::Key::O,        // 0x12
    KeyEvent::Key::P,        // 0x13
    KeyEvent::Key::Q,        // 0x14
    KeyEvent::Key::R,        // 0x15
    KeyEvent::Key::S,        // 0x16
    KeyEvent::Key::T,        // 0x17
    KeyEvent::Key::U,        // 0x18
    KeyEvent::Key::V,        // 0x19
    KeyEvent::Key::W,        // 0x1A
    KeyEvent::Key::X,        // 0x1B
    KeyEvent::Key::Y,        // 0x1C
    KeyEvent::Key::Z,        // 0x1D
    KeyEvent::Key::_1,       // 0x1E
    KeyEvent::Key::_2,       // 0x1F
    KeyEvent::Key::_3,       // 0x20
    KeyEvent::Key::_4,       // 0x21
    KeyEvent::Key::_5,       // 0x22
    KeyEvent::Key::_6,       // 0x23
    KeyEvent::Key::_7,       // 0x24
    KeyEvent::Key::_8,       // 0x25
    KeyEvent::Key::_9,       // 0x26
    KeyEvent::Key::_0,       // 0x27
    KeyEvent::Key::ENTER,    // 0x28
    KeyEvent::Key::ESC,      // 0x29
    KeyEvent::Key::BACKSPACE,// 0x2A
    KeyEvent::Key::TAB,      // 0x2B
    KeyEvent::Key::SPACE,    // 0x2C
    KeyEvent::Key::MINUS,    // 0x2D
    KeyEvent::Key::EQUAL,    // 0x2E
    KeyEvent::Key::LBRACKET, // 0x2F
    KeyEvent::Key::RBRACKET, // 0x30
    KeyEvent::Key::BACKSLASH,// 0x31
    KeyEvent::Key::UNKNOWN,  // 0x32 - Non-US # and ~
    KeyEvent::Key::SEMICOLON,// 0x33
    KeyEvent::Key::QUOTE,    // 0x34
    KeyEvent::Key::GRAVE,    // 0x35
    KeyEvent::Key::COMMA,    // 0x36
    KeyEvent::Key::PERIOD,   // 0x37
    KeyEvent::Key::SLASH,    // 0x38
    KeyEvent::Key::UNKNOWN,  // 0x39 - Caps Lock
    KeyEvent::Key::F1,       // 0x3A
    KeyEvent::Key::F2,       // 0x3B
    KeyEvent::Key::F3,       // 0x3C
    KeyEvent::Key::F4,       // 0x3D
    KeyEvent::Key::F5,       // 0x3E
    KeyEvent::Key::F6,       // 0x3F
    KeyEvent::Key::F7,       // 0x40
    KeyEvent::Key::F8,       // 0x41
    KeyEvent::Key::F9,       // 0x42
    KeyEvent::Key::F10,      // 0x43
    KeyEvent::Key::F11,      // 0x44
    KeyEvent::Key::F12,      // 0x45
    // Add more mappings as needed
};

KeyEvent::Key mapHIDUsageToKey(uint8_t usage) {
    if (usage >= sizeof(hid_usage_to_key) / sizeof(KeyEvent::Key)) {
        return KeyEvent::Key::UNKNOWN;
    }
    return hid_usage_to_key[usage];
}

char mapKeyToChar(KeyEvent::Key key, bool shift) {
    switch (key) {
        case KeyEvent::Key::A: return shift ? 'A' : 'a';
        case KeyEvent::Key::B: return shift ? 'B' : 'b';
        case KeyEvent::Key::C: return shift ? 'C' : 'c';
        case KeyEvent::Key::D: return shift ? 'D' : 'd';
        case KeyEvent::Key::E: return shift ? 'E' : 'e';
        case KeyEvent::Key::F: return shift ? 'F' : 'f';
        case KeyEvent::Key::G: return shift ? 'G' : 'g';
        case KeyEvent::Key::H: return shift ? 'H' : 'h';
        case KeyEvent::Key::I: return shift ? 'I' : 'i';
        case KeyEvent::Key::J: return shift ? 'J' : 'j';
        case KeyEvent::Key::K: return shift ? 'K' : 'k';
        case KeyEvent::Key::L: return shift ? 'L' : 'l';
        case KeyEvent::Key::M: return shift ? 'M' : 'm';
        case KeyEvent::Key::N: return shift ? 'N' : 'n';
        case KeyEvent::Key::O: return shift ? 'O' : 'o';
        case KeyEvent::Key::P: return shift ? 'P' : 'p';
        case KeyEvent::Key::Q: return shift ? 'Q' : 'q';
        case KeyEvent::Key::R: return shift ? 'R' : 'r';
        case KeyEvent::Key::S: return shift ? 'S' : 's';
        case KeyEvent::Key::T: return shift ? 'T' : 't';
        case KeyEvent::Key::U: return shift ? 'U' : 'u';
        case KeyEvent::Key::V: return shift ? 'V' : 'v';
        case KeyEvent::Key::W: return shift ? 'W' : 'w';
        case KeyEvent::Key::X: return shift ? 'X' : 'x';
        case KeyEvent::Key::Y: return shift ? 'Y' : 'y';
        case KeyEvent::Key::Z: return shift ? 'Z' : 'z';
        case KeyEvent::Key::_0: return shift ? ')' : '0';
        case KeyEvent::Key::_1: return shift ? '!' : '1';
        case KeyEvent::Key::_2: return shift ? '@' : '2';
        case KeyEvent::Key::_3: return shift ? '#' : '3';
        case KeyEvent::Key::_4: return shift ? '$' : '4';
        case KeyEvent::Key::_5: return shift ? '%' : '5';
        case KeyEvent::Key::_6: return shift ? '^' : '6';
        case KeyEvent::Key::_7: return shift ? '&' : '7';
        case KeyEvent::Key::_8: return shift ? '*' : '8';
        case KeyEvent::Key::_9: return shift ? '(' : '9';
        case KeyEvent::Key::SPACE: return ' ';
        case KeyEvent::Key::MINUS: return shift ? '_' : '-';
        case KeyEvent::Key::EQUAL: return shift ? '+' : '=';
        case KeyEvent::Key::LBRACKET: return shift ? '{' : '[';
        case KeyEvent::Key::RBRACKET: return shift ? '}' : ']';
        case KeyEvent::Key::BACKSLASH: return shift ? '|' : '\\';
        case KeyEvent::Key::SEMICOLON: return shift ? ':' : ';';
        case KeyEvent::Key::QUOTE: return shift ? '"' : '\'';
        case KeyEvent::Key::COMMA: return shift ? '<' : ',';
        case KeyEvent::Key::PERIOD: return shift ? '>' : '.';
        case KeyEvent::Key::SLASH: return shift ? '?' : '/';
        case KeyEvent::Key::GRAVE: return shift ? '~' : '`';
        default:
            return 0;
    }
}
