#include "chess.hpp"
#include "Minimax.h"
#include <algorithm>
#include "chess-simulator.h"

using namespace chess;

namespace ChessSimulator {

    const int INFINITY = 100000;

    // Square Table for better Center attention
    const int pawn_pst[64] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5, -5,-10,  0,  0,-10, -5,  5,
         5, 10, 10,-20,-20, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };

    const int knight_pst[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };

    const int bishop_pst[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };

    const int rook_pst[64] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        5, 10, 10, 10, 10, 10, 10,  5,
       -5,  0,  0,  0,  0,  0,  0, -5,
       -5,  0,  0,  0,  0,  0,  0, -5,
       -5,  0,  0,  0,  0,  0,  0, -5,
       -5,  0,  0,  0,  0,  0,  0, -5,
       -5,  0,  0,  0,  0,  0,  0, -5,
        0,  0,  0,  5,  5,  0,  0,  0
    };

    const int queen_pst[64] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };

    // Values Pawn = 100 , Knight = 300, Bishop = 320, Rook 500, Queen 900
    int evaluate(chess::Board& board) {
        int score = 0;

        // Assign Values
        auto evalPiece = [&](PieceType pt, Color color, const int pst[]) {
            int pScore = 0;
            Bitboard bb = board.pieces(pt, color);

            while (bb) {
                Square sq = bb.pop();

                if (pt == PieceType::PAWN) pScore += 100;
                else if (pt == PieceType::KNIGHT) pScore += 300;
                else if (pt == PieceType::BISHOP) pScore += 320;
                else if (pt == PieceType::ROOK) pScore += 500;
                else if (pt == PieceType::QUEEN) pScore += 900;

                int sqIdx = sq.index();
                int pstidx = (color == Color::WHITE) ? sqIdx : sqIdx ^ 56; // Flip
                pScore += pst[pstidx];
            }

            return pScore;
        };

        int whiteEval = 0;
        whiteEval += evalPiece(PieceType::PAWN, Color::WHITE, pawn_pst);
        whiteEval += evalPiece(PieceType::KNIGHT, Color::WHITE, knight_pst);
        whiteEval += evalPiece(PieceType::BISHOP, Color::WHITE, bishop_pst);
        whiteEval += evalPiece(PieceType::ROOK, Color::WHITE, rook_pst);
        whiteEval += evalPiece(PieceType::QUEEN, Color::WHITE, queen_pst);

        int blackEval = 0;
        blackEval += evalPiece(PieceType::PAWN, Color::BLACK, pawn_pst);
        blackEval += evalPiece(PieceType::KNIGHT, Color::BLACK, knight_pst);
        blackEval += evalPiece(PieceType::BISHOP, Color::BLACK, bishop_pst);
        blackEval += evalPiece(PieceType::ROOK, Color::BLACK, rook_pst);
        blackEval += evalPiece(PieceType::QUEEN, Color::BLACK, queen_pst);

        int perspective = (board.sideToMove() == Color::WHITE) ? 1 : -1;
        return (whiteEval - blackEval) * perspective;
    }

    int minimax(chess::Board& board, int depth, int alpha, int beta) {
        if (depth == 0) return evaluate(board);

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        if (moves.size() == 0) {
            if (board.inCheck()) return -INFINITY + (10 - depth); // Game ends in Checkmate
            return 0; // Game ends in Stalemate
        }

        int bestScore = -INFINITY;

        // Makes pruning faster
        std::sort(moves.begin(), moves.end(), [&](const chess::Move& a, const chess::Move& b) {
        return board.isCapture(a) > board.isCapture(b);
    });

        for (auto move : moves) {
            board.makeMove(move);
            int score = -minimax(board, depth - 1, -beta, -alpha);
            board.unmakeMove(move);

            if (score >= beta) return beta; // Prune
            if (score > alpha) alpha = score;
        }

        return alpha;
    }
}