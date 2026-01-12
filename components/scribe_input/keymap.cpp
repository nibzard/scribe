#include "keymap.h"
#include <esp_log.h>

static const char* TAG = "SCRIBE_KEYMAP";

// Current keyboard layout (default US)
static KeyboardLayout current_layout = KeyboardLayout::US;

// Layout name mapping
static const char* layout_names[] = {
    "US",
    "UK",
    "DE",
    "FR"
};

const char* getLayoutName(KeyboardLayout layout) {
    int idx = static_cast<int>(layout);
    if (idx >= 0 && idx < 4) {
        return layout_names[idx];
    }
    return "US";
}

KeyboardLayout intToLayout(int value) {
    if (value >= 0 && value <= 3) {
        return static_cast<KeyboardLayout>(value);
    }
    return KeyboardLayout::US;
}

void setKeyboardLayout(KeyboardLayout layout) {
    current_layout = layout;
    ESP_LOGI(TAG, "Keyboard layout set to: %s", getLayoutName(layout));
}

KeyboardLayout getKeyboardLayout() {
    return current_layout;
}

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
};

KeyEvent::Key mapHIDUsageToKey(uint8_t usage) {
    if (usage >= sizeof(hid_usage_to_key) / sizeof(KeyEvent::Key)) {
        return KeyEvent::Key::UNKNOWN;
    }
    return hid_usage_to_key[usage];
}

// Character mapping structure for each key and layout
struct KeyMapping {
    KeyEvent::Key key;
    char normal;      // Unshifted character
    char shifted;     // Shifted character
};

// US Layout (QWERTY)
static const KeyMapping us_layout[] = {
    {KeyEvent::Key::A, 'a', 'A'}, {KeyEvent::Key::B, 'b', 'B'}, {KeyEvent::Key::C, 'c', 'C'},
    {KeyEvent::Key::D, 'd', 'D'}, {KeyEvent::Key::E, 'e', 'E'}, {KeyEvent::Key::F, 'f', 'F'},
    {KeyEvent::Key::G, 'g', 'G'}, {KeyEvent::Key::H, 'h', 'H'}, {KeyEvent::Key::I, 'i', 'I'},
    {KeyEvent::Key::J, 'j', 'J'}, {KeyEvent::Key::K, 'k', 'K'}, {KeyEvent::Key::L, 'l', 'L'},
    {KeyEvent::Key::M, 'm', 'M'}, {KeyEvent::Key::N, 'n', 'N'}, {KeyEvent::Key::O, 'o', 'O'},
    {KeyEvent::Key::P, 'p', 'P'}, {KeyEvent::Key::Q, 'q', 'Q'}, {KeyEvent::Key::R, 'r', 'R'},
    {KeyEvent::Key::S, 's', 'S'}, {KeyEvent::Key::T, 't', 'T'}, {KeyEvent::Key::U, 'u', 'U'},
    {KeyEvent::Key::V, 'v', 'V'}, {KeyEvent::Key::W, 'w', 'W'}, {KeyEvent::Key::X, 'x', 'X'},
    {KeyEvent::Key::Y, 'y', 'Y'}, {KeyEvent::Key::Z, 'z', 'Z'},
    {KeyEvent::Key::_0, '0', ')'}, {KeyEvent::Key::_1, '1', '!'}, {KeyEvent::Key::_2, '2', '@'},
    {KeyEvent::Key::_3, '3', '#'}, {KeyEvent::Key::_4, '4', '$'}, {KeyEvent::Key::_5, '5', '%'},
    {KeyEvent::Key::_6, '6', '^'}, {KeyEvent::Key::_7, '7', '&'}, {KeyEvent::Key::_8, '8', '*'},
    {KeyEvent::Key::_9, '9', '('},
    {KeyEvent::Key::SPACE, ' ', ' '},
    {KeyEvent::Key::MINUS, '-', '_'}, {KeyEvent::Key::EQUAL, '=', '+'},
    {KeyEvent::Key::LBRACKET, '[', '{'}, {KeyEvent::Key::RBRACKET, ']', '}'},
    {KeyEvent::Key::BACKSLASH, '\\', '|'}, {KeyEvent::Key::SEMICOLON, ';', ':'},
    {KeyEvent::Key::QUOTE, '\'', '"'}, {KeyEvent::Key::COMMA, ',', '<'},
    {KeyEvent::Key::PERIOD, '.', '>'}, {KeyEvent::Key::SLASH, '/', '?'},
    {KeyEvent::Key::GRAVE, '`', '~'},
};

