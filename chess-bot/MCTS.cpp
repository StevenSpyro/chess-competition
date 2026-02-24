//
// Created by steve on 2/22/2026.
//

#include "MCTS.h"
#include "Minimax.h"
#include <chrono>
#include <random>
#include <limits>

namespace ChessSimulator {

    MCTSNode::MCTSNode(chess::Board s, MCTSNode* p, chess::Move m)
        : state(s), parent(p), move_from_parent(m), wins(0.0), visits(0) {}

    MCTSNode::~MCTSNode() {
        for (auto child : children) {
            delete child;
        }
    }

    double MCTSNode::ucb(double C) {
        if (visits == 0) return std::numeric_limits<double>::infinity();
        return (wins / visits) + C * std::sqrt(std::log(parent->visits) / visits);
    }

    bool MCTSNode::isLeaf() {
        return children.empty();
    }

    bool MCTSNode::isTerminal() {
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, state);
        return moves.empty() || state.halfMoveClock() >= 100;
    }

    double rollout(chess::Board board) {
        chess::Color start_turn = board.sideToMove();
        static std::random_device rd;
        static std::mt19937 gen(rd());

        int depth = 0;
        int max_rollout_depth = 15;

        while (true) {
            chess::Movelist moves;
            chess::movegen::legalmoves(moves, board);

            // 1. The game ends in stalemate or a checkmate
            if (moves.empty()) {
                if (board.inCheck()) {
                    return (board.sideToMove() == start_turn) ? 0.0 : 1.0;
                }
                return 0.5; // Staled
            }

            // Make sure that this stops and evaluates because without it the AI is NOT good.
            if (depth >= max_rollout_depth || board.halfMoveClock() >= 100) {

                int score = ChessSimulator::evaluate(board);

                if (score == 0) return 0.5; // Dead even

                // Checks to see who is winning
                bool start_turn_is_winning;
                if (board.sideToMove() == start_turn) {
                    start_turn_is_winning = (score > 0);
                } else {
                    start_turn_is_winning = (score < 0);
                }

                return start_turn_is_winning ? 1.0 : 0.0;
            }

            // Go random
            std::uniform_int_distribution<> dist(0, moves.size() - 1);
            board.makeMove(moves[dist(gen)]);
            depth++;
        }
    }

    // Allows for the flipping of the result.
    void backpropagate(MCTSNode* node, double result) {
        while (node != nullptr) {
            node -> visits++;
            node -> wins += result;
            result = 1.0 - result;
            node = node -> parent;
        }
    }

    // Searching Func
    std::string getBestMoveMCTS(const std::string& fen, int timeLimitMs) {
        chess::Board board(fen);
        chess::Movelist root_moves;
        chess::movegen::legalmoves(root_moves, board);

        if (root_moves.empty()) return "";

        MCTSNode* root = new MCTSNode(board, nullptr, chess::Move::NULL_MOVE);
        auto startTime = std::chrono::steady_clock::now();

        // Timer just like the Minimax
        int budget = timeLimitMs > 0 ? timeLimitMs : 1000;
        int buffer = 50; // Buffer time

        while (true) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= (budget - buffer)) {
                break;
            }

            // Select
            MCTSNode* current = root;
            while (!current -> isLeaf() && !current -> isTerminal()) {
                MCTSNode* best_child = nullptr;
                double best_ucb = -1.0;
                for (auto child : current -> children) {
                    double ucb_val = child -> ucb();
                    if (ucb_val == std::numeric_limits<double>::infinity()) {
                        best_child = child;
                        break;
                    }
                    if (ucb_val > best_ucb) {
                        best_ucb = ucb_val;
                        best_child = child;
                    }
                }

                current = best_child;
            }

            // Expand
            if (!current -> isTerminal()) {
                if (current -> visits > 0 || current == root) {
                    chess::Movelist moves;
                    chess::movegen::legalmoves(moves, current->state);
                    for (auto move : moves) {
                        chess::Board child_board = current -> state;
                        child_board.makeMove(move);
                        current->children.push_back(new MCTSNode(child_board, current, move));
                    }

                    current = current -> children[0];
                }
            }

            // Sim
            double result = rollout(current -> state);

            // Backprop
            backpropagate(current, 1.0 - result);
        }

        MCTSNode* best_child = nullptr;
        int max_visits = -1;
        for (auto child : root -> children) {
            if (child -> visits > max_visits) {
                max_visits = child -> visits;
                best_child = child;
            }
        }

        chess::Move best_move = best_child ? best_child->move_from_parent : root_moves[0];

        std::cout << "Total Iterations: " << root->visits << std::endl;
        if (best_child) {
            double win_rate = (best_child->wins / best_child->visits) * 100.0;
            std::cout << "Best Move: " << chess::uci::moveToUci(best_move) << std::endl;
            std::cout << "Node Visits: " << best_child->visits << std::endl;
            std::cout << "Expected Win Rate: " << win_rate << "%" << std::endl;
        }

        delete root;
        return chess::uci::moveToUci(best_move);
    }

}
