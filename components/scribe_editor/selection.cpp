#include "selection.h"
#include <algorithm>

size_t Selection::length() const {
    if (start <= end) {
        return end - start;
    }
    return start - end;
}

size_t Selection::min() const {
    return std::min(start, end);
}

size_t Selection::max() const {
    return std::max(start, end);
}
