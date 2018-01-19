project(g3log)

message( STATUS "build library g3log")
SET(LOG_SRC ${g3log_SOURCE_DIR})
include_directories(${LOG_SRC})
SET(ACTIVE_CPP0xx_DIR "Release")

IF (${CMAKE_CXX_COMPILER_ID} MATCHES ".*Clang")
   message( STATUS "" )
   message( STATUS "cmake for Clang " )
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Wunused -D_GLIBCXX_USE_NANOSLEEP")
   IF (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
       SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ -pthread")
   ELSE()
       SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   ENDIF()
   IF (${CMAKE_SYSTEM} MATCHES "FreeBSD-([0-9]*)\\.(.*)")
       IF (${CMAKE_MATCH_1} GREATER 9)
           set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
           set(PLATFORM_LINK_LIBRIES execinfo)
       ENDIF()
   ELSEIF (APPLE)
       set(PLATFORM_LINK_LIBRIES c++abi)
   ELSEIF (NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Linux"))
       set(PLATFORM_LINK_LIBRIES rt c++abi)
   ELSE()
       set(PLATFORM_LINK_LIBRIES rt)
   ENDIF()
ELSEIF(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   message( STATUS "cmake for GCC " )
   IF (APPLE)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused -std=c++11 -pthread -D_GLIBCXX_USE_NANOSLEEP")
   ELSEIF (MINGW)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused -std=c++11 -pthread -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD")
       set(PLATFORM_LINK_LIBRIES dbghelp)

       # deal with ERROR level conflicts with windows.h
       ADD_DEFINITIONS (-DNOGDI)
   ELSE()
       set(PLATFORM_LINK_LIBRIES rt)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -rdynamic -Wunused -std=c++11 -pthread -lrt -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD")
   ENDIF()
ELSEIF(MSVC)
   set(PLATFORM_LINK_LIBRIES dbghelp)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8") # source code already in utf-8, force it for compilers in non-utf8_windows_locale
   # ERROR level conflicts with windows.h
   ADD_DEFINITIONS (-DNOGDI)
   # support AMD proc on vc2015
   if(${CMAKE_CL_64} STREQUAL "0")
       set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:IA32")
   endif()
endif()

# GENERIC STEPS
file(GLOB SRC_FILES ${LOG_SRC}/g3log/*.h ${LOG_SRC}/g3log/*.hpp ${LOG_SRC}/*.cpp ${LOG_SRC}/*.ipp)
file(GLOB HEADER_FILES ${LOG_SRC}/g3log/*.hpp ${LOG_SRC}/*.hpp)

IF (MSVC OR MINGW)
  list(REMOVE_ITEM SRC_FILES  ${LOG_SRC}/crashhandler_unix.cpp)
ELSE()
  list(REMOVE_ITEM SRC_FILES  ${LOG_SRC}/crashhandler_windows.cpp ${LOG_SRC}/g3log/stacktrace_windows.hpp ${LOG_SRC}/stacktrace_windows.cpp)
ENDIF (MSVC OR MINGW)

set(SRC_FILES ${SRC_FILES})

# Create the g3log library
INCLUDE_DIRECTORIES(${LOG_SRC})
SET(G3LOG_LIBRARY g3log)

ADD_LIBRARY(${G3LOG_LIBRARY} STATIC ${SRC_FILES})

MESSAGE( STATUS "Creating ${G3LOG_LIBRARY}" )
SET_TARGET_PROPERTIES(g3log PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(${G3LOG_LIBRARY} PROPERTIES
  LINKER_LANGUAGE CXX
  OUTPUT_NAME g3log
  CLEAN_DIRECT_OUTPUT 1)

IF(APPLE)
  set_target_properties(${G3LOG_LIBRARY} PROPERTIES MACOSX_RPATH TRUE)
ENDIF()

TARGET_LINK_LIBRARIES(${G3LOG_LIBRARY} ${PLATFORM_LINK_LIBRIES})
