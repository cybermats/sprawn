add_library(sprawn_backend STATIC
    backend.cpp
    file_source.cpp
    lorem_ipsum_source.cpp
    string_array_document.cpp
)

target_include_directories(sprawn_backend
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src/backend
)
