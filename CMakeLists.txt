cmake_minimum_required(VERSION 3.0.0)
project(game_kapal VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(FetchContent)
enable_testing()

set(RAYLIB_VERSION 4.5.0)

add_executable(game_kapal main.cpp)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS ${RAYLIB_VERSION} EXACT
)

target_link_libraries(${PROJECT_NAME} raylib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
