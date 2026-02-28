add_library(sprawn_backend
    mapped_file.cpp
    file_source.cpp
    piece_table.cpp
    line_index.cpp
    encoding.cpp
    document.cpp
)

target_include_directories(sprawn_backend PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(sprawn_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
