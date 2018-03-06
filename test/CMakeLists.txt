include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/third_party/g3log)

find_package(Threads)

add_executable(echo echo)
target_link_libraries(echo ${CMAKE_THREAD_LIBS_INIT})

aux_source_directory(${CMAKE_SOURCE_DIR}/engine/handler ENGINE_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/engine/net ENGINE_SRCS)

add_executable(print_server print_server ${ENGINE_SRCS})
target_link_libraries(print_server ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(joker_test test ${ENGINE_SRCS})
target_link_libraries(joker_test ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(echo_server ./simple/echo/echo_server.cpp ${ENGINE_SRCS})
target_link_libraries(echo_server ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(echo_client ./simple/echo/echo_client.cpp ${ENGINE_SRCS})
target_link_libraries(echo_client ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(delimiter_server ./handler_test/delimiter_server.cpp ${ENGINE_SRCS})
target_link_libraries(delimiter_server ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(delimiter_client ./handler_test/delimiter_client.cpp ${ENGINE_SRCS})
target_link_libraries(delimiter_client ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(length_server ./handler_test/length_server.cpp ${ENGINE_SRCS})
target_link_libraries(length_server ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(length_client ./handler_test/length_client.cpp ${ENGINE_SRCS})
target_link_libraries(length_client ${CMAKE_THREAD_LIBS_INIT} g3log)

add_executable(client client ${ENGINE_SRCS})
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT} g3log)


