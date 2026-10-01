#include "Search.hpp"
#include "Evaluation.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace alphaone {

Search::Search(const SearchConfig& config)
    : config_(config), tt_(config.tt_size_mb) {
    clearHistory();
}

void Search::clearHistory() noexcept {
    std::memset(killer_moves_, 0, sizeof(killer_moves_));
    std::memset(history_, 0, sizeof(history_));
}

bool Search::shouldStop() noexcept {
    if (stop_flag_.load(std::memory_order_relaxed)) {
        return true;
    }
    if (effective_time_limit_ms_ > 0 && (stats_.nodes % 1024 == 0)) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
        if (elapsed >= effective_time_limit_ms_) {
            stop_flag_.store(true, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

void Search::orderMoves(std::vector<Move>& moves, const Board& board, const Move& tt_move, int ply) const noexcept {
    std::vector<int> scores(moves.size(), 0);

    for (size_t i = 0; i < moves.size(); ++i) {
        const auto& m = moves[i];
        if (!tt_move.isNull() && m == tt_move) {
            scores[i] = 20000000;
            continue;
        }

        uint8_t targetPiece = board.getPiece(m.to);
        uint8_t movedPiece = board.getPiece(m.from);

        // Capture (including en-passant)
        if (targetPiece != EMPTY) {
            int victimVal = Evaluation::pieceValue(targetPiece);
            int attackerVal = Evaluation::pieceValue(movedPiece);
            scores[i] = 10000000 + (victimVal * 10) - (attackerVal / 10);
        } else if (m.to == ((board.getState()->passantTarget != 0) ? lsbIndex(board.getState()->passantTarget) : 255)) {
            scores[i] = 10000000 + (Evaluation::PAWN_VAL * 10);
        } else if (m.promotion != EMPTY) {
            scores[i] = 9000000 + Evaluation::pieceValue(m.promotion);
        } else {
            // Killer moves
            if (ply < 64 && m == killer_moves_[ply][0]) {
                scores[i] = 8000000;
            } else if (ply < 64 && m == killer_moves_[ply][1]) {
                scores[i] = 7000000;
            } else {
                // History heuristic
                if (movedPiece < 12 && m.to < 64) {
                    scores[i] = history_[movedPiece][m.to];
                }
            }
        }
    }

    // Sort moves descending by heuristic score
    for (size_t i = 0; i < moves.size(); ++i) {
        size_t bestIdx = i;
        for (size_t j = i + 1; j < moves.size(); ++j) {
            if (scores[j] > scores[bestIdx]) {
                bestIdx = j;
            }
        }
        if (bestIdx != i) {
            std::swap(moves[i], moves[bestIdx]);
            std::swap(scores[i], scores[bestIdx]);
        }
    }
}

int Search::quiescence(Board& board, int alpha, int beta, int ply) {
    ++stats_.nodes;

    if (shouldStop()) return 0;
    if (board.softDraw()) return 0;

    int turnMultiplier = board.whiteToMove() ? 1 : -1;
    int standPat = turnMultiplier * Evaluation::evaluate(board);

    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    // Generate pseudo-legal moves and filter captures
    Move pseudo[218];
    int numMoves = 0;
    board.pseudoMoves(pseudo, numMoves);

    std::vector<Move> captures;
    captures.reserve(numMoves);

    for (int i = 0; i < numMoves; ++i) {
        const auto& m = pseudo[i];
        if (board.getPiece(m.to) != EMPTY || m.promotion != EMPTY ||
            (board.getState()->passantTarget && m.to == lsbIndex(board.getState()->passantTarget))) {
            captures.push_back(m);
        }
    }

    orderMoves(captures, board, Move{}, ply);

    for (const auto& m : captures) {
        // Delta pruning: if capture cannot raise alpha by reasonable margin, skip
        uint8_t captured = board.getPiece(m.to);
        int gain = (captured != EMPTY) ? Evaluation::pieceValue(captured) : Evaluation::PAWN_VAL;
        if (m.promotion != EMPTY) gain += Evaluation::QUEEN_VAL;
        if (standPat + gain + 200 < alpha) continue;

        board.move(m);
        bool illegal = (board.turn() == BLACK)
            ? board.isAttacked(board.whiteKingSquare(), BLACK)
            : board.isAttacked(board.blackKingSquare(), WHITE);

        if (!illegal) {
            int score = -quiescence(board, -beta, -alpha, ply + 1);
            board.undo();

            if (shouldStop()) return 0;

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        } else {
            board.undo();
        }
    }

    return alpha;
}

int Search::negamax(Board& board, int depth, int alpha, int beta, int ply, bool is_root) {
    ++stats_.nodes;

    if (shouldStop()) return 0;

    // Draw detection (repetition, 50-move rule, insufficient material)
    if (!is_root && board.softDraw()) {
        return 0;
    }

    uint64_t hashKey = board.zobristHash();
    int alphaOrig = alpha;
    Move ttMove{};

    // Transposition Table probe
    if (config_.use_tt) {
        int ttScore = 0;
        if (tt_.probe(hashKey, depth, alpha, beta, ttScore, ttMove, ply)) {
            if (!is_root) {
                return ttScore;
            }
        }
    }

    // Leaf node: drop into Quiescence Search
    if (depth <= 0) {
        return quiescence(board, alpha, beta, ply);
    }

    auto legalMoves = board.generateLegalMoves();
    if (legalMoves.empty()) {
        if (board.isInCheck()) {
            return -SCORE_CHECKMATE + ply; // Checkmate
        }
        return 0; // Stalemate
    }

    // Move ordering
    orderMoves(legalMoves, board, ttMove, ply);

    int maxScore = -SCORE_CHECKMATE;
    Move bestMoveThisNode = legalMoves[0];

    for (const auto& m : legalMoves) {
        board.move(m);
        int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, false);
        board.undo();

        if (shouldStop()) return 0;

        if (score > maxScore) {
            maxScore = score;
            bestMoveThisNode = m;
            if (is_root) {
                root_best_move_ = m;
            }
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            // Beta cutoff / fail-high: store killer and history heuristics for quiet moves
            if (board.getPiece(m.to) == EMPTY && m.promotion == EMPTY) {
                if (ply < 64) {
                    if (killer_moves_[ply][0] != m) {
                        killer_moves_[ply][1] = killer_moves_[ply][0];
                        killer_moves_[ply][0] = m;
                    }
                }
                uint8_t movedPiece = board.getPiece(m.from);
                if (movedPiece < 12 && m.to < 64) {
                    history_[movedPiece][m.to] += depth * depth;
                }
            }
            break;
        }
    }

    // Transposition Table store
    if (config_.use_tt && !shouldStop()) {
        BoundType bound = BoundType::Exact;
        if (maxScore <= alphaOrig) {
            bound = BoundType::UpperBound; // Fail-low
        } else if (maxScore >= beta) {
            bound = BoundType::LowerBound; // Fail-high
        }
        tt_.store(hashKey, depth, maxScore, bound, bestMoveThisNode, ply);
    }

    return maxScore;
}

SearchStats Search::searchBestMove(Board& board, int target_depth, int time_limit_ms) {
    if (target_depth <= 0) target_depth = config_.max_depth;
    effective_time_limit_ms_ = (time_limit_ms > 0) ? time_limit_ms : config_.time_limit_ms;

    stop_flag_.store(false, std::memory_order_relaxed);
    start_time_ = std::chrono::steady_clock::now();

    stats_ = SearchStats{};
    tt_.resetStats();
    clearHistory();

    auto legalMoves = board.generateLegalMoves();
    if (legalMoves.empty()) {
        int turnMul = board.whiteToMove() ? 1 : -1;
        stats_.score = turnMul * Evaluation::evaluate(board);
        return stats_;
    }

    root_best_move_ = legalMoves[0];
    Move bestCompletedMove = root_best_move_;
    int bestCompletedScore = 0;
    int completedDepth = 0;

    // Iterative deepening from depth 1 up to target_depth
    for (int d = 1; d <= target_depth; ++d) {
        root_best_move_ = bestCompletedMove;
        int score = negamax(board, d, -SCORE_CHECKMATE, SCORE_CHECKMATE, 0, true);

        if (stop_flag_.load(std::memory_order_relaxed) && d > 1) {
            // Aborted during this depth, fallback to previous completed depth
            break;
        }

        completedDepth = d;
        bestCompletedScore = score;
        bestCompletedMove = root_best_move_;

        // Early exit on mate found
        if (std::abs(score) >= SCORE_CHECKMATE - 100) {
            break;
        }

        if (shouldStop()) {
            break;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - start_time_).count());

    stats_.depth = completedDepth;
    stats_.score = bestCompletedScore;
    stats_.best_move = bestCompletedMove;
    stats_.best_move_uci = bestCompletedMove.toUci();
    stats_.elapsed_ms = elapsed;
    stats_.nodes_per_second = (elapsed > 0) ? ((stats_.nodes * 1000) / elapsed) : (stats_.nodes * 1000);
    stats_.transposition_hits = tt_.hits();

    return stats_;
}

} // namespace alphaone
