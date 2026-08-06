#pragma once

#include <cstdint>
#include <filesystem>
#include <utility>

namespace tetris {

class HighScoreStore {
public:
  explicit HighScoreStore(std::filesystem::path path)
      : path_(std::move(path)) {}

  [[nodiscard]] std::uint64_t load() const;
  [[nodiscard]] bool save(std::uint64_t score) const;
  [[nodiscard]] const std::filesystem::path &path() const { return path_; }

private:
  std::filesystem::path path_;
};

} // namespace tetris
