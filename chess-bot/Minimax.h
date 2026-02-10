//
// Created by steve on 2/10/2026.
//

#ifndef CHESS_MINIMAX_H
#define CHESS_MINIMAX_H

#include "chess.hpp"
#include <string>

namespace ChessSimulator {
    int evaluate(chess::Board& board);
    int minimax(chess::Board& board, int depth, int alpha, int beta);
}

#endif //CHESS_MINIMAX_H