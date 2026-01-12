#include "editor_core.h"
#include <algorithm>
#include <cctype>

// Standalone word count utility (can be used for stats display)
size_t countWords(const std::string& text) {
    if (text.empty()) return 0;

    size_t count = 0;
    bool in_word = false;

    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (in_word) {
                count++;
                in_word = false;
            }
        } else {
            in_word = true;
        }
    }

    if (in_word) count++;
    return count;
}

size_t countChars(const std::string& text) {
    return text.length();
}

size_t countLines(const std::string& text) {
    if (text.empty()) return 1;
    size_t lines = 1;
    for (char c : text) {
        if (c == '\n') lines++;
    }
    return lines;
}
