//
// Created by steve on 2/22/2026.
//

#include "MCTS.h"
#include "Minimax.h"
#include <chrono>
#include <random>
#include <limits>
#include <algorithm>

namespace ChessSimulator {

    // Pointer to hold the root
    static MCTSNode* global_mcts_root = nullptr;

    MCTSNode::MCTSNode(MCTSNode* p, chess::Move m, uint64_t h)
        : parent(p), move_from_parent(m), hash(h), wins(0.0), visits(0) {}

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

    /*
    bool MCTSNode::isTerminal() {
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, state);
        return moves.empty() || state.halfMoveClock() >= 100;
    }
    */

    double rollout(chess::Board board, int tree_depth) {
        chess::Color start_turn = board.sideToMove();

        if (board.halfMoveClock() >= 100 || board.isRepetition()) {
            return 0.45;
        }

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        static std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());

        if (moves.empty()) {
            if (board.inCheck()) {
                // Checkmated
                if (board.sideToMove() == start_turn) return 0.0;
                return 0.51 + (0.49 * std::pow(0.99, tree_depth));
            }
            return 0.45; // Stale
        }

        int score = ChessSimulator::quiescence(board, -1000000, 1000000, 0);
        int relative_score = (board.sideToMove() == start_turn) ? score : -score;

        int clamped_score = std::max(-2000, std::min(2000, relative_score));

        double advantage = clamped_score / 4000.0;
        advantage *= std::pow(0.99, tree_depth);

        double win_prob = 0.5 + (clamped_score / 4000.0);

        if (win_prob > 0.999) win_prob = 0.999;
        if (win_prob < 0.001) win_prob = 0.001;

        return win_prob;

        /*
        int depth = 0;
        int max_rollout_depth = 30;

        while (true) {
            // The game ends in stalemate or a checkmate
            if (board.halfMoveClock() >= 100 || board.isRepetition()) {
                return 0.5;
            }

            if (moves.empty()) {
                if (board.inCheck()) {
                    if (board.sideToMove() == start_turn) {
                        return 0.0; // We got mated
                    } else {
                        // Mate
                        return 0.51 + (0.49 * std::pow(0.99, tree_depth));
                    }
                }
                return 0.5; // Stalemate
            }

            int score = ChessSimulator::quiescence(board, -1000000, 1000000, 0);
            int relative_score = (board.sideToMove() == start_turn) ? score : -score;
            return 1.0 / (1.0 + std::pow(10.0, -relative_score / 400.0));


            chess::Movelist moves;
            chess::movegen::legalmoves(moves, board);

            // The game ends in stalemate or a checkmate
            if (moves.empty()) {
                if (board.inCheck()) {
                    return (board.sideToMove() == start_turn) ? 0.0 : 1.0;
                }
                return 0.5; // Stale
            }

            // Make sure that this stops and evaluates because without it the AI is NOT good. OLD BEFORE QUIESCENCE

            if (depth >= max_rollout_depth) {
                int score = ChessSimulator::evaluate(board);
                int relative_score = (board.sideToMove() == start_turn) ? score : -score;
                return 1.0 / (1.0 + std::pow(10.0, -relative_score / 400.0));
            }


            if (depth >= max_rollout_depth) {
                int score = ChessSimulator::quiescence(board, -1000000, 1000000, 0);

                int relative_score = (board.sideToMove() == start_turn) ? score : -score;
                return 1.0 / (1.0 + std::pow(10.0, -relative_score / 400.0));
            }

            // Have a greater preference for captures
            std::vector<chess::Move> capture_moves;
            std::vector<chess::Move> quiet_moves;

            for (auto move : moves) {
                if (board.isCapture(move) || move.promotionType() != chess::PieceType::NONE) { // Encourage promo
                    capture_moves.push_back(move);
                } else {
                    quiet_moves.push_back(move);
                }
            }

            // Capture piece if you can
            if (!capture_moves.empty()) {
                std::uniform_int_distribution<> dist(0, capture_moves.size() - 1);
                board.makeMove(capture_moves[dist(gen)]);
            }

            // Play a random move
            else {
                std::uniform_int_distribution<> dist(0, quiet_moves.size() - 1);
                board.makeMove(quiet_moves[dist(gen)]);
            }

            depth++;

            // Go random
            //std::uniform_int_distribution<> dist(0, moves.size() - 1);
            //board.makeMove(moves[dist(gen)]);
            //depth++;
            */
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

