add_library(sprawn_frontend STATIC
    frontend.cpp
    window.cpp
    font.cpp
    glyph_cache.cpp
    text_layout.cpp
    renderer.cpp
)

target_include_directories(sprawn_frontend
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src/frontend
)

target_link_libraries(sprawn_frontend
    PUBLIC
        PkgConfig::SDL2
    PRIVATE
        PkgConfig::FREETYPE
        PkgConfig::HARFBUZZ
)
