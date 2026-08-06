#include "tetris/game.hpp"
#include "tetris/persistence.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int LogicalWidth = 960;
constexpr int LogicalHeight = 720;
constexpr int CellSize = 30;
constexpr int BoardX = 330;
constexpr int BoardY = 60;
constexpr double FixedStepMs = 1000.0 / 60.0;

struct Color {
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};
  std::uint8_t a{255};
};

constexpr Color Background{7, 10, 24};
constexpr Color Panel{17, 23, 48, 235};
constexpr Color PanelBorder{55, 72, 126};
constexpr Color PrimaryText{235, 241, 255};
constexpr Color SecondaryText{139, 153, 195};
constexpr Color Accent{142, 92, 246};
constexpr std::array<Color, 8> PieceColors{{{0, 0, 0},
                                            {45, 220, 235},
                                            {250, 218, 65},
                                            {174, 92, 246},
                                            {69, 218, 125},
                                            {246, 72, 101},
                                            {68, 117, 246},
                                            {250, 151, 61}}};

void setColor(SDL_Renderer *renderer, Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fillRect(SDL_Renderer *renderer, float x, float y, float w, float h,
              Color color) {
  const SDL_FRect rectangle{x, y, w, h};
  setColor(renderer, color);
  SDL_RenderFillRect(renderer, &rectangle);
}

void strokeRect(SDL_Renderer *renderer, float x, float y, float w, float h,
                Color color) {
  const SDL_FRect rectangle{x, y, w, h};
  setColor(renderer, color);
  SDL_RenderRect(renderer, &rectangle);
}

std::array<std::uint8_t, 7> glyph(char character) {
  switch (character) {
  case 'A':
    return {14, 17, 17, 31, 17, 17, 17};
  case 'B':
    return {30, 17, 17, 30, 17, 17, 30};
  case 'C':
    return {14, 17, 16, 16, 16, 17, 14};
  case 'D':
    return {30, 17, 17, 17, 17, 17, 30};
  case 'E':
    return {31, 16, 16, 30, 16, 16, 31};
  case 'F':
    return {31, 16, 16, 30, 16, 16, 16};
  case 'G':
    return {14, 17, 16, 23, 17, 17, 14};
  case 'H':
    return {17, 17, 17, 31, 17, 17, 17};
  case 'I':
    return {31, 4, 4, 4, 4, 4, 31};
  case 'J':
    return {7, 2, 2, 2, 2, 18, 12};
  case 'K':
    return {17, 18, 20, 24, 20, 18, 17};
  case 'L':
    return {16, 16, 16, 16, 16, 16, 31};
  case 'M':
    return {17, 27, 21, 21, 17, 17, 17};
  case 'N':
    return {17, 25, 21, 19, 17, 17, 17};
  case 'O':
    return {14, 17, 17, 17, 17, 17, 14};
  case 'P':
    return {30, 17, 17, 30, 16, 16, 16};
  case 'Q':
    return {14, 17, 17, 17, 21, 18, 13};
  case 'R':
    return {30, 17, 17, 30, 20, 18, 17};
  case 'S':
    return {15, 16, 16, 14, 1, 1, 30};
  case 'T':
    return {31, 4, 4, 4, 4, 4, 4};
  case 'U':
    return {17, 17, 17, 17, 17, 17, 14};
  case 'V':
    return {17, 17, 17, 17, 17, 10, 4};
  case 'W':
    return {17, 17, 17, 21, 21, 21, 10};
  case 'X':
    return {17, 17, 10, 4, 10, 17, 17};
  case 'Y':
    return {17, 17, 10, 4, 4, 4, 4};
  case 'Z':
    return {31, 1, 2, 4, 8, 16, 31};
  case '0':
    return {14, 17, 19, 21, 25, 17, 14};
  case '1':
    return {4, 12, 4, 4, 4, 4, 14};
  case '2':
    return {14, 17, 1, 2, 4, 8, 31};
  case '3':
    return {30, 1, 1, 14, 1, 1, 30};
  case '4':
    return {2, 6, 10, 18, 31, 2, 2};
  case '5':
    return {31, 16, 16, 30, 1, 1, 30};
  case '6':
    return {14, 16, 16, 30, 17, 17, 14};
  case '7':
    return {31, 1, 2, 4, 8, 8, 8};
  case '8':
    return {14, 17, 17, 14, 17, 17, 14};
  case '9':
    return {14, 17, 17, 15, 1, 1, 14};
  case ':':
    return {0, 4, 4, 0, 4, 4, 0};
  case '-':
    return {0, 0, 0, 31, 0, 0, 0};
  case '/':
    return {1, 2, 2, 4, 8, 8, 16};
  case '.':
    return {0, 0, 0, 0, 0, 12, 12};
  case '+':
    return {0, 4, 4, 31, 4, 4, 0};
  case '?':
    return {14, 17, 1, 2, 4, 0, 4};
  default:
    return {0, 0, 0, 0, 0, 0, 0};
  }
}

float textWidth(std::string_view text, float scale) {
  if (text.empty())
    return 0.0F;
  return static_cast<float>(text.size() * 6 - 1) * scale;
}

void drawText(SDL_Renderer *renderer, std::string_view text, float x, float y,
              float scale, Color color) {
  for (const char raw : text) {
    const char character =
        raw >= 'a' && raw <= 'z' ? static_cast<char>(raw - 'a' + 'A') : raw;
    const auto rows = glyph(character);
    for (std::size_t row = 0; row < rows.size(); ++row) {
      for (int column = 0; column < 5; ++column) {
        if ((rows[row] & static_cast<std::uint8_t>(1U << (4 - column))) != 0) {
          fillRect(renderer, x + static_cast<float>(column) * scale,
                   y + static_cast<float>(row) * scale, scale, scale, color);
        }
      }
    }
    x += 6.0F * scale;
  }
}

void drawTextCentered(SDL_Renderer *renderer, std::string_view text,
                      float centerX, float y, float scale, Color color) {
  drawText(renderer, text, centerX - textWidth(text, scale) / 2.0F, y, scale,
           color);
}

Color brighten(Color color, int amount) {
  const auto add = [amount](std::uint8_t value) {
    return static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(value) + amount, 0, 255));
  };
  return {add(color.r), add(color.g), add(color.b), color.a};
}

