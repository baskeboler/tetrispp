# Tetris Clone

A polished desktop falling-block game written in C++20 with SDL3. It uses only
procedurally drawn graphics and generated sound effects, so there are no runtime
asset files to install.

## Features

- 7-bag piece generation, five-piece preview, hold, and ghost piece
- Super Rotation System wall kicks
- Soft/hard drop, lock delay, combos, back-to-back bonuses, T-spins, levels,
  and guideline-style scoring
- Resizable 16:9 presentation with keyboard and gamepad controls
- Procedural effects and audio with a mute toggle
- Persistent per-user high score
- SDL-independent game model with deterministic tests

## Build

You need CMake 3.24 or newer, a C++20 compiler, Git, and the platform libraries
SDL needs. If CMake cannot find an installed SDL3 package, it fetches the pinned
SDL 3.4.8 source release automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tetris_clone
```

To require an already-installed SDL3 package instead of downloading it:

```sh
cmake -S . -B build -DTETRIS_FETCH_SDL=OFF
```

### Tests

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For a non-interactive platform startup check, run the executable with
`--smoke-test`; it initializes SDL, renders three frames, and exits cleanly.

Sanitizers are available on supported Clang/GCC platforms:

```sh
cmake -S . -B build-sanitize \
  -DTETRIS_ENABLE_SANITIZERS=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sanitize -j
ctest --test-dir build-sanitize --output-on-failure
```

## Controls

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Move | Left / Right | D-pad Left / Right |
| Soft drop | Down | D-pad Down |
| Hard drop | Space | D-pad Up |
| Rotate clockwise | X or Up | South face button |
| Rotate counter-clockwise | Z | West face button |
| Hold | C or Left Shift | North face button |
| Pause / resume | P or Escape | Start |
| Mute | M | — |
| Restart after game over | Enter or R | South face button |

The game owns key-repeat timing rather than relying on desktop keyboard repeat,
so movement remains consistent across systems.

## Architecture

- `tetris_core` contains the board, pieces, randomizer, movement, scoring, timing,
  and high-score file parser. It has no SDL dependency.
- `tetris_clone` owns the SDL window, fixed-step loop, input devices, renderer,
  procedural audio, visual effects, and preference-directory selection.
- `tetris_tests` exercises deterministic game rules and persistence through CTest.

The project is verified on Linux and uses portable SDL3/CMake APIs so it can be
built on Windows and macOS. Platform-specific packaging is intentionally outside
the initial release.

## Branding

This repository is a private programming exercise. “Tetris” is a trademark of
The Tetris Company; no official art, music, logos, or other proprietary assets
are included. Rename the project before public distribution.