        // Check for Mate
        for (auto move : root_moves) {
            board.makeMove(move);
            chess::Movelist responses;
            chess::movegen::legalmoves(responses, board);
            bool is_mate = responses.empty() && board.inCheck();
            board.unmakeMove(move);

            if (is_mate) {
                return chess::uci::moveToUci(move);
            }
        }

        // Tree Reuse
        if (global_mcts_root == nullptr) {
            global_mcts_root = new MCTSNode(nullptr, chess::Move::NULL_MOVE, board.hash());
        } else {
            // Get current board state in tree
            MCTSNode* matching_child = nullptr;
            for (auto child : global_mcts_root->children) {
                // Compare with the current Zobrist hash
                if (child -> hash == board.hash()) {
                    matching_child = child;
                    break;
                }
            }

            if (matching_child) {
                // Take the opponents move into the system so it can't be deleted
                for (auto& child : global_mcts_root->children) {
                    if (child != matching_child) {
                        delete child;
                    }
                }

                global_mcts_root -> children.clear();
                delete global_mcts_root;

                global_mcts_root = matching_child;
                global_mcts_root -> parent = nullptr;
            } else {
                // Start over
                delete global_mcts_root;
                global_mcts_root = new MCTSNode(nullptr, chess::Move::NULL_MOVE, board.hash());
            }
        }

        MCTSNode* root = global_mcts_root;
        //MCTSNode* root = new MCTSNode(board, nullptr, chess::Move::NULL_MOVE);
        auto startTime = std::chrono::steady_clock::now();

        // Timer just like the Minimax
        int budget = timeLimitMs > 0 ? timeLimitMs : 10000;
        int buffer = 500; // Buffer time

        if (budget <= buffer) {
            buffer = 0;
        }

        int iterations = 0;
        //int max_iterations = 100000; shouldn't need now
        double exploration_constant = 0.5;

        while (true) {

            if (iterations % 10 == 0) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= (budget - buffer)) {
                    break;
                }
            }
            iterations++;

            MCTSNode* current = root;
            chess::Board current_board = board;

            int tree_depth = 0;

            // Select
            while (!current->isLeaf()) {
                chess::Movelist legal_moves;
                chess::movegen::legalmoves(legal_moves, current_board);

                if (legal_moves.empty() || current_board.halfMoveClock() >= 100 || current_board.isRepetition()) {
                    break;
                }

                MCTSNode* best_child = nullptr;
                double best_ucb = -1.0;
                for (auto child : current->children) {
                    double ucb_val = child->ucb(exploration_constant);
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
                current_board.makeMove(current->move_from_parent);

                tree_depth++;
            }

            /*
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
            */

            chess::Movelist moves;
            chess::movegen::legalmoves(moves, current_board);
            bool is_terminal = moves.empty() || current_board.halfMoveClock() >= 100 || current_board.isRepetition();

            // Expand
            if (!is_terminal) {
                if (current->visits > 0 || current == root) {

                    std::sort(moves.begin(), moves.end(), [&](const chess::Move& a, const chess::Move& b) {
                        return current_board.isCapture(a) > current_board.isCapture(b);
                    });

                    for (auto move : moves) {
                        current_board.makeMove(move);
                        current->children.push_back(new MCTSNode(current, move, current_board.hash()));
                        current_board.unmakeMove(move);
                    }

                    current = current->children[0];
                    current_board.makeMove(current->move_from_parent);

                    tree_depth++;
                }
            }

            // Sim
            //double result = rollout(current_board);
            double result = rollout(current_board, tree_depth);

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

        //chess::Move best_move = best_child ? best_child->move_from_parent : root_moves[0];
        chess::Move best_move = root_moves[0];

        if (best_child) {
            best_move = best_child->move_from_parent;

            // Remove child from root
            for (auto& child : global_mcts_root->children) {
                if (child != best_child) {
                    delete child;
                }
            }
            global_mcts_root->children.clear();

            // Delete root memory
            delete global_mcts_root;

            global_mcts_root = best_child;
            global_mcts_root->parent = nullptr;
        }

        /*
        std::cout << "Total Iterations: " << root->visits << std::endl;
        if (best_child) {
            double win_rate = (best_child->wins / best_child->visits) * 100.0;
            std::cout << "Best Move: " << chess::uci::moveToUci(best_move) << std::endl;
            std::cout << "Node Visits: " << best_child->visits << std::endl;
            std::cout << "Expected Win Rate: " << win_rate << "%" << std::endl;
        }
        */


        //delete root;
        return chess::uci::moveToUci(best_move);
    }

}
