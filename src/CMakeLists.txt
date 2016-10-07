set(_POSSIBLE_LUA_INCLUDE lua51 lua5.1 lua-5.1 luajit-2.0 luajit-2.1)

find_path(LUA_INCLUDE_DIR
    NAMES lua.h
    HINTS $ENV{LUA_DIR}
    PATHS ${CMAKE_CURRENT_SOURCE_DIR} /usr/include /usr/local/include
    PATH_SUFFIXES ${_POSSIBLE_LUA_INCLUDE}
    NO_DEFAULT_PATH
    )

if(NOT LUA_INCLUDE_DIR)
    message(FATAL_ERROR "Lua5.1 include directory not found")
else()
    message(STATUS "LUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}")
endif()

if(UNIX)
    set(EXTRA_C_FLAGS "-g -O2 -fomit-frame-pointer -Wall")
    if(APPLE)
        set(LINK_FLAGS "-dynamiclib -single_module -undefined dynamic_lookup")
    else(APPLE)
        set(LINK_FLAGS "-shared")
    endif(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ${EXTRA_C_FLAGS}")
else(UNIX)
    if(WIN32)
        add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB)
    else(WIN32)
        message(FATAL_ERROR "unsupported platform")
    endif(WIN32)
endif(UNIX)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUA_INCLUDE_DIR}
    )


# lua pty library
SET(LPTY_SRCS
    lpty.c
    timeout.c
    )

add_library(lpty SHARED ${LPTY_SRCS})
set_target_properties(lpty PROPERTIES PREFIX "")
target_link_libraries(lpty util)
if(LINK_FLAGS)
    set_target_properties(lpty PROPERTIES
        LINK_FLAGS ${LINK_FLAGS}
        )
endif()


# lua io library
SET(LIO_SRCS
    lio.c
    io_common.c
    timeout.c
    )

if(WIN32)
    list(APPEND LIO_SRCS io_win.c)
else(WIN32)
    list(APPEND LIO_SRCS io_unix.c)
endif(WIN32)


add_library(lio SHARED ${LIO_SRCS})
set_target_properties(lio PROPERTIES PREFIX "")
if(LINK_FLAGS)
    set_target_properties(lio PROPERTIES
        LINK_FLAGS ${LINK_FLAGS}
        )
endif()


# lua timeout library
SET(LTIMEOUT_SRCS
    ltimeout.c
    timeout.c
    )

add_library(ltimeout SHARED ${LTIMEOUT_SRCS})
set_target_properties(ltimeout PROPERTIES PREFIX "")
if(LINK_FLAGS)
    set_target_properties(ltimeout PROPERTIES
        LINK_FLAGS ${LINK_FLAGS}
        )
endif()
