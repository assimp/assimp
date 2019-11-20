cmake_minimum_required(VERSION 2.8)

# test
set(test_out test.out)
set(test_miniz_out test_miniz.out)

add_executable(${test_out} test.c)
target_link_libraries(${test_out} zip)
add_executable(${test_miniz_out} test_miniz.c)
target_link_libraries(${test_miniz_out} zip)

add_test(NAME ${test_out} COMMAND ${test_out})
add_test(NAME ${test_miniz_out} COMMAND ${test_miniz_out})

set(test_out ${test_out} PARENT_SCOPE)
set(test_miniz_out ${test_miniz_out} PARENT_SCOPE)