// UK Layout (QWERTY with differences)
static const KeyMapping uk_layout[] = {
    {KeyEvent::Key::A, 'a', 'A'}, {KeyEvent::Key::B, 'b', 'B'}, {KeyEvent::Key::C, 'c', 'C'},
    {KeyEvent::Key::D, 'd', 'D'}, {KeyEvent::Key::E, 'e', 'E'}, {KeyEvent::Key::F, 'f', 'F'},
    {KeyEvent::Key::G, 'g', 'G'}, {KeyEvent::Key::H, 'h', 'H'}, {KeyEvent::Key::I, 'i', 'I'},
    {KeyEvent::Key::J, 'j', 'J'}, {KeyEvent::Key::K, 'k', 'K'}, {KeyEvent::Key::L, 'l', 'L'},
    {KeyEvent::Key::M, 'm', 'M'}, {KeyEvent::Key::N, 'n', 'N'}, {KeyEvent::Key::O, 'o', 'O'},
    {KeyEvent::Key::P, 'p', 'P'}, {KeyEvent::Key::Q, 'q', 'Q'}, {KeyEvent::Key::R, 'r', 'R'},
    {KeyEvent::Key::S, 's', 'S'}, {KeyEvent::Key::T, 't', 'T'}, {KeyEvent::Key::U, 'u', 'U'},
    {KeyEvent::Key::V, 'v', 'V'}, {KeyEvent::Key::W, 'w', 'W'}, {KeyEvent::Key::X, 'x', 'X'},
    {KeyEvent::Key::Y, 'y', 'Y'}, {KeyEvent::Key::Z, 'z', 'Z'},
    {KeyEvent::Key::_0, '0', ')'}, {KeyEvent::Key::_1, '1', '!'}, {KeyEvent::Key::_2, '2', '"'},
    {KeyEvent::Key::_3, '3', '£'}, {KeyEvent::Key::_4, '4', '$'}, {KeyEvent::Key::_5, '5', '%'},
    {KeyEvent::Key::_6, '6', '^'}, {KeyEvent::Key::_7, '7', '&'}, {KeyEvent::Key::_8, '8', '*'},
    {KeyEvent::Key::_9, '9', '('},
    {KeyEvent::Key::SPACE, ' ', ' '},
    {KeyEvent::Key::MINUS, '-', '_'}, {KeyEvent::Key::EQUAL, '=', '+'},
    {KeyEvent::Key::LBRACKET, '[', '{'}, {KeyEvent::Key::RBRACKET, ']', '}'},
    {KeyEvent::Key::BACKSLASH, '#', '~'},  // UK: # is Shift+3, ~ is Shift+#
    {KeyEvent::Key::SEMICOLON, ';', ':'},
    {KeyEvent::Key::QUOTE, '\'', '@'},  // UK: @ is on Shift+2
    {KeyEvent::Key::COMMA, ',', '<'},
    {KeyEvent::Key::PERIOD, '.', '>'}, {KeyEvent::Key::SLASH, '/', '?'},
    {KeyEvent::Key::GRAVE, '¬', '|'},  // UK: ¬ is unshifted, | is shifted
};

