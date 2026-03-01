include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

function(sprawn_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE sprawn_backend doctest_with_main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

sprawn_add_test(test_piece_table)
sprawn_add_test(test_line_index)
sprawn_add_test(test_document)

add_executable(test_controller test_controller.cpp)
target_link_libraries(test_controller PRIVATE sprawn_middleware doctest_with_main)
add_test(NAME test_controller COMMAND test_controller)
