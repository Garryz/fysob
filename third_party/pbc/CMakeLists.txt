project(pbc)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-TP)
endif()

include_directories(.)

aux_source_directory(. PBC_SOURCES)

add_library(pbc ${PBC_SOURCES})
