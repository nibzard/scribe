#include "piece_table.h"
#include <algorithm>

PieceTable::PieceTable() : total_length_(0) {
    original_buffer_ = std::make_shared<std::string>();
    add_buffer_ = std::make_shared<std::string>();
    // Initial state: empty document with single empty piece
    pieces_.push_back(Piece(Piece::Type::ORIGINAL, 0, 0, 0));
}

void PieceTable::load(const std::string& content) {
    original_buffer_ = std::make_shared<std::string>(content);
    add_buffer_ = std::make_shared<std::string>();
    pieces_.clear();
    if (content.empty()) {
        pieces_.push_back(Piece(Piece::Type::ORIGINAL, 0, 0, 0));
    } else {
        pieces_.push_back(Piece(Piece::Type::ORIGINAL, 0, 0, content.length()));
    }
    total_length_ = content.length();
}

void PieceTable::insert(size_t pos, const std::string& text) {
    if (text.empty()) return;
    if (pos > total_length_) pos = total_length_;

    // Find and split piece at insertion point
    PiecePos ppos = findPiece(pos);
    splitPiece(ppos.piece_index, ppos.offset_in_piece);

    // Add text to add buffer
    ensureUniqueAddBuffer();
    size_t add_start = add_buffer_->length();
    add_buffer_->append(text);

    // Insert new piece (before when inserting at piece start)
    Piece new_piece(Piece::Type::ADD, 0, add_start, text.length());
    size_t insert_index = ppos.piece_index + (ppos.offset_in_piece > 0 ? 1 : 0);
    pieces_.insert(pieces_.begin() + insert_index, new_piece);

    total_length_ += text.length();
}

void PieceTable::remove(size_t start, size_t end) {
    if (start >= end) return;
    if (end > total_length_) end = total_length_;
    if (start >= total_length_) return;

    // Find and split at both boundaries
    PiecePos start_pos = findPiece(start);
    splitPiece(start_pos.piece_index, start_pos.offset_in_piece);

    PiecePos end_pos = findPiece(end);
    splitPiece(end_pos.piece_index, end_pos.offset_in_piece);

    // Remove pieces between start and end
    size_t remove_start = start_pos.piece_index + (start_pos.offset_in_piece > 0 ? 1 : 0);
    size_t remove_end = end_pos.piece_index + (end_pos.offset_in_piece > 0 ? 1 : 0);
    if (remove_start < remove_end) {
        pieces_.erase(pieces_.begin() + remove_start, pieces_.begin() + remove_end);
    }

    // Recalculate length
    total_length_ = 0;
    for (const auto& p : pieces_) {
        total_length_ += p.length;
    }
    if (pieces_.empty()) {
        pieces_.push_back(Piece(Piece::Type::ORIGINAL, 0, 0, 0));
    }
}

std::string PieceTable::getText() const {
    return getTextRange(0, total_length_);
}

std::string PieceTable::getTextRange(size_t start, size_t end) const {
    if (end > total_length_) end = total_length_;
    if (start >= end) return "";

    std::string result;
    result.reserve(end - start);

    PiecePos start_pos = findPiece(start);
    PiecePos end_pos = findPiece(end);

    for (size_t i = start_pos.piece_index; i <= end_pos.piece_index; ++i) {
        const Piece& p = pieces_[i];
        const std::string& buffer = (p.type == Piece::Type::ORIGINAL) ? *original_buffer_ : *add_buffer_;

        size_t piece_start = (i == start_pos.piece_index) ? p.start + start_pos.offset_in_piece : p.start;
        size_t piece_end = (i == end_pos.piece_index) ? p.start + end_pos.offset_in_piece : p.start + p.length;

        if (piece_start < piece_end) {
            result.append(buffer.substr(piece_start, piece_end - piece_start));
        }
    }

    return result;
}

PieceTable::PiecePos PieceTable::findPiece(size_t pos) const {
    if (pieces_.empty()) {
        return {0, 0};
    }
    size_t accumulated = 0;
    for (size_t i = 0; i < pieces_.size(); ++i) {
        if (accumulated + pieces_[i].length >= pos) {
            return {i, pos - accumulated};
        }
        accumulated += pieces_[i].length;
    }
    return {pieces_.size() - 1, pieces_.back().length};
}

void PieceTable::splitPiece(size_t piece_index, size_t offset) {
    if (offset == 0 || offset >= pieces_[piece_index].length) return;

    Piece& p = pieces_[piece_index];
    Piece right(p.type, p.buffer_index, p.start + offset, p.length - offset);
    p.length = offset;

    pieces_.insert(pieces_.begin() + piece_index + 1, right);
}

void PieceTable::compact() {
    // Serialize current state to new original buffer
    std::string full_text = getText();

    original_buffer_ = std::make_shared<std::string>(std::move(full_text));
    add_buffer_ = std::make_shared<std::string>();
    pieces_.clear();
    pieces_.push_back(Piece(Piece::Type::ORIGINAL, 0, 0, original_buffer_->length()));
}

PieceTableSnapshot PieceTable::createSnapshot() const {
    PieceTableSnapshot snapshot;
    snapshot.original_buffer = original_buffer_;
    snapshot.add_buffer = add_buffer_;
    snapshot.pieces = pieces_;
    snapshot.total_length = total_length_;
    return snapshot;
}

void PieceTable::ensureUniqueAddBuffer() {
    if (!add_buffer_.unique()) {
        add_buffer_ = std::make_shared<std::string>(*add_buffer_);
    }
}
