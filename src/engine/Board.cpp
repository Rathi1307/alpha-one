#include "Board.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>

namespace alphaone {

bool Board::tables_initialized_ = false;
uint64_t Board::knightAttacks_[64]{};
uint64_t Board::kingAttacks_[64]{};
uint64_t Board::diagonalRays_[64][4]{};
uint64_t Board::cardinalRays_[64][4]{};

void Board::initAttackTables() noexcept {
    if (tables_initialized_) return;
    Zobrist::init();

    for (int x1 = 0; x1 < 8; ++x1) {
        for (int y1 = 0; y1 < 8; ++y1) {
            uint8_t sq = static_cast<uint8_t>(x1 + 8 * y1);

            // Knight attacks
            knightAttacks_[sq] = 0;
            if (x1 < 7 && y1 < 6) knightAttacks_[sq] |= uint64_t(1) << (x1 + 1 + 8 * (y1 + 2));
            if (x1 < 6 && y1 < 7) knightAttacks_[sq] |= uint64_t(1) << (x1 + 2 + 8 * (y1 + 1));
            if (x1 < 7 && y1 > 1) knightAttacks_[sq] |= uint64_t(1) << (x1 + 1 + 8 * (y1 - 2));
            if (x1 > 1 && y1 < 7) knightAttacks_[sq] |= uint64_t(1) << (x1 - 2 + 8 * (y1 + 1));
            if (x1 < 6 && y1 > 0) knightAttacks_[sq] |= uint64_t(1) << (x1 + 2 + 8 * (y1 - 1));
            if (x1 > 0 && y1 < 6) knightAttacks_[sq] |= uint64_t(1) << (x1 - 1 + 8 * (y1 + 2));
            if (x1 > 0 && y1 > 1) knightAttacks_[sq] |= uint64_t(1) << (x1 - 1 + 8 * (y1 - 2));
            if (x1 > 1 && y1 > 0) knightAttacks_[sq] |= uint64_t(1) << (x1 - 2 + 8 * (y1 - 1));

            // Diagonal attack rays
            diagonalRays_[sq][0] = 0;
            for (int x2 = x1 + 1, y2 = y1 + 1; x2 < 8 && y2 < 8; ++x2, ++y2) diagonalRays_[sq][0] |= uint64_t(1) << (x2 + 8 * y2);
            diagonalRays_[sq][1] = 0;
            for (int x2 = x1 + 1, y2 = y1 - 1; x2 < 8 && y2 >= 0; ++x2, --y2) diagonalRays_[sq][1] |= uint64_t(1) << (x2 + 8 * y2);
            diagonalRays_[sq][2] = 0;
            for (int x2 = x1 - 1, y2 = y1 - 1; x2 >= 0 && y2 >= 0; --x2, --y2) diagonalRays_[sq][2] |= uint64_t(1) << (x2 + 8 * y2);
            diagonalRays_[sq][3] = 0;
            for (int x2 = x1 - 1, y2 = y1 + 1; x2 >= 0 && y2 < 8; --x2, ++y2) diagonalRays_[sq][3] |= uint64_t(1) << (x2 + 8 * y2);

            // Cardinal attack rays
            cardinalRays_[sq][0] = 0;
            for (int x2 = x1 + 1; x2 < 8; ++x2) cardinalRays_[sq][0] |= uint64_t(1) << (x2 + 8 * y1);
            cardinalRays_[sq][1] = 0;
            for (int x2 = x1 - 1; x2 >= 0; --x2) cardinalRays_[sq][1] |= uint64_t(1) << (x2 + 8 * y1);
            cardinalRays_[sq][2] = 0;
            for (int y2 = y1 + 1; y2 < 8; ++y2) cardinalRays_[sq][2] |= uint64_t(1) << (x1 + 8 * y2);
            cardinalRays_[sq][3] = 0;
            for (int y2 = y1 - 1; y2 >= 0; --y2) cardinalRays_[sq][3] |= uint64_t(1) << (x1 + 8 * y2);

            // King attacks
            kingAttacks_[sq] = 0;
            if (x1 < 7 && y1 < 7) kingAttacks_[sq] |= uint64_t(1) << (x1 + 1 + 8 * (y1 + 1));
            if (x1 < 7) kingAttacks_[sq] |= uint64_t(1) << (x1 + 1 + 8 * y1);
            if (x1 < 7 && y1 > 0) kingAttacks_[sq] |= uint64_t(1) << (x1 + 1 + 8 * (y1 - 1));
            if (y1 < 7) kingAttacks_[sq] |= uint64_t(1) << (x1 + 8 * (y1 + 1));
            if (y1 > 0) kingAttacks_[sq] |= uint64_t(1) << (x1 + 8 * (y1 - 1));
            if (x1 > 0 && y1 < 7) kingAttacks_[sq] |= uint64_t(1) << (x1 - 1 + 8 * (y1 + 1));
            if (x1 > 0) kingAttacks_[sq] |= uint64_t(1) << (x1 - 1 + 8 * y1);
            if (x1 > 0 && y1 > 0) kingAttacks_[sq] |= uint64_t(1) << (x1 - 1 + 8 * (y1 - 1));
        }
    }

    tables_initialized_ = true;
}

Board::Board() {
    initAttackTables();
    stateStack_ = new BoardState[MAX_STACK];
    newGame();
}

Board::Board(std::string_view fen) {
    initAttackTables();
    stateStack_ = new BoardState[MAX_STACK];
    setPosition(fen);
}

Board::~Board() {
    delete[] stateStack_;
}

Board::Board(const Board& other) {
    initAttackTables();
    stateStack_ = new BoardState[MAX_STACK];
    stackIndex_ = other.stackIndex_;
    for (int i = 0; i <= stackIndex_; ++i) {
        stateStack_[i] = other.stateStack_[i];
    }
}

Board& Board::operator=(const Board& other) {
    if (this != &other) {
        stackIndex_ = other.stackIndex_;
        for (int i = 0; i <= stackIndex_; ++i) {
            stateStack_[i] = other.stateStack_[i];
        }
    }
    return *this;
}

uint64_t Board::computeHash() const noexcept {
    uint64_t h = 0;
    const auto& s = stateStack_[stackIndex_];

    for (int p = 0; p < 12; ++p) {
        uint64_t bb = s.bitboards[p];
        while (bb) {
            h ^= Zobrist::pieceSquare(static_cast<uint8_t>(p), lsbIndex(bb));
            bb &= bb - 1;
        }
    }

    if (s.passantTarget) {
        h ^= Zobrist::enPassant(lsbIndex(s.passantTarget));
    }
    if (s.turn == WHITE) h ^= Zobrist::sideToMove();
    if (s.WKC) h ^= Zobrist::whiteKingCastle();
    if (s.WQC) h ^= Zobrist::whiteQueenCastle();
    if (s.BKC) h ^= Zobrist::blackKingCastle();
    if (s.BQC) h ^= Zobrist::blackQueenCastle();

    return h;
}

uint8_t Board::getPiece(uint8_t square) const noexcept {
    if (square >= 64) return EMPTY;
    uint64_t mask = uint64_t(1) << square;
    const auto& s = stateStack_[stackIndex_];
    for (int i = 0; i < 12; ++i) {
        if (s.bitboards[i] & mask) return static_cast<uint8_t>(i);
    }
    return EMPTY;
}

void Board::setPiece(uint8_t piece, uint8_t square) noexcept {
    if (square >= 64) return;
    uint64_t mask = uint64_t(1) << square;
    auto& s = stateStack_[stackIndex_];
    for (int i = 0; i < 12; ++i) {
        if (i == piece) s.bitboards[i] |= mask;
        else s.bitboards[i] &= ~mask;
    }
}

void Board::newGame() {
    stackIndex_ = 0;
    auto& s0 = stateStack_[0];

    s0.bitboards[WHITE_PAWN]   = 0x00ff000000000000ULL;
    s0.bitboards[WHITE_KNIGHT] = 0x4200000000000000ULL;
    s0.bitboards[WHITE_BISHOP] = 0x2400000000000000ULL;
    s0.bitboards[WHITE_ROOK]   = 0x8100000000000000ULL;
    s0.bitboards[WHITE_QUEEN]  = 0x0800000000000000ULL;
    s0.bitboards[WHITE_KING]   = 0x1000000000000000ULL;

    s0.bitboards[BLACK_PAWN]   = 0x000000000000ff00ULL;
    s0.bitboards[BLACK_KNIGHT] = 0x0000000000000042ULL;
    s0.bitboards[BLACK_BISHOP] = 0x0000000000000024ULL;
    s0.bitboards[BLACK_ROOK]   = 0x0000000000000081ULL;
    s0.bitboards[BLACK_QUEEN]  = 0x0000000000000008ULL;
    s0.bitboards[BLACK_KING]   = 0x0000000000000010ULL;

    s0.WKC = true;
    s0.WQC = true;
    s0.BKC = true;
    s0.BQC = true;
    s0.passantTarget = 0;
    s0.turn = WHITE;
    s0.halfMoves = 0;
    s0.hash = computeHash();
}

bool Board::setPosition(std::string_view fen) {
    stackIndex_ = 0;
    auto& s = stateStack_[0];

    for (int i = 0; i < 12; ++i) s.bitboards[i] = 0;
    s.WKC = false;
    s.WQC = false;
    s.BKC = false;
    s.BQC = false;
    s.passantTarget = 0;
    s.turn = WHITE;
    s.halfMoves = 0;

    std::istringstream ss{std::string(fen)};
    std::string boardStr, turnStr, castlingStr, passantStr, halfStr, fullStr;

    if (!(ss >> boardStr >> turnStr)) return false;
    if (!(ss >> castlingStr)) castlingStr = "-";
    if (!(ss >> passantStr)) passantStr = "-";
    if (!(ss >> halfStr)) halfStr = "0";
    if (!(ss >> fullStr)) fullStr = "1";

    int x = 0, y = 0;
    for (char c : boardStr) {
        uint8_t sq = static_cast<uint8_t>(x + 8 * y);
        if (c == '/') {
            x = 0;
            ++y;
        } else if (c >= '1' && c <= '8') {
            x += (c - '0');
        } else {
            switch (c) {
                case 'P': setPiece(WHITE_PAWN, sq); break;
                case 'N': setPiece(WHITE_KNIGHT, sq); break;
                case 'B': setPiece(WHITE_BISHOP, sq); break;
                case 'R': setPiece(WHITE_ROOK, sq); break;
                case 'Q': setPiece(WHITE_QUEEN, sq); break;
                case 'K': setPiece(WHITE_KING, sq); break;
                case 'p': setPiece(BLACK_PAWN, sq); break;
                case 'n': setPiece(BLACK_KNIGHT, sq); break;
                case 'b': setPiece(BLACK_BISHOP, sq); break;
                case 'r': setPiece(BLACK_ROOK, sq); break;
                case 'q': setPiece(BLACK_QUEEN, sq); break;
                case 'k': setPiece(BLACK_KING, sq); break;
                default: break;
            }
            ++x;
        }
    }

    s.turn = (turnStr == "b" || turnStr == "B") ? BLACK : WHITE;

    for (char c : castlingStr) {
        if (c == 'K') s.WKC = true;
        else if (c == 'Q') s.WQC = true;
        else if (c == 'k') s.BKC = true;
        else if (c == 'q') s.BQC = true;
    }

    if (passantStr.size() >= 2 && passantStr != "-") {
        int sq = algebraicToSquare(passantStr);
        if (sq >= 0 && sq < 64) {
            s.passantTarget = uint64_t(1) << sq;
        }
    }

    try {
        s.halfMoves = static_cast<uint8_t>(std::stoi(halfStr));
    } catch (...) {
        s.halfMoves = 0;
    }

    s.hash = computeHash();
    return true;
}

std::string Board::toFen() const {
    std::string fen;
    const auto& s = stateStack_[stackIndex_];

    for (int y = 0; y < 8; ++y) {
        int emptyCount = 0;
        for (int x = 0; x < 8; ++x) {
            uint8_t sq = static_cast<uint8_t>(x + 8 * y);
            uint8_t p = getPiece(sq);
            if (p == EMPTY) {
                ++emptyCount;
            } else {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                static const char pieceChars[] = "PNBRQKpnbrqk";
                fen += pieceChars[p];
            }
        }
        if (emptyCount > 0) fen += std::to_string(emptyCount);
        if (y < 7) fen += '/';
    }

    fen += (s.turn == WHITE ? " w " : " b ");

    std::string castling;
    if (s.WKC) castling += 'K';
    if (s.WQC) castling += 'Q';
    if (s.BKC) castling += 'k';
    if (s.BQC) castling += 'q';
    if (castling.empty()) castling = "-";
    fen += castling + " ";

    if (s.passantTarget) {
        fen += squareToAlgebraic(lsbIndex(s.passantTarget));
    } else {
        fen += "-";
    }

    fen += " " + std::to_string(s.halfMoves);
    fen += " " + std::to_string(1 + stackIndex_ / 2);

    return fen;
}

void Board::move(const Move& m) noexcept {
    if (stackIndex_ + 1 >= MAX_STACK) return;

    // Clean sequence point copy
    stateStack_[stackIndex_ + 1] = stateStack_[stackIndex_];
    ++stackIndex_;

    auto& cur = stateStack_[stackIndex_];
    ++cur.halfMoves;

    uint64_t fromBoard = uint64_t(1) << m.from;
    uint64_t toBoard = uint64_t(1) << m.to;
    uint64_t moveBoard = fromBoard | toBoard;

    uint8_t movedPiece = EMPTY;
    int startIdx = (cur.turn == WHITE) ? 0 : 6;
    int endIdx = (cur.turn == WHITE) ? 6 : 12;

    for (int i = startIdx; i < endIdx; ++i) {
        if (cur.bitboards[i] & fromBoard) {
            cur.bitboards[i] ^= moveBoard;
            cur.hash ^= Zobrist::pieceSquare(static_cast<uint8_t>(i), m.from) ^
                        Zobrist::pieceSquare(static_cast<uint8_t>(i), m.to);
            movedPiece = static_cast<uint8_t>(i);
            break;
        }
    }

    if (movedPiece == WHITE_PAWN || movedPiece == BLACK_PAWN) cur.halfMoves = 0;

    // Normal capture
    int oppStart = (cur.turn == WHITE) ? 6 : 0;
    int oppEnd = (cur.turn == WHITE) ? 12 : 6;
    for (int i = oppStart; i < oppEnd; ++i) {
        if (cur.bitboards[i] & toBoard) {
            cur.bitboards[i] ^= toBoard;
            cur.hash ^= Zobrist::pieceSquare(static_cast<uint8_t>(i), m.to);
            cur.halfMoves = 0;
            break;
        }
    }

    // En-passant capture
    if (toBoard == cur.passantTarget) {
        if (movedPiece == WHITE_PAWN) {
            cur.bitboards[BLACK_PAWN] ^= (cur.passantTarget << 8);
            cur.hash ^= Zobrist::pieceSquare(BLACK_PAWN, m.to + 8);
        } else if (movedPiece == BLACK_PAWN) {
            cur.bitboards[WHITE_PAWN] ^= (cur.passantTarget >> 8);
            cur.hash ^= Zobrist::pieceSquare(WHITE_PAWN, m.to - 8);
        }
        cur.halfMoves = 0;
    }

    // Remove old en-passant hash
    if (cur.passantTarget) {
        cur.hash ^= Zobrist::enPassant(lsbIndex(cur.passantTarget));
    }

    // Set new en-passant target for double pawn push
    if ((movedPiece == WHITE_PAWN || movedPiece == BLACK_PAWN) && std::abs(static_cast<int>(m.from) - static_cast<int>(m.to)) == 16) {
        cur.passantTarget = uint64_t(1) << ((m.from + m.to) / 2);
        cur.hash ^= Zobrist::enPassant(lsbIndex(cur.passantTarget));
    } else {
        cur.passantTarget = 0;
    }

    // Castling move execution
    if (movedPiece == WHITE_KING) {
        if (moveBoard == 0x5000000000000000ULL) { // White King-side (e1g1, 60->62)
            cur.bitboards[WHITE_ROOK] ^= 0xa000000000000000ULL;
            cur.hash ^= Zobrist::pieceSquare(WHITE_ROOK, 63) ^ Zobrist::pieceSquare(WHITE_ROOK, 61);
        } else if (moveBoard == 0x1400000000000000ULL) { // White Queen-side (e1c1, 60->58)
            cur.bitboards[WHITE_ROOK] ^= 0x0900000000000000ULL;
            cur.hash ^= Zobrist::pieceSquare(WHITE_ROOK, 56) ^ Zobrist::pieceSquare(WHITE_ROOK, 59);
        }
    } else if (movedPiece == BLACK_KING) {
        if (moveBoard == 0x0000000000000050ULL) { // Black King-side (e8g8, 4->6)
            cur.bitboards[BLACK_ROOK] ^= 0x00000000000000a0ULL;
            cur.hash ^= Zobrist::pieceSquare(BLACK_ROOK, 7) ^ Zobrist::pieceSquare(BLACK_ROOK, 5);
        } else if (moveBoard == 0x0000000000000014ULL) { // Black Queen-side (e8c8, 4->2)
            cur.bitboards[BLACK_ROOK] ^= 0x0000000000000009ULL;
            cur.hash ^= Zobrist::pieceSquare(BLACK_ROOK, 0) ^ Zobrist::pieceSquare(BLACK_ROOK, 3);
        }
    }

    // Update castling rights bitmasks
    if ((moveBoard & 0x9000000000000000ULL) && cur.WKC) {
        cur.WKC = false;
        cur.hash ^= Zobrist::whiteKingCastle();
    }
    if ((moveBoard & 0x1100000000000000ULL) && cur.WQC) {
        cur.WQC = false;
        cur.hash ^= Zobrist::whiteQueenCastle();
    }
    if ((moveBoard & 0x0000000000000090ULL) && cur.BKC) {
        cur.BKC = false;
        cur.hash ^= Zobrist::blackKingCastle();
    }
    if ((moveBoard & 0x0000000000000011ULL) && cur.BQC) {
        cur.BQC = false;
        cur.hash ^= Zobrist::blackQueenCastle();
    }

    // Pawn Promotion
    if (m.promotion != EMPTY) {
        uint8_t pawnPiece = (cur.turn == WHITE) ? WHITE_PAWN : BLACK_PAWN;
        cur.bitboards[pawnPiece] ^= toBoard;
        cur.hash ^= Zobrist::pieceSquare(pawnPiece, m.to);

        cur.bitboards[m.promotion] ^= toBoard;
        cur.hash ^= Zobrist::pieceSquare(m.promotion, m.to);
    }

    // Switch turns
    cur.turn = (cur.turn == WHITE) ? BLACK : WHITE;
    cur.hash ^= Zobrist::sideToMove();
}

void Board::undo() noexcept {
    if (stackIndex_ > 0) {
        --stackIndex_;
    }
}

void Board::pseudoMoves(Move* moves, int& numMoves) const noexcept {
    numMoves = 0;
    const auto& s = stateStack_[stackIndex_];

    uint64_t whiteBoard = 0, blackBoard = 0;
    for (int i = 0; i < 6; ++i) whiteBoard |= s.bitboards[i];
    for (int i = 6; i < 12; ++i) blackBoard |= s.bitboards[i];
    uint64_t board = whiteBoard | blackBoard;

    if (s.turn == WHITE) {
        // Single pawn push
        uint64_t bb = (s.bitboards[WHITE_PAWN] >> 8) & ~board;
        uint64_t doubles = ((bb & 0x0000ff0000000000ULL) >> 8) & ~board;
        uint8_t square;

        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint8_t origin = square + 8;
            if (square < 8) {
                moves[numMoves++] = Move{origin, square, WHITE_QUEEN};
                moves[numMoves++] = Move{origin, square, WHITE_ROOK};
                moves[numMoves++] = Move{origin, square, WHITE_BISHOP};
                moves[numMoves++] = Move{origin, square, WHITE_KNIGHT};
            } else {
                moves[numMoves++] = Move{origin, square, EMPTY};
            }
        }

        // Double pawn push
        while (doubles) {
            square = lsbIndex(doubles);
            doubles &= doubles - 1;
            uint8_t origin = square + 16;
            moves[numMoves++] = Move{origin, square, EMPTY};
        }

        // Pawn captures right
        bb = ((s.bitboards[WHITE_PAWN] & 0xfefefefefefefefeULL) >> 9) & (blackBoard | s.passantTarget);
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint8_t origin = square + 9;
            if (square < 8) {
                moves[numMoves++] = Move{origin, square, WHITE_QUEEN};
                moves[numMoves++] = Move{origin, square, WHITE_ROOK};
                moves[numMoves++] = Move{origin, square, WHITE_BISHOP};
                moves[numMoves++] = Move{origin, square, WHITE_KNIGHT};
            } else {
                moves[numMoves++] = Move{origin, square, EMPTY};
            }
        }

