#include "tetris/persistence.hpp"

#include <charconv>
#include <fstream>
#include <string>
#include <system_error>

namespace tetris {

std::uint64_t HighScoreStore::load() const {
  std::ifstream input(path_);
  std::string text;
  if (!input || !std::getline(input, text)) {
    return 0;
  }

  std::uint64_t score{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), score);
  if (error != std::errc{} || end != text.data() + text.size()) {
    return 0;
  }
  return score;
}

bool HighScoreStore::save(std::uint64_t score) const {
  std::error_code error;
  if (!path_.parent_path().empty()) {
    std::filesystem::create_directories(path_.parent_path(), error);
    if (error) {
      return false;
    }
  }

  auto temporary = path_;
  temporary += ".tmp";
  {
    std::ofstream output(temporary, std::ios::trunc);
    if (!output || !(output << score << '\n')) {
      return false;
    }
  }

  std::filesystem::rename(temporary, path_, error);
  if (!error) {
    return true;
  }

  std::filesystem::remove(path_, error);
  error.clear();
  std::filesystem::rename(temporary, path_, error);
  return !error;
}

}  // namespace tetris
