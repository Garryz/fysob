include_directories(${CMAKE_SOURCE_DIR})

add_executable(lunar_test account)
target_link_libraries(lunar_test lua)

add_executable(lunar_test1 account1)
target_link_libraries(lunar_test1 lua)