        // Pawn captures left
        bb = ((s.bitboards[WHITE_PAWN] & 0x7f7f7f7f7f7f7f7fULL) >> 7) & (blackBoard | s.passantTarget);
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint8_t origin = square + 7;
            if (square < 8) {
                moves[numMoves++] = Move{origin, square, WHITE_QUEEN};
                moves[numMoves++] = Move{origin, square, WHITE_ROOK};
                moves[numMoves++] = Move{origin, square, WHITE_BISHOP};
                moves[numMoves++] = Move{origin, square, WHITE_KNIGHT};
            } else {
                moves[numMoves++] = Move{origin, square, EMPTY};
            }
        }

        // Knights
        bb = s.bitboards[WHITE_KNIGHT];
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint64_t knightMoves = knightAttacks_[square] & ~whiteBoard;
            while (knightMoves) {
                uint8_t target = lsbIndex(knightMoves);
                knightMoves &= knightMoves - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
        }

        // Bishops & Queens (Diagonal)
        bb = s.bitboards[WHITE_BISHOP] | s.bitboards[WHITE_QUEEN];
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint64_t diagMoves = 0;
            for (int i = 0; i < 4; ++i) {
                diagMoves |= diagonalRays_[square][i];
                uint64_t blockers = board & diagonalRays_[square][i];
                if (blockers) {
                    uint8_t blockerIndex = (i == 0 || i == 3) ? lsbIndex(blockers) : msbIndex(blockers);
                    diagMoves &= ~diagonalRays_[blockerIndex][i];
                }
            }
            diagMoves &= ~whiteBoard;
            while (diagMoves) {
                uint8_t target = lsbIndex(diagMoves);
                diagMoves &= diagMoves - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
        }

        // Rooks & Queens (Cardinal)
        bb = s.bitboards[WHITE_ROOK] | s.bitboards[WHITE_QUEEN];
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint64_t cardMoves = 0;
            for (int i = 0; i < 4; ++i) {
                cardMoves |= cardinalRays_[square][i];
                uint64_t blockers = board & cardinalRays_[square][i];
                if (blockers) {
                    uint8_t blockerIndex = (i == 0 || i == 2) ? lsbIndex(blockers) : msbIndex(blockers);
                    cardMoves &= ~cardinalRays_[blockerIndex][i];
                }
            }
            cardMoves &= ~whiteBoard;
            while (cardMoves) {
                uint8_t target = lsbIndex(cardMoves);
                cardMoves &= cardMoves - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
        }

        // King moves
        if (s.bitboards[WHITE_KING]) {
            square = lsbIndex(s.bitboards[WHITE_KING]);
            bb = kingAttacks_[square] & ~whiteBoard;
            while (bb) {
                uint8_t target = lsbIndex(bb);
                bb &= bb - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
            // Castling
            if (s.WKC && !(board & 0x6000000000000000ULL) && !isAttacked(60, BLACK) && !isAttacked(61, BLACK)) {
                moves[numMoves++] = Move{60, 62, EMPTY};
            }
            if (s.WQC && !(board & 0x0e00000000000000ULL) && !isAttacked(60, BLACK) && !isAttacked(59, BLACK)) {
                moves[numMoves++] = Move{60, 58, EMPTY};
            }
        }
    } else {
        // Black Single pawn push
        uint64_t bb = (s.bitboards[BLACK_PAWN] << 8) & ~board;
        uint64_t doubles = ((bb & 0x0000000000ff0000ULL) << 8) & ~board;
        uint8_t square;

        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint8_t origin = square - 8;
            if (square >= 56) {
                moves[numMoves++] = Move{origin, square, BLACK_QUEEN};
                moves[numMoves++] = Move{origin, square, BLACK_ROOK};
                moves[numMoves++] = Move{origin, square, BLACK_BISHOP};
                moves[numMoves++] = Move{origin, square, BLACK_KNIGHT};
            } else {
                moves[numMoves++] = Move{origin, square, EMPTY};
            }
        }

        // Black Double pawn push
        while (doubles) {
            square = lsbIndex(doubles);
            doubles &= doubles - 1;
            uint8_t origin = square - 16;
            moves[numMoves++] = Move{origin, square, EMPTY};
        }

        // Black Pawn captures right
        bb = ((s.bitboards[BLACK_PAWN] & 0xfefefefefefefefeULL) << 7) & (whiteBoard | s.passantTarget);
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint8_t origin = square - 7;
            if (square >= 56) {
                moves[numMoves++] = Move{origin, square, BLACK_QUEEN};
                moves[numMoves++] = Move{origin, square, BLACK_ROOK};
                moves[numMoves++] = Move{origin, square, BLACK_BISHOP};
                moves[numMoves++] = Move{origin, square, BLACK_KNIGHT};
            } else {
                moves[numMoves++] = Move{origin, square, EMPTY};
            }
        }

        // Black Pawn captures left
        bb = ((s.bitboards[BLACK_PAWN] & 0x7f7f7f7f7f7f7f7fULL) << 9) & (whiteBoard | s.passantTarget);
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint8_t origin = square - 9;
            if (square >= 56) {
                moves[numMoves++] = Move{origin, square, BLACK_QUEEN};
                moves[numMoves++] = Move{origin, square, BLACK_ROOK};
                moves[numMoves++] = Move{origin, square, BLACK_BISHOP};
                moves[numMoves++] = Move{origin, square, BLACK_KNIGHT};
            } else {
                moves[numMoves++] = Move{origin, square, EMPTY};
            }
        }

        // Knights
        bb = s.bitboards[BLACK_KNIGHT];
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint64_t knightMoves = knightAttacks_[square] & ~blackBoard;
            while (knightMoves) {
                uint8_t target = lsbIndex(knightMoves);
                knightMoves &= knightMoves - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
        }

        // Bishops & Queens (Diagonal)
        bb = s.bitboards[BLACK_BISHOP] | s.bitboards[BLACK_QUEEN];
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint64_t diagMoves = 0;
            for (int i = 0; i < 4; ++i) {
                diagMoves |= diagonalRays_[square][i];
                uint64_t blockers = board & diagonalRays_[square][i];
                if (blockers) {
                    uint8_t blockerIndex = (i == 0 || i == 3) ? lsbIndex(blockers) : msbIndex(blockers);
                    diagMoves &= ~diagonalRays_[blockerIndex][i];
                }
            }
            diagMoves &= ~blackBoard;
            while (diagMoves) {
                uint8_t target = lsbIndex(diagMoves);
                diagMoves &= diagMoves - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
        }

        // Rooks & Queens (Cardinal)
        bb = s.bitboards[BLACK_ROOK] | s.bitboards[BLACK_QUEEN];
        while (bb) {
            square = lsbIndex(bb);
            bb &= bb - 1;
            uint64_t cardMoves = 0;
            for (int i = 0; i < 4; ++i) {
                cardMoves |= cardinalRays_[square][i];
                uint64_t blockers = board & cardinalRays_[square][i];
                if (blockers) {
                    uint8_t blockerIndex = (i == 0 || i == 2) ? lsbIndex(blockers) : msbIndex(blockers);
                    cardMoves &= ~cardinalRays_[blockerIndex][i];
                }
            }
            cardMoves &= ~blackBoard;
            while (cardMoves) {
                uint8_t target = lsbIndex(cardMoves);
                cardMoves &= cardMoves - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
        }

        // King moves
        if (s.bitboards[BLACK_KING]) {
            square = lsbIndex(s.bitboards[BLACK_KING]);
            bb = kingAttacks_[square] & ~blackBoard;
            while (bb) {
                uint8_t target = lsbIndex(bb);
                bb &= bb - 1;
                moves[numMoves++] = Move{square, target, EMPTY};
            }
            // Castling
            if (s.BKC && !(board & 0x0000000000000060ULL) && !isAttacked(4, WHITE) && !isAttacked(5, WHITE)) {
                moves[numMoves++] = Move{4, 6, EMPTY};
            }
            if (s.BQC && !(board & 0x000000000000000eULL) && !isAttacked(4, WHITE) && !isAttacked(3, WHITE)) {
                moves[numMoves++] = Move{4, 2, EMPTY};
            }
        }
    }
}

