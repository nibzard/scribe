#pragma once

#include "key_event.h"
#include <string>

// Keyboard layout enumeration
enum class KeyboardLayout : int {
    US = 0,
    UK = 1,
    DE = 2,  // German
    FR = 3,  // French AZERTY
};

// Get layout name for display
const char* getLayoutName(KeyboardLayout layout);

// Get layout enum from integer
KeyboardLayout intToLayout(int value);

// Set current keyboard layout
void setKeyboardLayout(KeyboardLayout layout);

// Get current keyboard layout
KeyboardLayout getKeyboardLayout();

// USB HID Usage ID to KeyEvent::Key mapping
KeyEvent::Key mapHIDUsageToKey(uint8_t usage);

// Map key to character for current layout and shift state
char mapKeyToChar(KeyEvent::Key key, bool shift);

// Get display string for a key (useful for help screens)
std::string getKeyDisplayString(KeyEvent::Key key);