void drawBlock(SDL_Renderer *renderer, float x, float y, float size,
               Color color, std::uint8_t alpha = 255) {
  color.a = alpha;
  auto dark = brighten(color, -65);
  dark.a = alpha;
  auto light = brighten(color, 45);
  light.a = alpha;
  fillRect(renderer, x + 1, y + 1, size - 2, size - 2, dark);
  fillRect(renderer, x + 3, y + 3, size - 6, size - 6, color);
  fillRect(renderer, x + 4, y + 4, size - 8, 3, light);
  fillRect(renderer, x + 4, y + 7, 3, size - 11, light);
}

void drawPanel(SDL_Renderer *renderer, float x, float y, float w, float h) {
  fillRect(renderer, x, y, w, h, Panel);
  strokeRect(renderer, x, y, w, h, PanelBorder);
  strokeRect(renderer, x + 3, y + 3, w - 6, h - 6, {31, 40, 79});
}

void drawPiecePreview(SDL_Renderer *renderer, const tetris::Game &game,
                      tetris::Piece piece, float centerX, float centerY,
                      float size) {
  const auto blocks = game.blocks({piece, 0, 0, 0});
  int minX = 4;
  int maxX = 0;
  int minY = 4;
  int maxY = 0;
  for (const auto block : blocks) {
    minX = std::min(minX, block.x);
    maxX = std::max(maxX, block.x);
    minY = std::min(minY, block.y);
    maxY = std::max(maxY, block.y);
  }
  const float width = static_cast<float>(maxX - minX + 1) * size;
  const float height = static_cast<float>(maxY - minY + 1) * size;
  const float originX =
      centerX - width / 2.0F - static_cast<float>(minX) * size;
  const float originY =
      centerY - height / 2.0F - static_cast<float>(minY) * size;
  const auto color = PieceColors[static_cast<std::size_t>(piece) + 1];
  for (const auto block : blocks) {
    drawBlock(renderer, originX + static_cast<float>(block.x) * size,
              originY + static_cast<float>(block.y) * size, size, color);
  }
}