bool Board::isAttacked(uint8_t square, uint8_t color) const noexcept {
    if (square >= 64) return false;
    const auto& s = stateStack_[stackIndex_];

    uint64_t board = 0;
    for (int i = 0; i < 12; ++i) board |= s.bitboards[i];

    if (color == BLACK) {
        // Cardinal
        uint64_t attackers = s.bitboards[BLACK_QUEEN] | s.bitboards[BLACK_ROOK];
        for (int i = 0; i < 4; ++i) {
            uint64_t blockers = cardinalRays_[square][i] & board;
            if (blockers & attackers) {
                uint8_t blockerIndex = (i == 0 || i == 2) ? lsbIndex(blockers) : msbIndex(blockers);
                if (attackers & (uint64_t(1) << blockerIndex)) return true;
            }
        }

        // Diagonal
        attackers = s.bitboards[BLACK_QUEEN] | s.bitboards[BLACK_BISHOP];
        for (int i = 0; i < 4; ++i) {
            uint64_t blockers = diagonalRays_[square][i] & board;
            if (blockers & attackers) {
                uint8_t blockerIndex = (i == 0 || i == 3) ? lsbIndex(blockers) : msbIndex(blockers);
                if (attackers & (uint64_t(1) << blockerIndex)) return true;
            }
        }

        // Knight
        if (knightAttacks_[square] & s.bitboards[BLACK_KNIGHT]) return true;

        // Pawn
        if ((((s.bitboards[BLACK_PAWN] & 0xfefefefefefefefeULL) << 7) |
             ((s.bitboards[BLACK_PAWN] & 0x7f7f7f7f7f7f7f7fULL) << 9)) & (uint64_t(1) << square)) return true;

        // King
        if (kingAttacks_[square] & s.bitboards[BLACK_KING]) return true;
    } else if (color == WHITE) {
        // Cardinal
        uint64_t attackers = s.bitboards[WHITE_QUEEN] | s.bitboards[WHITE_ROOK];
        for (int i = 0; i < 4; ++i) {
            uint64_t blockers = cardinalRays_[square][i] & board;
            if (blockers & attackers) {
                uint8_t blockerIndex = (i == 0 || i == 2) ? lsbIndex(blockers) : msbIndex(blockers);
                if (attackers & (uint64_t(1) << blockerIndex)) return true;
            }
        }

        // Diagonal
        attackers = s.bitboards[WHITE_QUEEN] | s.bitboards[WHITE_BISHOP];
        for (int i = 0; i < 4; ++i) {
            uint64_t blockers = diagonalRays_[square][i] & board;
            if (blockers & attackers) {
                uint8_t blockerIndex = (i == 0 || i == 3) ? lsbIndex(blockers) : msbIndex(blockers);
                if (attackers & (uint64_t(1) << blockerIndex)) return true;
            }
        }

        // Knight
        if (knightAttacks_[square] & s.bitboards[WHITE_KNIGHT]) return true;

        // Pawn
        if ((((s.bitboards[WHITE_PAWN] & 0xfefefefefefefefeULL) >> 9) |
             ((s.bitboards[WHITE_PAWN] & 0x7f7f7f7f7f7f7f7fULL) >> 7)) & (uint64_t(1) << square)) return true;

        // King
        if (kingAttacks_[square] & s.bitboards[WHITE_KING]) return true;
    }

    return false;
}