// German Layout (QWERTZ)
static const KeyMapping de_layout[] = {
    {KeyEvent::Key::A, 'a', 'A'}, {KeyEvent::Key::B, 'b', 'B'}, {KeyEvent::Key::C, 'c', 'C'},
    {KeyEvent::Key::D, 'd', 'D'}, {KeyEvent::Key::E, 'e', 'E'}, {KeyEvent::Key::F, 'f', 'F'},
    {KeyEvent::Key::G, 'g', 'G'}, {KeyEvent::Key::H, 'h', 'H'}, {KeyEvent::Key::I, 'i', 'I'},
    {KeyEvent::Key::J, 'j', 'J'}, {KeyEvent::Key::K, 'k', 'K'}, {KeyEvent::Key::L, 'l', 'L'},
    {KeyEvent::Key::M, 'm', 'M'}, {KeyEvent::Key::N, 'n', 'N'}, {KeyEvent::Key::O, 'o', 'O'},
    {KeyEvent::Key::P, 'p', 'P'}, {KeyEvent::Key::Q, 'q', 'Q'}, {KeyEvent::Key::R, 'r', 'R'},
    {KeyEvent::Key::S, 's', 'S'}, {KeyEvent::Key::T, 't', 'T'}, {KeyEvent::Key::U, 'u', 'U'},
    {KeyEvent::Key::V, 'v', 'V'}, {KeyEvent::Key::W, 'w', 'W'}, {KeyEvent::Key::X, 'x', 'X'},
    {KeyEvent::Key::Y, 'z', 'Z'},  // German: Z and Y are swapped
    {KeyEvent::Key::Z, 'y', 'Y'},
    {KeyEvent::Key::_0, '0', '='}, {KeyEvent::Key::_1, '1', '!'}, {KeyEvent::Key::_2, '2', '"'},
    {KeyEvent::Key::_3, '3', '§'}, {KeyEvent::Key::_4, '4', '$'}, {KeyEvent::Key::_5, '5', '%'},
    {KeyEvent::Key::_6, '6', '&'}, {KeyEvent::Key::_7, '7', '/'}, {KeyEvent::Key::_8, '8', '('},
    {KeyEvent::Key::_9, '9', ')'},
    {KeyEvent::Key::SPACE, ' ', ' '},
    {KeyEvent::Key::MINUS, '-', '?'},  // German: ? is on Shift-
    {KeyEvent::Key::EQUAL, '\'', '`'},  // German: ' is unshifted, ` is shifted (on = key)
    {KeyEvent::Key::LBRACKET, '[', '{'}, {KeyEvent::Key::RBRACKET, ']', '}'},
    {KeyEvent::Key::BACKSLASH, '#', '\''},  // German: # is unshifted on \\ key
    {KeyEvent::Key::SEMICOLON, ';', ':'},
    {KeyEvent::Key::QUOTE, '+', '*'},  // German: + is on ' key, * is shifted
    {KeyEvent::Key::COMMA, ',', ';'},  // German: ; is on , key
    {KeyEvent::Key::PERIOD, '.', ':'},  // German: : is on . key
    {KeyEvent::Key::SLASH, '-', '_'},  // German: - is on / key
    {KeyEvent::Key::GRAVE, '^', '°'},  // German: ^ is unshifted, ° is shifted
};

