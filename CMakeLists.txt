cmake_minimum_required(VERSION 3.30)
project(lgb LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY C_STANDARD 23)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(lutil CONFIG REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} src/)

file(GLOB_RECURSE SRCS src/*.c)

add_executable(lgb ${SRCS})

target_link_libraries(lgb lutil::lutil ${SDL2_LIBRARIES} -lSDL2_ttf)