std::vector<Move> Board::generateLegalMoves() const {
    Move pseudo[218];
    int count = 0;
    pseudoMoves(pseudo, count);

    std::vector<Move> legal;
    legal.reserve(count);

    Board* self = const_cast<Board*>(this);
    for (int i = 0; i < count; ++i) {
        self->move(pseudo[i]);
        bool illegal = (self->turn() == BLACK)
            ? self->isAttacked(self->whiteKingSquare(), BLACK)
            : self->isAttacked(self->blackKingSquare(), WHITE);
        if (!illegal) {
            legal.push_back(pseudo[i]);
        }
        self->undo();
    }
    return legal;
}

bool Board::isLegal(const Move& m) const noexcept {
    Move pseudo[218];
    int count = 0;
    pseudoMoves(pseudo, count);

    for (int i = 0; i < count; ++i) {
        if (m == pseudo[i]) {
            Board* self = const_cast<Board*>(this);
            self->move(m);
            bool illegal = (self->turn() == BLACK)
                ? self->isAttacked(self->whiteKingSquare(), BLACK)
                : self->isAttacked(self->blackKingSquare(), WHITE);
            self->undo();
            return !illegal;
        }
    }
    return false;
}

bool Board::isInCheck() const noexcept {
    return whiteToMove() ? isAttacked(whiteKingSquare(), BLACK) : isAttacked(blackKingSquare(), WHITE);
}