// French Layout (AZERTY)
static const KeyMapping fr_layout[] = {
    {KeyEvent::Key::A, 'a', 'A'},  // French: A is on QWERTY Q position
    {KeyEvent::Key::B, 'b', 'B'},
    {KeyEvent::Key::C, 'c', 'C'},
    {KeyEvent::Key::D, 'd', 'D'},
    {KeyEvent::Key::E, 'e', 'E'},
    {KeyEvent::Key::F, 'f', 'F'},
    {KeyEvent::Key::G, 'g', 'G'},
    {KeyEvent::Key::H, 'h', 'H'},
    {KeyEvent::Key::I, 'i', 'I'},
    {KeyEvent::Key::J, 'j', 'J'},
    {KeyEvent::Key::K, 'k', 'K'},
    {KeyEvent::Key::L, 'l', 'L'},
    {KeyEvent::Key::M, 'm', 'M'},
    {KeyEvent::Key::N, 'n', 'N'},
    {KeyEvent::Key::O, 'o', 'O'},
    {KeyEvent::Key::P, 'p', 'P'},
    {KeyEvent::Key::Q, 'z', 'Z'},  // French: Z is on QWERTY Q position
    {KeyEvent::Key::R, 'r', 'R'},
    {KeyEvent::Key::S, 's', 'S'},
    {KeyEvent::Key::T, 't', 'T'},
    {KeyEvent::Key::U, 'u', 'U'},
    {KeyEvent::Key::V, 'v', 'V'},
    {KeyEvent::Key::W, 'w', 'W'},  // French: W is on Z position
    {KeyEvent::Key::X, 'x', 'X'},  // French: X is on QWERTY W position (roughly)
    {KeyEvent::Key::Y, 'y', 'Y'},  // French: Y is on QWERTY Y position
    {KeyEvent::Key::Z, 'w', 'W'},  // French: W is on QWERTY Z position (roughly)
    {KeyEvent::Key::_0, '0', '0'}, {KeyEvent::Key::_1, '1', '1'}, {KeyEvent::Key::_2, '2', '2'},
    {KeyEvent::Key::_3, '3', '3'}, {KeyEvent::Key::_4, '4', '4'}, {KeyEvent::Key::_5, '5', '5'},
    {KeyEvent::Key::_6, '6', '6'}, {KeyEvent::Key::_7, '7', '7'}, {KeyEvent::Key::_8, '8', '8'},
    {KeyEvent::Key::_9, '9', '9'},
    {KeyEvent::Key::SPACE, ' ', ' '},
    {KeyEvent::Key::MINUS, '-', '+'},  // French: standard
    {KeyEvent::Key::EQUAL, '=', '+'},  // Note: French AZERTY differences handled below
    {KeyEvent::Key::LBRACKET, '^', '¨'},  // French: ^ unshifted, ¨ shifted
    {KeyEvent::Key::RBRACKET, '$', '£'},  // French: $ unshifted, £ shifted
    {KeyEvent::Key::BACKSLASH, '*', 'µ'},  // French: * unshifted, µ shifted
    {KeyEvent::Key::SEMICOLON, 'm', 'M'},  // French: M is on ; key position
    {KeyEvent::Key::QUOTE, 'ù', '%'},  // French: ù unshifted, % shifted
    {KeyEvent::Key::COMMA, ',', '?'},
    {KeyEvent::Key::PERIOD, '.', '.'},
    {KeyEvent::Key::SLASH, ':', '/'},
    {KeyEvent::Key::GRAVE, '²', '~'},  // French: ² unshifted, ~ shifted
};

// Helper: look up character in layout table
static char lookupChar(const KeyMapping* layout, size_t size, KeyEvent::Key key, bool shift) {
    for (size_t i = 0; i < size; i++) {
        if (layout[i].key == key) {
            return shift ? layout[i].shifted : layout[i].normal;
        }
    }
    return 0;
}

char mapKeyToChar(KeyEvent::Key key, bool shift) {
    switch (current_layout) {
        case KeyboardLayout::US:
            return lookupChar(us_layout, sizeof(us_layout)/sizeof(KeyMapping), key, shift);
        case KeyboardLayout::UK:
            return lookupChar(uk_layout, sizeof(uk_layout)/sizeof(KeyMapping), key, shift);
        case KeyboardLayout::DE:
            return lookupChar(de_layout, sizeof(de_layout)/sizeof(KeyMapping), key, shift);
        case KeyboardLayout::FR:
            return lookupChar(fr_layout, sizeof(fr_layout)/sizeof(KeyMapping), key, shift);
        default:
            return lookupChar(us_layout, sizeof(us_layout)/sizeof(KeyMapping), key, shift);
    }
}

std::string getKeyDisplayString(KeyEvent::Key key) {
    switch (key) {
        case KeyEvent::Key::SPACE: return "Space";
        case KeyEvent::Key::ENTER: return "Enter";
        case KeyEvent::Key::TAB: return "Tab";
        case KeyEvent::Key::BACKSPACE: return "Backspace";
        case KeyEvent::Key::DELETE: return "Delete";
        case KeyEvent::Key::ESC: return "Esc";
        case KeyEvent::Key::UP: return "↑";
        case KeyEvent::Key::DOWN: return "↓";
        case KeyEvent::Key::LEFT: return "←";
        case KeyEvent::Key::RIGHT: return "→";
        case KeyEvent::Key::HOME: return "Home";
        case KeyEvent::Key::END: return "End";
        case KeyEvent::Key::PAGE_UP: return "Page Up";
        case KeyEvent::Key::PAGE_DOWN: return "Page Down";
        default: {
            char c = mapKeyToChar(key, false);
            if (c > 0 && c >= 32 && c <= 126) {
                return std::string(1, c);
            }
            return "?";
        }
    }
}
