cmake_minimum_required(VERSION 3.25)
project(game_kapal VERSION 1.0.0 DESCRIPTION "KAPALLLLLL")

set(CMAKE_CXX_STANDARD 20)

find_package(raylib REQUIRED)

set(projectSOURCES
    src/main.cpp

)

include_directories(/home/olel/raylib/src)

add_executable(${PROJECT_NAME} ${projectSOURCES} ${projectHEADERS})

target_link_libraries(${PROJECT_NAME} raylib)