bool Board::softDraw() const noexcept {
    const auto& s = stateStack_[stackIndex_];

    // 50-move rule (100 half-moves)
    if (s.halfMoves >= 100) return true;

    // 3-fold repetition
    int count = 0;
    for (int i = stackIndex_ - 2; i >= std::max(0, stackIndex_ - static_cast<int>(s.halfMoves)); i -= 2) {
        if (s.hash == stateStack_[i].hash) {
            ++count;
            if (count >= 2) return true;
        }
    }

    // Insufficient material
    if (s.bitboards[WHITE_PAWN] || s.bitboards[BLACK_PAWN] ||
        s.bitboards[WHITE_ROOK] || s.bitboards[BLACK_ROOK] ||
        s.bitboards[WHITE_QUEEN] || s.bitboards[BLACK_QUEEN]) {
        return false;
    }

    uint64_t whiteMinors = s.bitboards[WHITE_BISHOP] | s.bitboards[WHITE_KNIGHT];
    uint64_t blackMinors = s.bitboards[BLACK_BISHOP] | s.bitboards[BLACK_KNIGHT];

    if (popcount(whiteMinors) <= 1 && popcount(blackMinors) <= 1) {
        return true;
    }

    return false;
}

uint8_t Board::isTerminal() const noexcept {
    if (softDraw()) return DRAW;

    auto legalMoves = generateLegalMoves();
    if (legalMoves.empty()) {
        if (isInCheck()) {
            return whiteToMove() ? BLACK : WHITE;
        }
        return DRAW;
    }
    return 0;
}

bool Board::isCheckmate() const noexcept {
    uint8_t term = isTerminal();
    return term == WHITE || term == BLACK;
}

bool Board::isStalemate() const noexcept {
    return isTerminal() == DRAW && !softDraw();
}

void Board::print() const {
    std::cout << "\n  +-----------------+\n";
    for (int r = 0; r < 8; ++r) {
        std::cout << (8 - r) << " | ";
        for (int c = 0; c < 8; ++c) {
            uint8_t sq = static_cast<uint8_t>(r * 8 + c);
            uint8_t p = getPiece(sq);
            if (p == EMPTY) {
                std::cout << ". ";
            } else {
                static const char chars[] = "PNBRQKpnbrqk";
                std::cout << chars[p] << " ";
            }
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n\n";
    std::cout << "Turn: " << (whiteToMove() ? "White" : "Black")
              << " | FEN: " << toFen() << "\n\n";
}

} // namespace alphaone
