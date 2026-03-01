add_library(sprawn_middleware
    controller.cpp
)

target_include_directories(sprawn_middleware
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sprawn_middleware
    PUBLIC
        sprawn_backend
)
