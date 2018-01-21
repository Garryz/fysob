project(lua)

include_directories(.)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DLUA_USE_LINUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions(-DLUA_USE_MACOSX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DLUA_USE_WINDOWS)
endif()

aux_source_directory(. LUA_SOURCES)
list(REMOVE_ITEM LUA_SOURCES "lua.c" "luac.c")

add_library(lua ${LUA_SOURCES})
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(lua dl readline)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(lua readline)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
endif()
