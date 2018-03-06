include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/third_party/g3log)
include_directories(${CMAKE_SOURCE_DIR}/third_party/lua)

find_package(Threads)

set(bin_root "${CMAKE_SOURCE_DIR}/bin")
file(COPY ../config DESTINATION ${bin_root})
file(COPY ../script DESTINATION ${bin_root})

aux_source_directory(${CMAKE_SOURCE_DIR}/engine/handler ENGINE_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/engine/net ENGINE_SRCS)

add_executable(app ./app/main.cpp ${ENGINE_SRCS})
target_link_libraries(app ${CMAKE_THREAD_LIBS_INIT} g3log lua)
set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${bin_root})

