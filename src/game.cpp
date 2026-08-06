#include "tetris/game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace tetris {
namespace {

using Shape = std::array<Point, 4>;
using Rotations = std::array<Shape, 4>;

constexpr std::array<Rotations, 7> Shapes{{
    // I
    {{{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
      {{{2, 0}, {2, 1}, {2, 2}, {2, 3}}},
      {{{0, 2}, {1, 2}, {2, 2}, {3, 2}}},
      {{{1, 0}, {1, 1}, {1, 2}, {1, 3}}}}},
    // O
    {{{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}}},
    // T
    {{{{{1, 0}, {0, 1}, {1, 1}, {2, 1}}},
      {{{1, 0}, {1, 1}, {2, 1}, {1, 2}}},
      {{{0, 1}, {1, 1}, {2, 1}, {1, 2}}},
      {{{1, 0}, {0, 1}, {1, 1}, {1, 2}}}}},
    // S
    {{{{{1, 0}, {2, 0}, {0, 1}, {1, 1}}},
      {{{1, 0}, {1, 1}, {2, 1}, {2, 2}}},
      {{{1, 1}, {2, 1}, {0, 2}, {1, 2}}},
      {{{0, 0}, {0, 1}, {1, 1}, {1, 2}}}}},
    // Z
    {{{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}},
      {{{2, 0}, {1, 1}, {2, 1}, {1, 2}}},
      {{{0, 1}, {1, 1}, {1, 2}, {2, 2}}},
      {{{1, 0}, {0, 1}, {1, 1}, {0, 2}}}}},
    // J
    {{{{{0, 0}, {0, 1}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {1, 2}}},
      {{{0, 1}, {1, 1}, {2, 1}, {2, 2}}},
      {{{1, 0}, {1, 1}, {0, 2}, {1, 2}}}}},
    // L
    {{{{{2, 0}, {0, 1}, {1, 1}, {2, 1}}},
      {{{1, 0}, {1, 1}, {1, 2}, {2, 2}}},
      {{{0, 1}, {1, 1}, {2, 1}, {0, 2}}},
      {{{0, 0}, {1, 0}, {1, 1}, {1, 2}}}}},
}};

constexpr std::size_t index(Piece piece) {
  return static_cast<std::size_t>(piece);
}

constexpr std::array<Point, 5> noKicks() {
  return {{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}};
}

// SRS kick coordinates converted to a screen coordinate system where +Y is
// down.
constexpr std::array<Point, 5> jlstzKicks(int from, int to) {
  if (from == 0 && to == 1)
    return {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}};
  if (from == 1 && to == 0)
    return {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}};
  if (from == 1 && to == 2)
    return {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}};
  if (from == 2 && to == 1)
    return {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}};
  if (from == 2 && to == 3)
    return {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}};
  if (from == 3 && to == 2)
    return {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}};
  if (from == 3 && to == 0)
    return {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}};
  if (from == 0 && to == 3)
    return {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}};
  return noKicks();
}

