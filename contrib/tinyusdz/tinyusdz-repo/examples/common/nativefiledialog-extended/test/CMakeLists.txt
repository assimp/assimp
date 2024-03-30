
set(TEST_LIST
  test_opendialog.c
  test_opendialog_cpp.cpp
  test_opendialogmultiple.c
  test_opendialogmultiple_cpp.cpp
  test_opendialogmultiple_enum.c
  test_pickfolder.c
  test_pickfolder_cpp.cpp
  test_savedialog.c)
  
foreach (TEST ${TEST_LIST})
  string(REPLACE "." "_" CLEAN_TEST_NAME ${TEST})
  add_executable(${CLEAN_TEST_NAME}
    ${TEST})
  target_link_libraries(${CLEAN_TEST_NAME}
    PUBLIC nfd)
endforeach()