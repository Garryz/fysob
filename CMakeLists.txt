cmake_minimum_required (VERSION 2.8.12.2)
project (joker)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_HAS_STD_CHRONO)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DASIO_MSVC)
endif ()

add_subdirectory(${CMAKE_SOURCE_DIR}/engine)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/lua)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/pbc)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/g3log)
add_subdirectory(${CMAKE_SOURCE_DIR}/test)
add_subdirectory(${CMAKE_SOURCE_DIR}/test/any_test)
add_subdirectory(${CMAKE_SOURCE_DIR}/test/lunar_test)
