cmake_minimum_required(VERSION 3.20)
project(sprawn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

add_subdirectory(src/backend)
add_subdirectory(src/middleware)
add_subdirectory(src/frontend)

option(SPRAWN_BUILD_TESTS "Build tests" ON)
if(SPRAWN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
