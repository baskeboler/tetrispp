# Repository Guidelines

## Project Structure & Module Organization

- `include/tetris/` contains public C++ interfaces. Keep gameplay types independent of SDL.
- `src/game.cpp` implements deterministic board, piece, scoring, and timing rules.
- `src/persistence.cpp` handles high-score storage; `src/main.cpp` owns SDL windowing, rendering, input, audio, and the application loop.
- `tests/game_tests.cpp` contains the lightweight assertion-based test suite registered with CTest.
- `build/` and `build-*` are generated directories and must not be committed. The game has no runtime asset directory because visuals and audio are procedural.

## Build, Test, and Development Commands

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/tetris_clone
```

CMake uses an installed SDL3 package when available and otherwise fetches pinned SDL 3.4.8 sources. Use `-DTETRIS_FETCH_SDL=OFF` to require a system installation. For memory and undefined-behavior checks, configure a separate debug tree with `-DTETRIS_ENABLE_SANITIZERS=ON`. Run `./build/tetris_clone --smoke-test` for a three-frame, non-interactive SDL startup check.

## Coding Style & Naming Conventions

Use C++20, two-space indentation, braces on the same line, and no tab characters. Follow existing naming: `PascalCase` for types and enum values, `camelCase` for functions, and trailing underscores for private fields. Keep constants descriptive (`BoardWidth`) and implementation helpers inside anonymous namespaces. Prefer RAII, standard-library containers, fixed-width integers for serialized/state values, and `[[nodiscard]]` for meaningful return values. Builds must remain warning-free under the flags defined in `CMakeLists.txt`.

## Testing Guidelines

Add deterministic tests for every gameplay rule or persistence change. Name test functions after behavior, such as `holdWorksOncePerPiece`, and use fixed RNG seeds. Keep core tests SDL-free. Before submitting, run CTest, the sanitizer build when practical, and `git diff --check`.

## Commit & Pull Request Guidelines

The repository has no commit history, so no convention is established. Use short imperative subjects, for example `Add lock-delay reset cap`, and keep commits focused. Pull requests should explain gameplay or architecture changes, list validation commands, link relevant issues, and include screenshots or a short capture for visible UI changes. Note platform-specific limitations, especially unavailable SDL audio or display backends.
