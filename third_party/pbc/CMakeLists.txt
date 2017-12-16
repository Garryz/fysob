project(pbc)

include_directories(.)

aux_source_directory(. PBC_SOURCES)

add_library(pbc ${PBC_SOURCES})