class SoundBank {
public:
  SoundBank() {
    SDL_AudioSpec specification{};
    specification.format = SDL_AUDIO_F32;
    specification.channels = 1;
    specification.freq = 48000;
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        &specification, nullptr, nullptr);
    if (stream_ != nullptr) {
      SDL_ResumeAudioStreamDevice(stream_);
    }
  }

  ~SoundBank() { shutdown(); }

  SoundBank(const SoundBank &) = delete;
  SoundBank &operator=(const SoundBank &) = delete;

  void toggle() { muted_ = !muted_; }
  [[nodiscard]] bool muted() const { return muted_; }
  [[nodiscard]] bool available() const { return stream_ != nullptr; }

  void shutdown() {
    if (stream_ != nullptr) {
      SDL_DestroyAudioStream(stream_);
      stream_ = nullptr;
    }
  }

  void tone(float frequency, int durationMs, float volume = 0.12F,
            float slide = 0.0F) {
    if (muted_ || stream_ == nullptr)
      return;
    const int sampleCount = 48000 * durationMs / 1000;
    std::vector<float> samples(static_cast<std::size_t>(sampleCount));
    double phase = 0.0;
    for (int sample = 0; sample < sampleCount; ++sample) {
      const float progress = static_cast<float>(sample) /
                             static_cast<float>(std::max(1, sampleCount));
      const float envelope =
          (1.0F - progress) * std::min(1.0F, progress * 12.0F);
      const float currentFrequency = frequency + slide * progress;
      phase +=
          6.283185307179586 * static_cast<double>(currentFrequency) / 48000.0;
      const float wave =
          static_cast<float>(std::sin(phase) * 0.72 +
                             (std::sin(phase * 2.0) > 0.0 ? 0.28 : -0.28));
      samples[static_cast<std::size_t>(sample)] = wave * volume * envelope;
    }
    SDL_PutAudioStreamData(stream_, samples.data(),
                           static_cast<int>(samples.size() * sizeof(float)));
  }

private:
  SDL_AudioStream *stream_{};
  bool muted_{};
};

enum class Screen { Title, Playing, Paused, GameOver };

struct RepeatButton {
  bool held{};
  float elapsed{};
  float sinceRepeat{};

  void press() {
    held = true;
    elapsed = 0.0F;
    sinceRepeat = 0.0F;
  }
  void release() { held = false; }
};

struct Particle {
  float x{};
  float y{};
  float vx{};
  float vy{};
  float life{};
  Color color{};
};

struct App {
  SDL_Window *window{};
  SDL_Renderer *renderer{};
  SDL_Gamepad *gamepad{};
  tetris::Game game{std::random_device{}()};
  Screen screen{Screen::Title};
  SoundBank sound;
  RepeatButton left;
  RepeatButton right;
  RepeatButton down;
  std::vector<Particle> particles;
  std::mt19937 effectsRandom{std::random_device{}()};
  std::uint64_t highScore{};
  std::unique_ptr<tetris::HighScoreStore> highScoreStore;
  int observedLines{};
  int observedLevel{1};
  float clearFlash{};
  bool running{true};

  void startGame() {
    game.reset(std::random_device{}());
    screen = Screen::Playing;
    observedLines = 0;
    observedLevel = 1;
    particles.clear();
    clearFlash = 0.0F;
    sound.tone(330.0F, 70, 0.12F, 220.0F);
  }

  void saveHighScore() {
    if (game.statistics().score > highScore) {
      highScore = game.statistics().score;
    }
    if (highScoreStore && !highScoreStore->save(highScore)) {
      std::cerr << "Could not save high score to " << highScoreStore->path()
                << '\n';
    }
  }

