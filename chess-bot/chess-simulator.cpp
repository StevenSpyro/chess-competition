#include "chess-simulator.h"
// disservin's lib. drop a star on his hard work!
// https://github.com/Disservin/chess-library
#include <chrono>

#include "chess.hpp"
#include "Minimax.h"
#include "MCTS.h"
#include <random>
#include <unordered_map>

using namespace ChessSimulator;

namespace ChessSimulator {
  extern std::unordered_map<uint64_t, int> evalcache;

  int minimax(chess::Board& board, int depth, int alpha, int beta,
              std::chrono::time_point<std::chrono::steady_clock> startTime,
              int time_limit, bool& time_up);
}

std::string ChessSimulator::Move(std::string fen, int timeLimitMs) {
  // create your board based on the board string following the FEN notation
  // search for the best move using minimax / monte carlo tree search /
  // alpha-beta pruning / ... try to use nice heuristics to speed up the search
  // and have better results return the best move in UCI notation you will gain
  // extra points if you create your own board/move representation instead of
  // using the one provided by the library

  if (ChessSimulator::evalcache.size() > 100000) {
    ChessSimulator::evalcache.clear();
  }

  chess::Board board(fen);
  chess::Movelist moves;
  chess::movegen::legalmoves(moves, board);

  if(moves.size() == 0) {
    return "";
  }

  auto startTime = std::chrono::steady_clock::now();
  int budget = timeLimitMs > 0 ? timeLimitMs : 10000;
  int time_limit = budget - 500;
  bool time_up = false;

  chess::Move bestMoveGlobal = moves[0];

  // Get the iterative deepening back working hopefully helps
  for (int depth = 1; depth <= 30; depth++) {
    int bestScore = -1000000;
    chess::Move currentBestMove = moves[0];

    std::sort(moves.begin(), moves.end(), [&](const chess::Move& a, const chess::Move& b) {
        return board.isCapture(a) > board.isCapture(b);
    });

    for (auto& move : moves) {
      board.makeMove(move);
      int score = -ChessSimulator::minimax(board, depth - 1, -1000000, 1000000, startTime, time_limit, time_up);
      board.unmakeMove(move);

      if (score > bestScore) {
        bestScore = score;
        currentBestMove = move;
      }
    }

    bestMoveGlobal = currentBestMove;

    // PLEASE CHECKMATE
    if (bestScore > 29000) {
      break;
    }
  }

  return chess::uci::moveToUci(bestMoveGlobal);

  /*

  // Make the game use MCTS and then the Minimax
  if (board.sideToMove() == chess::Color::WHITE) {
    return getBestMoveMCTS(fen, timeLimitMs);
  } else {
    chess::Move bestMoveGlobal = moves[0];
    auto startTime = std::chrono::steady_clock::now();

    // Depth Search aka iterative
    for (int depth = 1; depth <= 20; depth++) {
      int bestScore = -1000000;
      chess::Move currentBestMove = moves[0];

      for (auto& move : moves) {
        board.makeMove(move);
        int score = -ChessSimulator::minimax(board, depth - 1, -1000000, 1000000);
        board.unmakeMove(move);

        if (score > bestScore) {
          bestScore = score;
          currentBestMove = move;
        }

        // Return time
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= 100) {
          return chess::uci::moveToUci(bestMoveGlobal);
        }
      }
      bestMoveGlobal = currentBestMove;
    }
    return chess::uci::moveToUci(bestMoveGlobal);
  }
  */
}



/*
  chess::Move bestMoveGlobal = moves[0];
  auto startTime = std::chrono::steady_clock::now();

  // Depth search
  for (int depth = 1; depth <= 20; depth++) {
    int bestScore = -1000000;
    chess::Move currentBestMove = moves[0];

    for (auto& move : moves) {
      board.makeMove(move);
      int score = -ChessSimulator::minimax(board, depth - 1, -1000000, 1000000);
      board.unmakeMove(move);

      if (score > bestScore) {
        bestScore = score;
        currentBestMove = move;
      }

      // Ensure it returns before 5 seconds
      auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= 100) {
        return chess::uci::moveToUci(bestMoveGlobal);
      }
    }

    bestMoveGlobal = currentBestMove;
  }

  return chess::uci::moveToUci(bestMoveGlobal);
*/
  /*
  // get random move
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, moves.size() - 1);
  auto move = moves[dist(gen)];
  return chess::uci::moveToUci(move);

  */
//}