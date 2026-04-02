cmake_minimum_required(VERSION 3.20)

project(Pinnacium VERSION 0.1.0 LANGUAGES CXX)

add_library(pinnacium benchmark.cpp)
add_library(Pinnacium::pinnacium ALIAS pinnacium)

target_include_directories(
    pinnacium
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(pinnacium PUBLIC cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(pinnacium PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(pinnacium PRIVATE /W4 /permissive-)
endif()

add_executable(pinnacium_basic examples/basic_benchmark.cpp)
target_link_libraries(pinnacium_basic PRIVATE Pinnacium::pinnacium)
