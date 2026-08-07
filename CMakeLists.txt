cmake_minimum_required(VERSION 3.24)

project(tetris_clone VERSION 1.0.0 LANGUAGES CXX)

option(TETRIS_FETCH_SDL "Fetch SDL3 when no system package is available" ON)
option(TETRIS_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
find_package(SDL3 3.2 CONFIG QUIET)

if(NOT SDL3_FOUND)
  if(NOT TETRIS_FETCH_SDL)
    message(FATAL_ERROR "SDL3 was not found. Install SDL3 or configure with TETRIS_FETCH_SDL=ON.")
  endif()

  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.8
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

function(tetris_target_defaults target)
  target_compile_features(${target} PUBLIC cxx_std_20)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()

  if(TETRIS_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

add_library(tetris_core
  src/game.cpp
  src/persistence.cpp
)
target_include_directories(tetris_core PUBLIC include)
tetris_target_defaults(tetris_core)

add_executable(tetris_clone src/main.cpp)
target_link_libraries(tetris_clone PRIVATE tetris_core SDL3::SDL3)
tetris_target_defaults(tetris_clone)

include(CTest)
if(BUILD_TESTING)
  add_executable(tetris_tests tests/game_tests.cpp)
  target_link_libraries(tetris_tests PRIVATE tetris_core)
  tetris_target_defaults(tetris_tests)
  add_test(NAME tetris_core_tests COMMAND tetris_tests)
endif()

install(TARGETS tetris_clone RUNTIME DESTINATION bin)