  void checkGameEvents() {
    const auto &stats = game.statistics();
    if (stats.lines > observedLines) {
      const int cleared = stats.lines - observedLines;
      clearFlash = 0.22F;
      std::uniform_real_distribution<float> velocityX(-160.0F, 160.0F);
      std::uniform_real_distribution<float> velocityY(-230.0F, -60.0F);
      std::uniform_real_distribution<float> locationX(
          static_cast<float>(BoardX),
          static_cast<float>(BoardX + tetris::BoardWidth * CellSize));
      for (int i = 0; i < 18 * cleared; ++i) {
        const auto color =
            PieceColors[static_cast<std::size_t>((i + cleared) % 7 + 1)];
        particles.push_back(
            {locationX(effectsRandom), static_cast<float>(BoardY + 360),
             velocityX(effectsRandom), velocityY(effectsRandom), 0.75F, color});
      }
      sound.tone(440.0F + static_cast<float>(cleared) * 90.0F,
                 120 + cleared * 25, 0.17F, 240.0F);
      observedLines = stats.lines;
    }
    if (stats.level > observedLevel) {
      sound.tone(660.0F, 180, 0.16F, 440.0F);
      observedLevel = stats.level;
    }
    if (stats.score > highScore)
      highScore = stats.score;
    if (game.phase() == tetris::Phase::GameOver && screen == Screen::Playing) {
      screen = Screen::GameOver;
      sound.tone(230.0F, 360, 0.18F, -170.0F);
      saveHighScore();
    }
  }

  bool action(tetris::Action actionValue) {
    if (screen != Screen::Playing)
      return false;
    const bool applied = game.apply(actionValue);
    if (applied) {
      switch (actionValue) {
      case tetris::Action::RotateClockwise:
      case tetris::Action::RotateCounterClockwise:
        sound.tone(315.0F, 35, 0.055F, 90.0F);
        break;
      case tetris::Action::HardDrop:
        sound.tone(105.0F, 55, 0.11F, -25.0F);
        break;
      case tetris::Action::Hold:
        sound.tone(245.0F, 65, 0.08F, 120.0F);
        break;
      case tetris::Action::MoveLeft:
      case tetris::Action::MoveRight:
        sound.tone(165.0F, 18, 0.025F);
        break;
      case tetris::Action::SoftDrop:
        break;
      }
    }
    checkGameEvents();
    return applied;
  }
};

void openFirstGamepad(App &app) {
  if (app.gamepad != nullptr)
    return;
  int count = 0;
  SDL_JoystickID *gamepads = SDL_GetGamepads(&count);
  if (gamepads != nullptr && count > 0) {
    app.gamepad = SDL_OpenGamepad(gamepads[0]);
  }
  SDL_free(gamepads);
}

void handleKeyDown(App &app, SDL_Scancode key, bool repeat) {
  if (key == SDL_SCANCODE_M && !repeat) {
    app.sound.toggle();
    return;
  }
  if (key == SDL_SCANCODE_ESCAPE && !repeat) {
    if (app.screen == Screen::Playing)
      app.screen = Screen::Paused;
    else if (app.screen == Screen::Paused)
      app.screen = Screen::Playing;
    else if (app.screen == Screen::Title)
      app.running = false;
    return;
  }
  if (key == SDL_SCANCODE_P && !repeat) {
    if (app.screen == Screen::Playing)
      app.screen = Screen::Paused;
    else if (app.screen == Screen::Paused)
      app.screen = Screen::Playing;
    return;
  }
  if ((key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_SPACE) && !repeat &&
      (app.screen == Screen::Title || app.screen == Screen::GameOver)) {
    app.startGame();
    return;
  }
  if (key == SDL_SCANCODE_R && !repeat && app.screen == Screen::GameOver) {
    app.startGame();
    return;
  }
  if (app.screen != Screen::Playing)
    return;

  if (key == SDL_SCANCODE_LEFT && !repeat) {
    app.left.press();
    app.action(tetris::Action::MoveLeft);
  } else if (key == SDL_SCANCODE_RIGHT && !repeat) {
    app.right.press();
    app.action(tetris::Action::MoveRight);
  } else if (key == SDL_SCANCODE_DOWN && !repeat) {
    app.down.press();
    app.action(tetris::Action::SoftDrop);
  } else if (key == SDL_SCANCODE_Z && !repeat) {
    app.action(tetris::Action::RotateCounterClockwise);
  } else if ((key == SDL_SCANCODE_X || key == SDL_SCANCODE_UP) && !repeat) {
    app.action(tetris::Action::RotateClockwise);
  } else if (key == SDL_SCANCODE_SPACE && !repeat) {
    app.action(tetris::Action::HardDrop);
  } else if ((key == SDL_SCANCODE_C || key == SDL_SCANCODE_LSHIFT) && !repeat) {
    app.action(tetris::Action::Hold);
  }
}

