cmake_minimum_required(VERSION 3.10)

# test
set(test_out test.out)

add_executable(${test_out} test.c)
target_link_libraries(${test_out} zip)

add_test(NAME ${test_out} COMMAND ${test_out})

set(test_out ${test_out} PARENT_SCOPE)
