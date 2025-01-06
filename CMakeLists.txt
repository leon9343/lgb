cmake_minimum_required(VERSION 3.30)
project(lgb LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY C_STANDARD 23)

set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose build type: Debug or Release" FORCE)
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(lutil CONFIG REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} src/)

file(GLOB_RECURSE SRCS src/*.c)

add_executable(lgb ${SRCS})

set_target_properties(lgb PROPERTIES 
    DEBUG_POSTFIX "_debug"
    OUTPUT_NAME "lgb"
)

target_link_libraries(lgb 
    PRIVATE 
        lutil::lutil 
        ${SDL2_LIBRARIES} 
        -lSDL2_ttf
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build . --target lgb
    COMMENT "Building debug version"
)

add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build . --target lgb
    COMMENT "Building release version"
)
