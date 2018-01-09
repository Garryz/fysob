include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)

find_package(Threads)
add_executable(echo echo)
target_link_libraries(echo ${CMAKE_THREAD_LIBS_INIT})
add_executable(client client)
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})
add_executable(print_server print_server 
    ${CMAKE_SOURCE_DIR}/engine/net/asio_buffer
    ${CMAKE_SOURCE_DIR}/engine/handle/context
    ${CMAKE_SOURCE_DIR}/engine/handle/pipeline)
target_link_libraries(print_server ${CMAKE_THREAD_LIBS_INIT})