void handleKeyUp(App &app, SDL_Scancode key) {
  if (key == SDL_SCANCODE_LEFT)
    app.left.release();
  if (key == SDL_SCANCODE_RIGHT)
    app.right.release();
  if (key == SDL_SCANCODE_DOWN)
    app.down.release();
}

void handleGamepadButton(App &app, SDL_GamepadButton button, bool down) {
  auto setRepeat = [down](RepeatButton &state) {
    if (down)
      state.press();
    else
      state.release();
  };
  if (button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) {
    setRepeat(app.left);
    if (down)
      app.action(tetris::Action::MoveLeft);
  } else if (button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
    setRepeat(app.right);
    if (down)
      app.action(tetris::Action::MoveRight);
  } else if (button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
    setRepeat(app.down);
    if (down)
      app.action(tetris::Action::SoftDrop);
  } else if (!down) {
    return;
  } else if (button == SDL_GAMEPAD_BUTTON_SOUTH) {
    if (app.screen == Screen::Title || app.screen == Screen::GameOver)
      app.startGame();
    else
      app.action(tetris::Action::RotateClockwise);
  } else if (button == SDL_GAMEPAD_BUTTON_WEST) {
    app.action(tetris::Action::RotateCounterClockwise);
  } else if (button == SDL_GAMEPAD_BUTTON_NORTH) {
    app.action(tetris::Action::Hold);
  } else if (button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
    app.action(tetris::Action::HardDrop);
  } else if (button == SDL_GAMEPAD_BUTTON_START) {
    if (app.screen == Screen::Playing)
      app.screen = Screen::Paused;
    else if (app.screen == Screen::Paused)
      app.screen = Screen::Playing;
  }
}

void updateRepeat(App &app, RepeatButton &button, tetris::Action action,
                  float deltaMs, float delay, float interval) {
  if (!button.held || app.screen != Screen::Playing)
    return;
  button.elapsed += deltaMs;
  if (button.elapsed < delay)
    return;
  button.sinceRepeat += deltaMs;
  while (button.sinceRepeat >= interval) {
    app.action(action);
    button.sinceRepeat -= interval;
  }
}

void updateParticles(App &app, float deltaSeconds) {
  for (auto &particle : app.particles) {
    particle.life -= deltaSeconds;
    particle.x += particle.vx * deltaSeconds;
    particle.y += particle.vy * deltaSeconds;
    particle.vy += 420.0F * deltaSeconds;
  }
  std::erase_if(app.particles,
                [](const Particle &particle) { return particle.life <= 0.0F; });
  app.clearFlash = std::max(0.0F, app.clearFlash - deltaSeconds);
}

void drawBackground(SDL_Renderer *renderer, std::uint64_t ticks) {
  setColor(renderer, Background);
  SDL_RenderClear(renderer);
  const float drift = static_cast<float>((ticks / 40) % 48);
  for (int x = -LogicalHeight; x < LogicalWidth + LogicalHeight; x += 48) {
    setColor(renderer, {20, 27, 57, 110});
    SDL_RenderLine(renderer, static_cast<float>(x) + drift, 0.0F,
                   static_cast<float>(x - LogicalHeight) + drift,
                   static_cast<float>(LogicalHeight));
  }
  fillRect(renderer, 0, 0, LogicalWidth, LogicalHeight, {7, 10, 24, 150});
}

