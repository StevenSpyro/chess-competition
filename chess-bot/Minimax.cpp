#include "chess.hpp"
#include "Minimax.h"
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <chrono>
#include "chess-simulator.h"

using namespace chess;

namespace ChessSimulator {

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

    const int king_pst[64] = {
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
         20, 30, 10,  0,  0, 10, 30, 20
    };

    // Good for making better moves in the end
    const int king_endgame_pst[64] = {
        -50,-40,-30,-20,-20,-30,-40,-50,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -50,-30,-30,-30,-30,-30,-30,-50
    };

    std::unordered_map<uint64_t, int> evalcache;

    // Values Pawn = 100 , Knight = 300, Bishop = 320, Rook 500, Queen 900, King 20000
    int evaluate(chess::Board& board) {
        // Hash key
        uint64_t hashKey = board.hash();

        // Check if pos has been eval
        auto it = evalcache.find(hashKey);
        if (it != evalcache.end()) {
            return it -> second;
        }

        int score = 0;

        // Assign Values
        auto evalPiece = [&](PieceType pt, Color color, const int pst[]) {
            int pScore = 0;
            Bitboard bb = board.pieces(pt, color);

            while (bb) {
                Square sq = bb.pop();

                if (pt == PieceType::PAWN) pScore += 100;
                else if (pt == PieceType::KNIGHT) pScore += 320;
                else if (pt == PieceType::BISHOP) pScore += 330;
                else if (pt == PieceType::ROOK) pScore += 500;
                else if (pt == PieceType::QUEEN) pScore += 900;
                else if (pt == PieceType::KING) pScore += 99999;

                int sqIdx = sq.index();
                int pstidx = (color == Color::WHITE) ? sqIdx : sqIdx ^ 56; // Flip
                pScore += pst[pstidx];
            }

            return pScore;
        };

        // Make the king more active when there are less pieces
        bool isEndgame = board.occ().count() <= 12;
        const int* active_king_pst = isEndgame ? king_endgame_pst : king_pst;

        int whiteEval = 0;
        whiteEval += evalPiece(PieceType::PAWN, Color::WHITE, pawn_pst);
        whiteEval += evalPiece(PieceType::KNIGHT, Color::WHITE, knight_pst);
        whiteEval += evalPiece(PieceType::BISHOP, Color::WHITE, bishop_pst);
        whiteEval += evalPiece(PieceType::ROOK, Color::WHITE, rook_pst);
        whiteEval += evalPiece(PieceType::QUEEN, Color::WHITE, queen_pst);
        whiteEval += evalPiece(PieceType::KING, Color::WHITE, active_king_pst);

        int blackEval = 0;
        blackEval += evalPiece(PieceType::PAWN, Color::BLACK, pawn_pst);
        blackEval += evalPiece(PieceType::KNIGHT, Color::BLACK, knight_pst);
        blackEval += evalPiece(PieceType::BISHOP, Color::BLACK, bishop_pst);
        blackEval += evalPiece(PieceType::ROOK, Color::BLACK, rook_pst);
        blackEval += evalPiece(PieceType::QUEEN, Color::BLACK, queen_pst);
        blackEval += evalPiece(PieceType::KING, Color::BLACK, active_king_pst);

        /*
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        int mobilityScore = moves.size() * 10;
        int finalScore = (whiteEval - blackEval);

        if (board.sideToMove() == Color::WHITE) finalScore += mobilityScore;
        else finalScore -= mobilityScore;

        int perspective = (board.sideToMove() == Color::WHITE) ? 1 : -1;
        int final_eval = finalScore * perspective;

        if (evalcache.size() < 500000) {
            evalcache[hashKey] = final_eval;
        }
        */

        int finalScore = whiteEval - blackEval;
        int perspective = (board.sideToMove() == Color::WHITE) ? 1 : -1;
        int final_eval = finalScore * perspective;

        if (evalcache.size() < 100000) {
            evalcache[hashKey] = final_eval;
        }

        return final_eval;
    }

    int quiescence(chess::Board& board, int alpha, int beta, int depth) {
        int stand_pat = evaluate(board);

        if (depth >= 10) return stand_pat;

        if (stand_pat >= beta) {
            return beta;
        }
        if (alpha < stand_pat) {
            alpha = stand_pat;
        }

        chess::Movelist captures;

        // Generate capture
        chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(captures, board);
        std::sort(captures.begin(), captures.end(), [&](const chess::Move& a, const chess::Move& b) {
            int victim_a = board.at(a.to()) != chess::Piece::NONE ? (int)board.at(a.to()).type().internal() : 0;
            int victim_b = board.at(b.to()) != chess::Piece::NONE ? (int)board.at(b.to()).type().internal() : 0;
            return victim_a > victim_b;
        });

        for (auto move : captures) {
            board.makeMove(move);
            int score = -quiescence(board, -beta, -alpha, depth + 1);
            board.unmakeMove(move);

            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
        return alpha;
    }

    static int node_count = 0;

    int minimax(chess::Board& board, int depth, int alpha, int beta,
                std::chrono::time_point<std::chrono::steady_clock> startTime,
                int time_limit, bool& time_up) {

        // EMERGENCY BRAKE: Check the clock every 2048 nodes
        if ((node_count++ & 2047) == 0) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= time_limit) {
                time_up = true;
                return 0;
            }
        }

        if (time_up) return 0; // Abort instantly if time is out

        if (depth == 0) return quiescence(board, alpha, beta, 0);
        if (board.isRepetition() || board.halfMoveClock() >= 100) return 0;

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        if (moves.size() == 0) {
            if (board.inCheck()) return -30000 + (20 - depth);
            return 0;
        }

        std::sort(moves.begin(), moves.end(), [&](const chess::Move& a, const chess::Move& b) {
            return board.isCapture(a) > board.isCapture(b);
        });

        for (auto move : moves) {
            board.makeMove(move);
            int score = -minimax(board, depth - 1, -beta, -alpha, startTime, time_limit, time_up);
            board.unmakeMove(move);

            if (time_up) return 0; // Abort instantly if time is out

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }
}