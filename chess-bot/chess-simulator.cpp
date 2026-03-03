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
}

std::string ChessSimulator::Move(std::string fen, int timeLimitMs) {
  // create your board based on the board string following the FEN notation
  // search for the best move using minimax / monte carlo tree search /
  // alpha-beta pruning / ... try to use nice heuristics to speed up the search
  // and have better results return the best move in UCI notation you will gain
  // extra points if you create your own board/move representation instead of
  // using the one provided by the library

  if (fen == "startpos") {
    fen = chess::constants::STARTPOS;
  }

  if (evalcache.size() > 1000000) {
    evalcache.clear();
  }

  // here goes a random movement
  chess::Board board(fen);

  // Need to implement this so that there is a key to access. board.hash()

  // Implement this so it is faster and better. std::unordered_map<uint64_t, int> evalcache

  chess::Movelist moves;
  chess::movegen::legalmoves(moves, board);
  if(moves.size() == 0)
    return "";

  return getBestMoveMCTS(fen, timeLimitMs);

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