#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// Piece table editor for efficient edits and snapshots
// Based on the piece table data structure used in VS Code, AbiWord, etc.

struct Piece {
    enum class Type { ORIGINAL, ADD };

    Type type;
    size_t buffer_index;  // Index into original or add buffer
    size_t start;         // Start offset in buffer
    size_t length;        // Length of this piece

    Piece(Type t, size_t buf_idx, size_t s, size_t l)
        : type(t), buffer_index(buf_idx), start(s), length(l) {}
};

struct PieceTableSnapshot {
    std::shared_ptr<const std::string> original_buffer;
    std::shared_ptr<const std::string> add_buffer;
    std::vector<Piece> pieces;
    size_t total_length = 0;
};

class PieceTable {
public:
    PieceTable();
    ~PieceTable() = default;

    // Load initial document (sets original buffer)
    void load(const std::string& content);

    // Insert text at position
    void insert(size_t pos, const std::string& text);

    // Delete range [start, end)
    void remove(size_t start, size_t end);

    // Get full document text
    std::string getText() const;

    // Get text range
    std::string getTextRange(size_t start, size_t end) const;

    // Get document length
    size_t length() const { return total_length_; }

    // Get piece list (for snapshots)
    const std::vector<Piece>& getPieces() const { return pieces_; }

    // Get buffer pointers (for snapshot serialization)
    const std::string& getOriginalBuffer() const { return *original_buffer_; }
    const std::string& getAddBuffer() const { return *add_buffer_; }

    // Snapshot for background serialization
    PieceTableSnapshot createSnapshot() const;

    // Optimization: rebuild from current text (creates new single piece)
    void compact();

private:
    // Original document buffer (immutable)
    std::shared_ptr<std::string> original_buffer_;

    // Add buffer for all insertions (append-only)
    std::shared_ptr<std::string> add_buffer_;

    // Piece list describing document structure
    std::vector<Piece> pieces_;

    // Cached total length
    size_t total_length_;

    // Find piece containing position
    struct PiecePos {
        size_t piece_index;
        size_t offset_in_piece;
    };
    PiecePos findPiece(size_t pos) const;

    // Split piece at position
    void splitPiece(size_t piece_index, size_t offset);

    // Ensure add buffer is not shared before mutation
    void ensureUniqueAddBuffer();
};