void drawBoard(App &app) {
  auto *renderer = app.renderer;
  fillRect(renderer, BoardX - 9, BoardY - 9, tetris::BoardWidth * CellSize + 18,
           tetris::VisibleHeight * CellSize + 18, {9, 13, 30});
  strokeRect(renderer, BoardX - 10, BoardY - 10,
             tetris::BoardWidth * CellSize + 20,
             tetris::VisibleHeight * CellSize + 20, Accent);

  for (int y = 0; y < tetris::VisibleHeight; ++y) {
    for (int x = 0; x < tetris::BoardWidth; ++x) {
      const float drawX = static_cast<float>(BoardX + x * CellSize);
      const float drawY = static_cast<float>(BoardY + y * CellSize);
      fillRect(renderer, drawX + 1, drawY + 1, CellSize - 2, CellSize - 2,
               {13, 18, 39});
      const auto cell =
          app.game.board()[static_cast<std::size_t>(y + tetris::HiddenRows)]
                          [static_cast<std::size_t>(x)];
      if (cell != 0)
        drawBlock(renderer, drawX, drawY, CellSize, PieceColors[cell]);
    }
  }

  auto ghost = app.game.active();
  ghost.y = app.game.ghostY();
  const auto activeColor =
      PieceColors[static_cast<std::size_t>(app.game.active().type) + 1];
  for (const auto block : app.game.blocks(ghost)) {
    if (block.y >= tetris::HiddenRows) {
      drawBlock(renderer, static_cast<float>(BoardX + block.x * CellSize),
                static_cast<float>(BoardY +
                                   (block.y - tetris::HiddenRows) * CellSize),
                CellSize, activeColor, 65);
    }
  }
  for (const auto block : app.game.activeBlocks()) {
    if (block.y >= tetris::HiddenRows) {
      drawBlock(renderer, static_cast<float>(BoardX + block.x * CellSize),
                static_cast<float>(BoardY +
                                   (block.y - tetris::HiddenRows) * CellSize),
                CellSize, activeColor);
    }
  }

  if (app.clearFlash > 0.0F) {
    const auto alpha = static_cast<std::uint8_t>(
        std::clamp(app.clearFlash * 520.0F, 0.0F, 115.0F));
    fillRect(renderer, BoardX, BoardY, tetris::BoardWidth * CellSize,
             tetris::VisibleHeight * CellSize, {255, 255, 255, alpha});
  }
}

void drawHud(App &app) {
  auto *renderer = app.renderer;
  drawPanel(renderer, 65, 60, 220, 164);
  drawText(renderer, "HOLD", 88, 82, 3, SecondaryText);
  if (app.game.heldPiece()) {
    drawPiecePreview(renderer, app.game, *app.game.heldPiece(), 175, 158, 23);
  } else {
    drawTextCentered(renderer, "EMPTY", 175, 151, 2, {74, 86, 123});
  }

  drawPanel(renderer, 65, 246, 220, 259);
  const auto &stats = app.game.statistics();
  drawText(renderer, "SCORE", 88, 270, 2, SecondaryText);
  drawText(renderer, std::to_string(stats.score), 88, 298, 3, PrimaryText);
  drawText(renderer, "HIGH", 88, 346, 2, SecondaryText);
  drawText(renderer, std::to_string(std::max(app.highScore, stats.score)), 88,
           374, 3, PrimaryText);
  drawText(renderer, "LEVEL", 88, 422, 2, SecondaryText);
  drawText(renderer, std::to_string(stats.level), 88, 450, 3, {69, 218, 206});
  drawText(renderer, "LINES", 175, 422, 2, SecondaryText);
  drawText(renderer, std::to_string(stats.lines), 175, 450, 3, {250, 218, 65});

  drawPanel(renderer, 675, 60, 220, 445);
  drawText(renderer, "NEXT", 698, 82, 3, SecondaryText);
  const auto next = app.game.nextPieces(5);
  for (std::size_t i = 0; i < next.size(); ++i) {
    drawPiecePreview(renderer, app.game, next[i], 785,
                     143.0F + static_cast<float>(i) * 72.0F,
                     i == 0 ? 20.0F : 16.0F);
  }

  drawPanel(renderer, 65, 527, 220, 133);
  drawText(renderer, "CONTROLS", 88, 548, 2, SecondaryText);
  drawText(renderer, "ARROWS MOVE", 88, 576, 1.5F, PrimaryText);
  drawText(renderer, "Z X ROTATE", 88, 598, 1.5F, PrimaryText);
  drawText(renderer, "SPACE DROP", 88, 620, 1.5F, PrimaryText);
  drawText(renderer, "C HOLD  P PAUSE", 88, 642, 1.25F, PrimaryText);

  drawPanel(renderer, 675, 527, 220, 133);
  drawText(renderer, "STATUS", 698, 548, 2, SecondaryText);
  drawText(renderer, app.sound.muted() ? "SOUND OFF" : "SOUND ON", 698, 580,
           1.75F,
           app.sound.muted() ? Color{246, 72, 101} : Color{69, 218, 125});
  drawText(renderer, "M TO TOGGLE", 698, 610, 1.4F, PrimaryText);
  if (stats.combo > 0) {
    drawText(renderer, "COMBO " + std::to_string(stats.combo + 1), 698, 636,
             1.4F, {250, 151, 61});
  } else if (stats.backToBack) {
    drawText(renderer, "BACK TO BACK", 698, 636, 1.25F, {250, 151, 61});
  }
}

