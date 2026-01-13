#pragma once

#include <esp_err.h>

// Simple audio feedback system for UI interactions
// Uses tone generation for simple beep/click sounds

namespace AudioFeedback {

// Sound effect types
enum class SoundType {
    STARTUP,      // Startup chime
    SHUTDOWN,     // Shutdown confirmation
    CLICK,        // Button click feedback
    ERROR,        // Error notification
    SUCCESS,      // Success notification
    CONNECTING,   // WiFi connecting
    CONNECTED,    // WiFi connected
};

// Initialize audio feedback (codec, DAC, etc.)
esp_err_t init();

// Play a sound effect
esp_err_t play(SoundType type);

// Check if audio is available
bool isAvailable();

// Enable/disable audio feedback
void setEnabled(bool enabled);
bool isEnabled();

} // namespace AudioFeedback
