find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

add_library(sprawn_frontend
    window.cpp
    renderer.cpp
    font_face.cpp
    glyph_atlas.cpp
    text_layout.cpp
    line_cache.cpp
    viewport.cpp
    input_handler.cpp
    editor.cpp
    application.cpp
)

target_include_directories(sprawn_frontend
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${HARFBUZZ_INCLUDE_DIRS}
)

# SDL2 imported target name varies by cmake version / package provider.
# Try SDL2::SDL2 first, then fall back to the pkg-config result.
if(TARGET SDL2::SDL2)
    set(_SDL2_LIB SDL2::SDL2)
else()
    pkg_check_modules(SDL2 REQUIRED sdl2)
    set(_SDL2_LIB ${SDL2_LIBRARIES})
    target_include_directories(sprawn_frontend PRIVATE ${SDL2_INCLUDE_DIRS})
endif()

target_link_libraries(sprawn_frontend
    PUBLIC
        sprawn_backend
    PRIVATE
        ${_SDL2_LIB}
        Freetype::Freetype
        ${HARFBUZZ_LIBRARIES}
)

target_compile_features(sprawn_frontend PUBLIC cxx_std_20)

add_executable(sprawn ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(sprawn PRIVATE sprawn_frontend)