void drawParticles(App &app) {
  for (const auto &particle : app.particles) {
    auto color = particle.color;
    color.a = static_cast<std::uint8_t>(
        std::clamp(particle.life * 340.0F, 0.0F, 255.0F));
    fillRect(app.renderer, particle.x, particle.y, 7, 7, color);
  }
}

void drawOverlay(App &app, std::string_view title, std::string_view prompt) {
  fillRect(app.renderer, 0, 0, LogicalWidth, LogicalHeight, {4, 6, 17, 190});
  drawPanel(app.renderer, 230, 245, 500, 230);
  drawTextCentered(app.renderer, title, 480, 284, 6, PrimaryText);
  drawTextCentered(app.renderer, prompt, 480, 380, 2.25F, {69, 218, 206});
  drawTextCentered(app.renderer, "ESC TO RESUME  M TOGGLE SOUND", 480, 425,
                   1.35F, SecondaryText);
}

void drawTitle(App &app, std::uint64_t ticks) {
  const float pulse =
      0.5F +
      0.5F * static_cast<float>(std::sin(static_cast<double>(ticks) / 450.0));
  drawTextCentered(app.renderer, "TETRIS", 480, 135, 12, {174, 92, 246});
  drawTextCentered(app.renderer, "CLONE", 480, 240, 6, {69, 218, 206});

  const std::array<tetris::Piece, 7> pieces{
      tetris::Piece::I, tetris::Piece::O, tetris::Piece::T, tetris::Piece::S,
      tetris::Piece::Z, tetris::Piece::J, tetris::Piece::L};
  for (std::size_t i = 0; i < pieces.size(); ++i) {
    drawPiecePreview(app.renderer, app.game, pieces[i],
                     174.0F + static_cast<float>(i) * 102.0F,
                     355.0F + static_cast<float>((i % 2) * 18), 16.0F);
  }

  const auto promptColor =
      brighten(Color{69, 218, 206}, static_cast<int>(pulse * 45.0F));
  drawTextCentered(app.renderer, "PRESS ENTER OR GAMEPAD A", 480, 474, 3,
                   promptColor);
  drawTextCentered(app.renderer, "MODERN FALLING BLOCKS", 480, 535, 2,
                   SecondaryText);
  drawTextCentered(app.renderer, "HIGH SCORE " + std::to_string(app.highScore),
                   480, 582, 2, PrimaryText);
  drawTextCentered(app.renderer, "ESC QUIT   M SOUND", 480, 642, 1.5F,
                   SecondaryText);
}

void render(App &app) {
  const auto ticks = SDL_GetTicks();
  drawBackground(app.renderer, ticks);
  if (app.screen == Screen::Title) {
    drawTitle(app, ticks);
  } else {
    drawBoard(app);
    drawHud(app);
    drawParticles(app);
    if (app.screen == Screen::Paused) {
      drawOverlay(app, "PAUSED", "PRESS P OR START");
    } else if (app.screen == Screen::GameOver) {
      drawOverlay(app, "GAME OVER", "PRESS ENTER TO RESTART");
    }
  }
  SDL_RenderPresent(app.renderer);
}

std::filesystem::path preferencePath() {
  char *rawPath = SDL_GetPrefPath("OpenAI", "TetrisClone");
  if (rawPath == nullptr)
    return "highscore.txt";
  const std::filesystem::path result(rawPath);
  SDL_free(rawPath);
  return result / "highscore.txt";
}

} // namespace

