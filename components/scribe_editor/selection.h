#pragma once

#include <cstdint>
#include <cstddef>

// Selection range - lightweight struct for editor
struct Selection {
    size_t start = 0;
    size_t end = 0;

    bool isActive() const { return start != end; }
    bool isReversed() const { return start > end; }
    size_t length() const;
    size_t min() const;
    size_t max() const;

    void clear() { start = end = 0; }
    void set(size_t s, size_t e) { start = s; end = e; }
};
