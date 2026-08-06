#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <vector>

namespace tetris {

constexpr int BoardWidth = 10;
constexpr int VisibleHeight = 20;
constexpr int HiddenRows = 2;
constexpr int BoardHeight = VisibleHeight + HiddenRows;

enum class Piece : std::uint8_t { I, O, T, S, Z, J, L };
enum class Action : std::uint8_t {
  MoveLeft,
  MoveRight,
  SoftDrop,
  HardDrop,
  RotateClockwise,
  RotateCounterClockwise,
  Hold,
};
enum class Phase : std::uint8_t { Playing, GameOver };

struct Point {
  int x{};
  int y{};
  friend constexpr bool operator==(const Point &, const Point &) = default;
};

struct ActivePiece {
  Piece type{Piece::I};
  int rotation{};
  int x{};
  int y{};
};

struct Statistics {
  std::uint64_t score{};
  int lines{};
  int level{1};
  int combo{-1};
  bool backToBack{};
  int lastClearCount{};
  bool lastClearWasTSpin{};
};

using Board = std::array<std::array<std::uint8_t, BoardWidth>, BoardHeight>;

class Game {
public:
  explicit Game(std::uint32_t seed = std::random_device{}());

  void reset(std::uint32_t seed = std::random_device{}());
  bool apply(Action action);
  void tick(std::uint32_t elapsedMs);

  [[nodiscard]] const Board &board() const { return board_; }
  [[nodiscard]] const ActivePiece &active() const { return active_; }
  [[nodiscard]] std::array<Point, 4> blocks(const ActivePiece &piece) const;
  [[nodiscard]] std::array<Point, 4> activeBlocks() const {
    return blocks(active_);
  }
  [[nodiscard]] int ghostY() const;
  [[nodiscard]] Phase phase() const { return phase_; }
  [[nodiscard]] const Statistics &statistics() const { return statistics_; }
  [[nodiscard]] std::optional<Piece> heldPiece() const { return held_; }
  [[nodiscard]] bool canHold() const { return canHold_; }
  [[nodiscard]] std::vector<Piece> nextPieces(std::size_t count = 5) const;

private:
  friend struct GameTestAccess;

  [[nodiscard]] bool collides(const ActivePiece &piece) const;
  [[nodiscard]] bool grounded() const;
  [[nodiscard]] bool tryMove(int dx, int dy, bool playerMove);
  [[nodiscard]] bool tryRotate(int direction);
  [[nodiscard]] bool isTSpin() const;
  void resetLockDelayAfterMovement();
  void lockPiece();
  int clearLines();
  void scoreClear(int linesCleared, bool tSpin);
  void spawn(Piece piece);
  Piece takeNext();
  void refillQueue();
  [[nodiscard]] std::uint32_t gravityIntervalMs() const;

  Board board_{};
  ActivePiece active_{};
  std::deque<Piece> queue_;
  std::optional<Piece> held_;
  Statistics statistics_{};
  Phase phase_{Phase::Playing};
  std::mt19937 random_;
  std::uint32_t gravityAccumulatorMs_{};
  std::uint32_t lockAccumulatorMs_{};
  int lockResetCount_{};
  bool canHold_{true};
  bool lastActionWasRotation_{};
};

[[nodiscard]] const char *pieceName(Piece piece);

} // namespace tetris