int main(int argc, char **argv) {
  int smokeTestFrames = -1;
  for (int argument = 1; argument < argc; ++argument) {
    if (std::string_view(argv[argument]) == "--smoke-test")
      smokeTestFrames = 3;
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    std::cerr << "Could not initialize SDL: " << SDL_GetError() << '\n';
    return EXIT_FAILURE;
  }
  SDL_InitSubSystem(SDL_INIT_AUDIO);

  App app;
  if (!SDL_CreateWindowAndRenderer("Tetris Clone", LogicalWidth, LogicalHeight,
                                   SDL_WINDOW_RESIZABLE, &app.window,
                                   &app.renderer)) {
    std::cerr << "Could not create the SDL window: " << SDL_GetError() << '\n';
    SDL_Quit();
    return EXIT_FAILURE;
  }
  SDL_SetRenderVSync(app.renderer, 1);
  SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderLogicalPresentation(app.renderer, LogicalWidth, LogicalHeight,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetWindowMinimumSize(app.window, 640, 480);

  app.highScoreStore =
      std::make_unique<tetris::HighScoreStore>(preferencePath());
  app.highScore = app.highScoreStore->load();
  openFirstGamepad(app);

  std::uint64_t previousTicks = SDL_GetTicks();
  double simulationAccumulator = 0.0;
  int millisecondRemainder = 0;

  while (app.running) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        app.running = false;
        break;
      case SDL_EVENT_KEY_DOWN:
        handleKeyDown(app, event.key.scancode, event.key.repeat);
        break;
      case SDL_EVENT_KEY_UP:
        handleKeyUp(app, event.key.scancode);
        break;
      case SDL_EVENT_GAMEPAD_ADDED:
        openFirstGamepad(app);
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        if (app.gamepad != nullptr &&
            SDL_GetGamepadID(app.gamepad) == event.gdevice.which) {
          SDL_CloseGamepad(app.gamepad);
          app.gamepad = nullptr;
          openFirstGamepad(app);
        }
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        handleGamepadButton(
            app, static_cast<SDL_GamepadButton>(event.gbutton.button), true);
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        handleGamepadButton(
            app, static_cast<SDL_GamepadButton>(event.gbutton.button), false);
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        if (app.screen == Screen::Playing)
          app.screen = Screen::Paused;
        app.left.release();
        app.right.release();
        app.down.release();
        break;
      default:
        break;
      }
    }

    const std::uint64_t currentTicks = SDL_GetTicks();
    const auto elapsedTicks = currentTicks - previousTicks;
    previousTicks = currentTicks;
    const float deltaMs =
        static_cast<float>(std::min<std::uint64_t>(elapsedTicks, 100));
    const float deltaSeconds = deltaMs / 1000.0F;

    updateRepeat(app, app.left, tetris::Action::MoveLeft, deltaMs, 150.0F,
                 35.0F);
    updateRepeat(app, app.right, tetris::Action::MoveRight, deltaMs, 150.0F,
                 35.0F);
    updateRepeat(app, app.down, tetris::Action::SoftDrop, deltaMs, 45.0F,
                 35.0F);
    updateParticles(app, deltaSeconds);

    if (app.screen == Screen::Playing) {
      simulationAccumulator += static_cast<double>(deltaMs);
      while (simulationAccumulator >= FixedStepMs) {
        int stepMs = 16;
        millisecondRemainder += 40;
        if (millisecondRemainder >= 60) {
          ++stepMs;
          millisecondRemainder -= 60;
        }
        app.game.tick(static_cast<std::uint32_t>(stepMs));
        simulationAccumulator -= FixedStepMs;
        app.checkGameEvents();
      }
    } else {
      simulationAccumulator = 0.0;
    }

    render(app);
    if (smokeTestFrames > 0 && --smokeTestFrames == 0)
      app.running = false;
  }

  app.saveHighScore();
  if (app.gamepad != nullptr)
    SDL_CloseGamepad(app.gamepad);
  app.sound.shutdown();
  SDL_DestroyRenderer(app.renderer);
  SDL_DestroyWindow(app.window);
  SDL_Quit();
  return EXIT_SUCCESS;
}
