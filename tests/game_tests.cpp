#include "tetris/game.hpp"
#include "tetris/persistence.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace tetris {

struct GameTestAccess {
  static void clearBoard(Game& game) {
    for (auto& row : game.board_) row.fill(0);
  }
  static void setCell(Game& game, int x, int y, std::uint8_t value = 1) {
    game.board_[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = value;
  }
  static void setActive(Game& game, ActivePiece piece) {
    game.active_ = piece;
    game.phase_ = Phase::Playing;
    game.lockAccumulatorMs_ = 0;
    game.lockResetCount_ = 0;
  }
  static void setLastRotation(Game& game, bool value) { game.lastActionWasRotation_ = value; }
  static bool collides(const Game& game, const ActivePiece& piece) { return game.collides(piece); }
  static bool isTSpin(const Game& game) { return game.isTSpin(); }
  static void lock(Game& game) { game.lockPiece(); }
  static void spawn(Game& game, Piece piece) { game.spawn(piece); }
  static void score(Game& game, int lines, bool tSpin) { game.scoreClear(lines, tSpin); }
};

}  // namespace tetris

namespace {

using tetris::Action;
using tetris::ActivePiece;
using tetris::BoardHeight;
using tetris::BoardWidth;
using tetris::Game;
using tetris::GameTestAccess;
using tetris::HighScoreStore;
using tetris::Phase;
using tetris::Piece;

int failures = 0;

void check(bool condition, std::string_view expression, std::string_view test, int line) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL " << test << ':' << line << " — " << expression << '\n';
  }
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __func__, __LINE__)

void sevenBagContainsEveryPieceOnce() {
  Game game(12345);
  std::vector<Piece> sequence;
  sequence.push_back(game.active().type);
  for (int i = 1; i < 14; ++i) {
    GameTestAccess::clearBoard(game);
    CHECK(game.apply(Action::HardDrop));
    sequence.push_back(game.active().type);
  }

  for (int bag = 0; bag < 2; ++bag) {
    const auto begin = sequence.begin() + bag * 7;
    const std::set<Piece> unique(begin, begin + 7);
    CHECK(unique.size() == 7);
  }
}

void seededGamesAreDeterministic() {
  Game first(88);
  Game second(88);
  CHECK(first.active().type == second.active().type);
  CHECK(first.nextPieces(12) == second.nextPieces(12));
  first.tick(1000);
  second.tick(1000);
  CHECK(first.active().y == second.active().y);
}

void holdWorksOncePerPiece() {
  Game game(7);
  const Piece original = game.active().type;
  const Piece next = game.nextPieces(1).front();
  CHECK(game.apply(Action::Hold));
  CHECK(game.heldPiece() == original);
  CHECK(game.active().type == next);
  CHECK(!game.apply(Action::Hold));

  GameTestAccess::clearBoard(game);
  CHECK(game.apply(Action::HardDrop));
  CHECK(game.canHold());
  CHECK(game.apply(Action::Hold));
  CHECK(game.active().type == original);
}

void ghostStopsAtFloor() {
  Game game(1);
  GameTestAccess::clearBoard(game);
  GameTestAccess::setActive(game, {Piece::I, 0, 3, 0});
  CHECK(game.ghostY() == 20);
}

void hardDropLocksAndAwardsDistance() {
  Game game(1);
  GameTestAccess::clearBoard(game);
  GameTestAccess::setActive(game, {Piece::O, 0, 3, 0});
  CHECK(game.apply(Action::HardDrop));
  CHECK(game.statistics().score == 40);
  CHECK(game.board()[20][4] != 0);
  CHECK(game.board()[21][5] != 0);
}

void lineClearCompactsAndScores() {
  Game game(2);
  GameTestAccess::clearBoard(game);
  for (int x = 0; x < BoardWidth - 1; ++x) {
    GameTestAccess::setCell(game, x, BoardHeight - 1);
  }
  GameTestAccess::setCell(game, 0, BoardHeight - 2, 7);
  GameTestAccess::setActive(game, {Piece::I, 1, 7, BoardHeight - 4});
  GameTestAccess::lock(game);
  CHECK(game.statistics().lines == 1);
  CHECK(game.statistics().score == 100);
  CHECK(game.board()[BoardHeight - 1][0] == 7);
}

void tetrisAndBackToBackScoring() {
  Game game(3);
  GameTestAccess::score(game, 4, false);
  CHECK(game.statistics().score == 800);
  CHECK(game.statistics().backToBack);
  GameTestAccess::score(game, 4, false);
  CHECK(game.statistics().score == 2050);
  CHECK(game.statistics().combo == 1);
}

void tSpinUsesThreeCornerRule() {
  Game game(4);
  GameTestAccess::clearBoard(game);
  GameTestAccess::setActive(game, {Piece::T, 0, 3, 10});
  GameTestAccess::setCell(game, 3, 10);
  GameTestAccess::setCell(game, 5, 10);
  GameTestAccess::setCell(game, 3, 12);
  GameTestAccess::setLastRotation(game, true);
  CHECK(GameTestAccess::isTSpin(game));
  GameTestAccess::setLastRotation(game, false);
  CHECK(!GameTestAccess::isTSpin(game));
}

void rotationKicksAwayFromWall() {
  Game game(5);
  GameTestAccess::clearBoard(game);
  GameTestAccess::setActive(game, {Piece::T, 1, -1, 8});
  CHECK(!GameTestAccess::collides(game, game.active()));
  CHECK(game.apply(Action::RotateCounterClockwise));
  for (const auto block : game.activeBlocks()) {
    CHECK(block.x >= 0);
    CHECK(block.x < BoardWidth);
  }
}

void lockDelayIsFiveHundredMilliseconds() {
  Game game(6);
  GameTestAccess::clearBoard(game);
  GameTestAccess::setActive(game, {Piece::O, 0, 3, 20});
  game.tick(499);
  CHECK(game.board()[21][4] == 0);
  game.tick(1);
  CHECK(game.board()[21][4] != 0);
}

void blockedSpawnEndsGame() {
  Game game(9);
  GameTestAccess::clearBoard(game);
  GameTestAccess::setCell(game, 4, 0);
  GameTestAccess::spawn(game, Piece::O);
  CHECK(game.phase() == Phase::GameOver);
}

void persistenceHandlesMissingCorruptAndValidData() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto directory = std::filesystem::temp_directory_path() /
                         ("tetris-tests-" + std::to_string(stamp));
  const auto path = directory / "highscore.txt";
  const HighScoreStore store(path);
  CHECK(store.load() == 0);
  CHECK(store.save(987654));
  CHECK(store.load() == 987654);
  {
    std::ofstream corrupt(path, std::ios::trunc);
    corrupt << "not-a-score\n";
  }
  CHECK(store.load() == 0);
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

}  // namespace

int main() {
  sevenBagContainsEveryPieceOnce();
  seededGamesAreDeterministic();
  holdWorksOncePerPiece();
  ghostStopsAtFloor();
  hardDropLocksAndAwardsDistance();
  lineClearCompactsAndScores();
  tetrisAndBackToBackScoring();
  tSpinUsesThreeCornerRule();
  rotationKicksAwayFromWall();
  lockDelayIsFiveHundredMilliseconds();
  blockedSpawnEndsGame();
  persistenceHandlesMissingCorruptAndValidData();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "All Tetris core tests passed\n";
  return 0;
}
