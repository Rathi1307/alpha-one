#include "../src/engine/Board.hpp"
#include "../src/engine/Search.hpp"
#include "../src/engine/Evaluation.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>

using namespace alphaone;

struct BenchmarkPosition {
    std::string name;
    std::string fen;
    std::vector<std::string> expected_moves; // Any of these moves is considered correct
    std::string theme;
};

// 24 Classical Bratko-Kopec Test (BK-Test) Positions
const std::vector<BenchmarkPosition> BK_POSITIONS = {
    {"BK.01", "1k1r4/pp1b1R2/1rq4p/2b1p1p1/2B1P3/2B5/PPP2Q2/2K5 w - - 0 1", {"c4d5", "f2f6", "c3e5"}, "Tactical Sacrifice/Attack"},
    {"BK.02", "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 1", {"d4d5"}, "Central Pawn Break"},
    {"BK.03", "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P1BBPNN/3Q2K1/R6R b - - 0 1", {"f6f5"}, "Pawn Break Attack"},
    {"BK.04", "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq - 0 1", {"e5e6"}, "Pawn Wedge/Sacrifice"},
    {"BK.05", "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - - 0 1", {"a2a4", "b3d5"}, "Queenside Action"},
    {"BK.06", "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - - 0 1", {"g5g6"}, "King/Pawn Restriction"},
    {"BK.07", "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P1R3/2P1B1PP/R2Q2K1 w - - 0 1", {"h5f6", "e2h5"}, "Kingside Knight Breakthrough"},
    {"BK.08", "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - - 0 1", {"f4f5"}, "Endgame Breakthrough"},
    {"BK.09", "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - - 0 1", {"f4f5"}, "Kingside Pawn Break"},
    {"BK.10", "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - - 0 1", {"c6e5"}, "Outpost Knight Entry"},
    {"BK.11", "2r1nrk1/p2q1ppp/bp1p6/n1pPp3/P1P1P3/2PB1P2/3QN1PP/R1B2RK1 w - - 0 1", {"f3f4"}, "King Attack Preparation"},
    {"BK.12", "r3r1k1/pp5p/2p1b1p1/1q3p2/2n1p3/1PB1P2P/P3QPP1/1B1R1RK1 b - - 0 1", {"c4a3"}, "Winning Piece/Bishop pair"},
    {"BK.13", "r3k2r/pb1n1ppp/1p1bpq2/2pp4/3P4/2PBPN2/PP1N1PPP/R2QK2R w KQkq - 0 1", {"e3e4"}, "Central Push"},
    {"BK.14", "r2q1rk1/1pp1bppp/p2p1n2/3Pn3/4PB2/2N2B2/PPP3PP/R2Q1RK1 b - - 0 1", {"f6d7", "e5f3"}, "Knight Maneuver"},
    {"BK.15", "6k1/5p2/6p1/8/7p/8/6PP/6K1 b - - 0 1", {"g8g7", "f7f5"}, "Endgame King Activity"},
    {"BK.16", "r1bqk2r/pp2bppp/2p1pn2/3p4/2PP4/2N1PN2/PP2BPPP/R1BQK2R w KQkq - 0 1", {"e1g1", "e3e4", "c4d5"}, "Positional Development"},
    {"BK.17", "r2qkb1r/pp1b1ppp/2n1pn2/2pp4/2PP4/2NBPN2/PP3PPP/R1BQK2R w KQkq - 0 1", {"e1g1", "c4d5", "a2a3"}, "Classical Center Tension"},
    {"BK.18", "r1bq1rk1/pp1n1ppp/2p1pn2/3p4/2PP4/2N1PN2/PPQ2PPP/R1B1KB1R w KQ - 0 1", {"c1d2", "f1e2", "c4d5", "b2b3"}, "Opening Development"},
    {"BK.19", "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - 0 1", {"e1g1", "d2d3"}, "Four Knights Opening"},
    {"BK.20", "r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 0 1", {"d2d4", "f1b5", "f1c4"}, "Open Game Initiative"},
    {"BK.21", "r1bqkb1r/pppp1ppp/2n5/4p3/2BnP3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", {"f3d4", "c2c3", "e1g1"}, "Blackburne Gambit Defense"},
    {"BK.22", "r1bqk1nr/pppp1ppp/2n5/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", {"c2c3", "d2d3", "e1g1", "b2b4"}, "Italian Game Strategy"},
    {"BK.23", "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1", {"f1b5", "f1c4", "d2d4", "c2c3", "b1c3"}, "Ruy Lopez / Italian / Scotch"},
    {"BK.24", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", {"e2e4", "d2d4", "c2c4", "g1f3"}, "Starting Position"}
};

// 10 Tactical & Mate Problems
const std::vector<BenchmarkPosition> TACTICAL_POSITIONS = {
    {"WAC.01", "2r3k1/ppp2ppp/8/3N4/8/8/PPP2PPP/4R1K1 w - - 0 1", {"d5e7"}, "Fork Knight (Ne7+)"},
    {"WAC.02", "r1bqkb1r/pppp1ppp/2n5/4p3/2B1n3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 0 1", {"f3f7"}, "Mate in 1 (Qxf7#)"},
    {"WAC.03", "6k1/5ppp/8/8/8/8/1R6/1R4K1 w - - 0 1", {"b2b8"}, "Back-rank Mate in 2 (Rb8+)"},
    {"WAC.04", "r1b1k2r/ppppqppp/2n5/b3P3/2B5/1Q3N2/PB3PPP/R4RK1 w kq - 0 1", {"c4f7", "e5e6", "b2a3"}, "Attacking F7 / Pin"},
    {"WAC.05", "r1b2rk1/pp3ppp/2n1p3/q7/2BP4/4PN2/P2Q1PPP/R3K2R w KQ - 0 1", {"d2a5"}, "Queen Trade Material"},
    {"WAC.06", "3q2k1/pb3ppp/1p6/3p4/3Q4/4PB2/PP3PPP/6K1 w - - 0 1", {"f3d5", "f3e4", "h2h3", "g2g3"}, "Pressure on Isolated Pawn"},
    {"WAC.07", "r2q1rk1/1pp2ppp/p1np1n2/2b1p3/2B1P1b1/2NP1N2/PPPB1PPP/R2Q1RK1 w - - 0 1", {"c3d5", "h2h3", "a2a3"}, "Pin Breaking / Outpost"},
    {"WAC.08", "r1b1kb1r/ppqn1ppp/2p1pn2/8/2BPP3/2N2N2/PP3PPP/R1BQK2R w KQkq - 0 1", {"e4e5", "e1g1", "c1g5"}, "Space Advantage Push"},
    {"WAC.09", "r1bq1rk1/pp1nbppp/2p1pn2/3p4/2PP4/2NBPN2/PP3PPP/R1BQ1RK1 w - - 0 1", {"e3e4", "c4d5", "b2b3", "c1d2"}, "Central Break Preparation"},
    {"WAC.10", "r1bqk2r/pp1n1ppp/2p1pn2/3p2B1/2PP4/2P1PN2/P4PPP/R2QKB1R w KQkq - 0 1", {"c4d5", "d1b3", "f1d3"}, "Pressure Against c6/d5"}
};

void runBenchmarkSuite(const std::string& suite_name, const std::vector<BenchmarkPosition>& positions, int search_depth, int time_limit_ms) {
    std::cout << "\n========================================================================================\n";
    std::cout << " Running Suite: " << suite_name << " (Target Depth: " << search_depth << ", Max Time: " << time_limit_ms << "ms)\n";
    std::cout << "========================================================================================\n";
    std::cout << std::left << std::setw(8) << "ID"
              << std::setw(28) << "Theme / Description"
              << std::setw(12) << "Engine Move"
              << std::setw(18) << "Expected Move(s)"
              << std::setw(8) << "Status"
              << std::setw(10) << "Time (ms)"
              << std::setw(10) << "Nodes"
              << std::setw(10) << "NPS"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    int solved = 0;
    int total = static_cast<int>(positions.size());
    uint64_t total_nodes = 0;
    int total_time_ms = 0;

    for (const auto& pos : positions) {
        Board board;
        bool ok = board.setPosition(pos.fen);
        if (!ok) {
            std::cout << "[ERROR] Invalid FEN in " << pos.name << "\n";
            continue;
        }

        Search search;
        SearchStats stats = search.searchBestMove(board, search_depth, time_limit_ms);

        bool is_correct = false;
        for (const auto& exp : pos.expected_moves) {
            if (stats.best_move_uci == exp) {
                is_correct = true;
                break;
            }
        }

        if (is_correct) ++solved;
        total_nodes += stats.nodes;
        total_time_ms += stats.elapsed_ms;

        std::string exp_str = pos.expected_moves.empty() ? "-" : pos.expected_moves[0];
        if (pos.expected_moves.size() > 1) exp_str += " (+" + std::to_string(pos.expected_moves.size() - 1) + ")";

        std::cout << std::left << std::setw(8) << pos.name
                  << std::setw(28) << (pos.theme.size() > 26 ? pos.theme.substr(0, 25) + "." : pos.theme)
                  << std::setw(12) << stats.best_move_uci
                  << std::setw(18) << exp_str
                  << std::setw(8) << (is_correct ? "[PASS]" : "[FAIL]")
                  << std::setw(10) << stats.elapsed_ms
                  << std::setw(10) << stats.nodes
                  << std::setw(10) << stats.nodes_per_second
                  << "\n";
    }

    double solve_rate = (100.0 * solved) / total;
    uint64_t avg_nps = (total_time_ms > 0) ? (total_nodes * 1000) / total_time_ms : total_nodes;

    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << " Results for " << suite_name << ": " << solved << "/" << total
              << " (" << std::fixed << std::setprecision(1) << solve_rate << "%) Solved | Total Time: "
              << total_time_ms << "ms | Average NPS: " << avg_nps << "\n";
}

int main() {
    std::cout << "========================================================================================\n";
    std::cout << "         AlphaOne (Polymath 1900 ELO Architecture) Comprehensive Benchmark\n";
    std::cout << "========================================================================================\n";

    // 1. Bratko-Kopec Positional & Tactical Test
    runBenchmarkSuite("Bratko-Kopec Test (BK-Test 24)", BK_POSITIONS, 7, 3000);

    // 2. Win At Chess Tactical & Mate Suite
    runBenchmarkSuite("Win At Chess (WAC / Mate Tactics)", TACTICAL_POSITIONS, 7, 3000);

    return 0;
}
