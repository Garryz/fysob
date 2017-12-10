project(lua)

include_directories(.)

aux_source_directory(. LUA_SOURCES)
list(REMOVE_ITEM LUA_SOURCES "lua.c" "luac.c")

add_library(lua ${LUA_SOURCES})
