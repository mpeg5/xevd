cmake_minimum_required (VERSION 3.5)

# Check input arguments.
if(NOT SET_PROF)
  set(SET_PROF "BASE")
endif()

if(("${SET_PROF}" STREQUAL "MAIN") OR ("${SET_PROF}" STREQUAL "BASE"))
  message("SET_PROF=${SET_PROF}")
  if("${SET_PROF}" STREQUAL "MAIN")
    message("Main profile is not yet supported ")
    return()
  endif()
else()
  message("SET_PORF should be BASE or MAIN [Input error: ${SET_PROF}]")
  return()
endif()

option(XEVD_APP_STATIC_BUILD "app will be statically linked against static library" ON)
if(XEVD_APP_STATIC_BUILD)
    add_definitions(-DXEVD_STATIC_DEFINE)
endif(XEVD_APP_STATIC_BUILD)

# Maps to a solution file (XEVD.sln).
project (XEVD)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 
if( MSVC )
elseif( UNIX OR MINGW )
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(OPT_LV "O0")
    else()
        set(OPT_LV "O3")
    endif()
    
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fomit-frame-pointer -Wall -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -Wno-attributes -Werror -Wno-strict-overflow -Wno-unknown-pragmas -Wno-stringop-overflow -std=c99")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign -pthread -Wno-pointer-to-int-cast -Wno-maybe-uninitialized")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
endif()

# Command to output information to the console
message ("c Flags: " ${CMAKE_C_FLAGS})
message ("linker Flags: " ${CMAKE_EXE_LINKER_FLAGS})

# Sub-directories where more CMakeLists.txt exist
if(("${SET_PROF}" STREQUAL "BASE"))
   add_subdirectory(src_base)
endif()
if(("${SET_PROF}" STREQUAL "MAIN"))
   add_subdirectory(src_main)
endif()
add_subdirectory(app)