constexpr std::array<Point, 5> iKicks(int from, int to) {
  if (from == 0 && to == 1)
    return {{{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}}};
  if (from == 1 && to == 0)
    return {{{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}}};
  if (from == 1 && to == 2)
    return {{{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}};
  if (from == 2 && to == 1)
    return {{{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}}};
  if (from == 2 && to == 3)
    return {{{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}}};
  if (from == 3 && to == 2)
    return {{{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}}};
  if (from == 3 && to == 0)
    return {{{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}}};
  if (from == 0 && to == 3)
    return {{{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}};
  return noKicks();
}

constexpr std::array<Piece, 7> AllPieces{Piece::I, Piece::O, Piece::T, Piece::S,
                                         Piece::Z, Piece::J, Piece::L};

} // namespace

Game::Game(std::uint32_t seed) : random_(seed) { reset(seed); }

void Game::reset(std::uint32_t seed) {
  for (auto &row : board_) {
    row.fill(0);
  }
  queue_.clear();
  held_.reset();
  statistics_ = {};
  phase_ = Phase::Playing;
  random_.seed(seed);
  gravityAccumulatorMs_ = 0;
  lockAccumulatorMs_ = 0;
  lockResetCount_ = 0;
  canHold_ = true;
  lastActionWasRotation_ = false;
  refillQueue();
  spawn(takeNext());
}

std::array<Point, 4> Game::blocks(const ActivePiece &piece) const {
  auto result =
      Shapes[index(piece.type)][static_cast<std::size_t>(piece.rotation & 3)];
  for (auto &block : result) {
    block.x += piece.x;
    block.y += piece.y;
  }
  return result;
}

bool Game::collides(const ActivePiece &piece) const {
  for (const auto block : blocks(piece)) {
    if (block.x < 0 || block.x >= BoardWidth || block.y >= BoardHeight) {
      return true;
    }
    if (block.y >= 0 && board_[static_cast<std::size_t>(block.y)]
                              [static_cast<std::size_t>(block.x)] != 0) {
      return true;
    }
  }
  return false;
}

bool Game::grounded() const {
  auto candidate = active_;
  ++candidate.y;
  return collides(candidate);
}

void Game::resetLockDelayAfterMovement() {
  if (lockResetCount_ < 15) {
    lockAccumulatorMs_ = 0;
    ++lockResetCount_;
  }
}

bool Game::tryMove(int dx, int dy, bool playerMove) {
  const bool wasGrounded = grounded();
  auto candidate = active_;
  candidate.x += dx;
  candidate.y += dy;
  if (collides(candidate)) {
    return false;
  }

  active_ = candidate;
  if (playerMove) {
    lastActionWasRotation_ = false;
    if (dy == 0 && wasGrounded) {
      resetLockDelayAfterMovement();
    }
  }
  if (!grounded()) {
    lockAccumulatorMs_ = 0;
  }
  return true;
}

bool Game::tryRotate(int direction) {
  if (active_.type == Piece::O) {
    lastActionWasRotation_ = true;
    return true;
  }

  const bool wasGrounded = grounded();
  const int from = active_.rotation;
  const int to = (from + direction + 4) % 4;
  const auto tests =
      active_.type == Piece::I ? iKicks(from, to) : jlstzKicks(from, to);
  for (const auto kick : tests) {
    auto candidate = active_;
    candidate.rotation = to;
    candidate.x += kick.x;
    candidate.y += kick.y;
    if (!collides(candidate)) {
      active_ = candidate;
      lastActionWasRotation_ = true;
      if (wasGrounded) {
        resetLockDelayAfterMovement();
      }
      if (!grounded()) {
        lockAccumulatorMs_ = 0;
      }
      return true;
    }
  }
  return false;
}

bool Game::apply(Action action) {
  if (phase_ != Phase::Playing) {
    return false;
  }

  switch (action) {
  case Action::MoveLeft:
    return tryMove(-1, 0, true);
  case Action::MoveRight:
    return tryMove(1, 0, true);
  case Action::SoftDrop:
    if (tryMove(0, 1, true)) {
      ++statistics_.score;
      return true;
    }
    return false;
  case Action::HardDrop: {
    int distance = 0;
    while (true) {
      auto candidate = active_;
      ++candidate.y;
      if (collides(candidate)) {
        break;
      }
      active_ = candidate;
      ++distance;
    }
    statistics_.score += static_cast<std::uint64_t>(distance * 2);
    lockPiece();
    return true;
  }
  case Action::RotateClockwise:
    return tryRotate(1);
  case Action::RotateCounterClockwise:
    return tryRotate(-1);
  case Action::Hold: {
    if (!canHold_) {
      return false;
    }
    const Piece outgoing = active_.type;
    if (held_) {
      const Piece incoming = *held_;
      held_ = outgoing;
      spawn(incoming);
    } else {
      held_ = outgoing;
      spawn(takeNext());
    }
    canHold_ = false;
    return true;
  }
  }
  return false;
}

void Game::tick(std::uint32_t elapsedMs) {
  if (phase_ != Phase::Playing) {
    return;
  }

  gravityAccumulatorMs_ += elapsedMs;
  const auto interval = gravityIntervalMs();
  while (gravityAccumulatorMs_ >= interval && phase_ == Phase::Playing) {
    gravityAccumulatorMs_ -= interval;
    if (!tryMove(0, 1, false)) {
      gravityAccumulatorMs_ = 0;
      break;
    }
  }

  if (grounded()) {
    lockAccumulatorMs_ += elapsedMs;
    if (lockAccumulatorMs_ >= 500) {
      lockPiece();
    }
  } else {
    lockAccumulatorMs_ = 0;
  }
}

int Game::ghostY() const {
  auto ghost = active_;
  while (true) {
    auto candidate = ghost;
    ++candidate.y;
    if (collides(candidate)) {
      return ghost.y;
    }
    ghost = candidate;
  }
}

bool Game::isTSpin() const {
  if (active_.type != Piece::T || !lastActionWasRotation_) {
    return false;
  }

  const int centerX = active_.x + 1;
  const int centerY = active_.y + 1;
  const std::array<Point, 4> corners{{{centerX - 1, centerY - 1},
                                      {centerX + 1, centerY - 1},
                                      {centerX - 1, centerY + 1},
                                      {centerX + 1, centerY + 1}}};
  int occupied = 0;
  for (const auto corner : corners) {
    if (corner.x < 0 || corner.x >= BoardWidth || corner.y < 0 ||
        corner.y >= BoardHeight ||
        board_[static_cast<std::size_t>(corner.y)]
              [static_cast<std::size_t>(corner.x)] != 0) {
      ++occupied;
    }
  }
  return occupied >= 3;
}

void Game::lockPiece() {
  const bool tSpin = isTSpin();
  for (const auto block : activeBlocks()) {
    if (block.y < 0) {
      phase_ = Phase::GameOver;
      return;
    }
    board_[static_cast<std::size_t>(block.y)]
          [static_cast<std::size_t>(block.x)] =
              static_cast<std::uint8_t>(index(active_.type) + 1);
  }

  const int linesCleared = clearLines();
  scoreClear(linesCleared, tSpin);
  canHold_ = true;
  spawn(takeNext());
}

int Game::clearLines() {
  int writeRow = BoardHeight - 1;
  int cleared = 0;
  for (int readRow = BoardHeight - 1; readRow >= 0; --readRow) {
    const auto &row = board_[static_cast<std::size_t>(readRow)];
    const bool full = std::all_of(row.begin(), row.end(),
                                  [](std::uint8_t cell) { return cell != 0; });
    if (full) {
      ++cleared;
      continue;
    }
    if (writeRow != readRow) {
      board_[static_cast<std::size_t>(writeRow)] = row;
    }
    --writeRow;
  }
  while (writeRow >= 0) {
    board_[static_cast<std::size_t>(writeRow)].fill(0);
    --writeRow;
  }
  return cleared;
}

void Game::scoreClear(int linesCleared, bool tSpin) {
  const int level = statistics_.level;
  std::uint64_t base = 0;
  if (tSpin) {
    constexpr std::array<int, 4> TSpinScores{400, 800, 1200, 1600};
    base = static_cast<std::uint64_t>(
        TSpinScores[static_cast<std::size_t>(std::clamp(linesCleared, 0, 3))]);
  } else {
    constexpr std::array<int, 5> LineScores{0, 100, 300, 500, 800};
    base = static_cast<std::uint64_t>(
        LineScores[static_cast<std::size_t>(std::clamp(linesCleared, 0, 4))]);
  }

  const bool difficult = (linesCleared == 4) || (tSpin && linesCleared > 0);
  if (difficult && statistics_.backToBack) {
    base = base * 3 / 2;
  }
  if (difficult) {
    statistics_.backToBack = true;
  } else if (linesCleared > 0) {
    statistics_.backToBack = false;
  }

  if (linesCleared > 0) {
    ++statistics_.combo;
    if (statistics_.combo > 0) {
      base += static_cast<std::uint64_t>(50 * statistics_.combo);
    }
  } else {
    statistics_.combo = -1;
  }

  statistics_.score += base * static_cast<std::uint64_t>(level);
  statistics_.lines += linesCleared;
  statistics_.level = statistics_.lines / 10 + 1;
  statistics_.lastClearCount = linesCleared;
  statistics_.lastClearWasTSpin = tSpin;
}

void Game::spawn(Piece piece) {
  active_ = {piece, 0, 3, 0};
  gravityAccumulatorMs_ = 0;
  lockAccumulatorMs_ = 0;
  lockResetCount_ = 0;
  lastActionWasRotation_ = false;
  if (collides(active_)) {
    phase_ = Phase::GameOver;
  }
}

Piece Game::takeNext() {
  refillQueue();
  const Piece piece = queue_.front();
  queue_.pop_front();
  refillQueue();
  return piece;
}

void Game::refillQueue() {
  while (queue_.size() < 12) {
    auto bag = AllPieces;
    std::shuffle(bag.begin(), bag.end(), random_);
    for (const auto piece : bag) {
      queue_.push_back(piece);
    }
  }
}

std::vector<Piece> Game::nextPieces(std::size_t count) const {
  const auto actual = std::min(count, queue_.size());
  return {queue_.begin(), queue_.begin() + static_cast<std::ptrdiff_t>(actual)};
}

std::uint32_t Game::gravityIntervalMs() const {
  const double level = static_cast<double>(statistics_.level - 1);
  const double base = std::max(0.05, 0.8 - level * 0.007);
  const double seconds = std::pow(base, level);
  return static_cast<std::uint32_t>(std::clamp(seconds * 1000.0, 20.0, 1000.0));
}

const char *pieceName(Piece piece) {
  constexpr std::array<const char *, 7> Names{"I", "O", "T", "S",
                                              "Z", "J", "L"};
  return Names[index(piece)];
}

} // namespace tetris
