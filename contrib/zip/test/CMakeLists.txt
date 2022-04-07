cmake_minimum_required(VERSION 3.4)

# tests
set(test_write_out test_write.out)
add_executable(${test_write_out} test_write.c)
target_link_libraries(${test_write_out} zip)
add_test(NAME ${test_write_out} COMMAND ${test_write_out})
set(test_write_out ${test_write_out} PARENT_SCOPE)

set(test_append_out test_append.out)
add_executable(${test_append_out} test_append.c)
target_link_libraries(${test_append_out} zip)
add_test(NAME ${test_append_out} COMMAND ${test_append_out})
set(test_append_out ${test_append_out} PARENT_SCOPE)

set(test_read_out test_read.out)
add_executable(${test_read_out} test_read.c)
target_link_libraries(${test_read_out} zip)
add_test(NAME ${test_read_out} COMMAND ${test_read_out})
set(test_read_out ${test_read_out} PARENT_SCOPE)

set(test_extract_out test_extract.out)
add_executable(${test_extract_out} test_extract.c)
target_link_libraries(${test_extract_out} zip)
add_test(NAME ${test_extract_out} COMMAND ${test_extract_out})
set(test_extract_out ${test_extract_out} PARENT_SCOPE)

set(test_entry_out test_entry.out)
add_executable(${test_entry_out} test_entry.c)
target_link_libraries(${test_entry_out} zip)
add_test(NAME ${test_entry_out} COMMAND ${test_entry_out})
set(test_entry_out ${test_entry_out} PARENT_SCOPE)

set(test_permissions_out test_permissions.out)
add_executable(${test_permissions_out} test_permissions.c)
target_link_libraries(${test_permissions_out} zip)
add_test(NAME ${test_permissions_out} COMMAND ${test_permissions_out})
set(test_permissions_out ${test_permissions_out} PARENT_SCOPE)
