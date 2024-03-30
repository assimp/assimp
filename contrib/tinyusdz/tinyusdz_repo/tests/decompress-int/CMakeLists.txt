set(TEST_TARGET_NAME test-decompress-int)

set(TEST_SOURCES
  test-decompress-int-main.cc
  ${PROJECT_SOURCE_DIR}/src/lz4/lz4.c 
  ${PROJECT_SOURCE_DIR}/src/lz4-compression.cc
  ${PROJECT_SOURCE_DIR}/src/integerCoding.cpp
   )

add_executable(${TEST_TARGET_NAME}
	${TEST_SOURCES}
	)

add_sanitizers(${TEST_TARGET_NAME})

# TODO: Add test-decompress-int to unit test suite?
#if (WIN32)
#  add_test(NAME ${TEST_TARGET_NAME} COMMAND "${TEST_TARGET_NAME}.exe"
#           WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_TARGET_NAME}>  )
#else (WIN32)
#  add_test(NAME ${TEST_TARGET_NAME} COMMAND "${TEST_TARGET_NAME}"
#           WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_TARGET_NAME}>  )
#endif (WIN32)

target_include_directories(${TEST_TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

set_target_properties(${TEST_TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

