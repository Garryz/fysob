include_directories(${CMAKE_SOURCE_DIR})

set(TEST_FILE "account.lua")
add_custom_command(OUTPUT ${TEST_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_FILE} ${TEST_FILE}
    )
add_custom_target(Test1 ALL DEPENDS ${TEST_FILE})

add_executable(lunar_test account)
target_link_libraries(lunar_test lua)

add_executable(lunar_test1 account1)
target_link_libraries(lunar_test1 lua)
