//
// Created by steve on 2/22/2026.
//

#ifndef CHESS_MCTS_H
#define CHESS_MCTS_H

#include "chess.hpp"
#include <vector>
#include <cmath>
#include <string>

namespace ChessSimulator {
    class MCTSNode {
    public:
        MCTSNode* parent;
        std::vector<MCTSNode*> children;
        double wins;
        int visits;
        chess::Board state;
        chess::Move move_from_parent;

        MCTSNode(chess::Board s, MCTSNode* p, chess::Move m);
        ~MCTSNode();

        double ucb(double C = std::sqrt(2.0));

        bool isLeaf();
        bool isTerminal();
    };

    std::string getBestMoveMCTS(const std::string& fen, int timeLimitMs);
}


#endif //CHESS_MCTS_H