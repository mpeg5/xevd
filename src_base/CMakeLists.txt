# common library
set( LIB_NAME_BASE xevdb )

file (GLOB XEVD_INC "../inc/*.h")
file (GLOB LIB_API_SRC "xevd.c")
file (GLOB LIB_SRC "xevd_*.c")
file (GLOB LIB_INC "xevd_*.h")
file (GLOB LIB_SSE_SRC "sse/xevd_*.c")
file (GLOB LIB_SSE_INC "sse/xevd_*.h" )
file (GLOB LIB_AVX_SRC "avx/xevd_*.c")
file (GLOB LIB_AVX_INC "avx/xevd_*.h" )

add_library( ${LIB_NAME_BASE} STATIC ${XEVD_INC} ${LIB_API_SRC} ${LIB_SRC} ${LIB_INC} ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC})
add_library( ${LIB_NAME_BASE}_dynamic SHARED ${XEVD_INC} ${LIB_API_SRC} ${LIB_SRC} ${LIB_INC} ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC})

target_include_directories( ${LIB_NAME_BASE} PUBLIC . .. ../inc ./sse  ./avx )
set_target_properties(${LIB_NAME_BASE} PROPERTIES FOLDER lib
                                               ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_include_directories( ${LIB_NAME_BASE}_dynamic PUBLIC . .. ../inc ./sse ./avx)
set_target_properties(${LIB_NAME_BASE}_dynamic PROPERTIES OUTPUT_NAME ${LIB_NAME_BASE})

set( SSE ${BASE_INC_FILES} ${LIB_SSE_SRC})
set( AVX ${LIB_AVX_SRC} )

source_group("base\\header" FILES ${LIB_INC} ${XEVD_INC})
source_group("base\\source" FILES ${LIB_SRC} ${LIB_API_SRC})
source_group("base\\sse\\header" FILES ${LIB_SSE_INC})
source_group("base\\sse\\source" FILES ${LIB_SSE_SRC})
source_group("base\\avx\\header" FILES ${LIB_AVX_INC})
source_group("base\\avx\\source" FILES ${LIB_AVX_SRC})

if( MSVC )
  set_target_properties(${LIB_NAME_BASE}_dynamic PROPERTIES FOLDER lib
                                                       RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  target_compile_definitions( ${LIB_NAME_BASE} PUBLIC ANY _CRT_SECURE_NO_WARNINGS)
  target_include_directories( ${LIB_NAME_BASE} PUBLIC . .. )
elseif( UNIX OR MINGW )
  set_target_properties(${LIB_NAME_BASE}_dynamic PROPERTIES FOLDER lib
                                                       LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  target_compile_definitions( ${LIB_NAME_BASE} PUBLIC ANY LINUX )
  set_property( SOURCE ${SSE} APPEND PROPERTY COMPILE_FLAGS "-msse4.1" )
  set_property( SOURCE ${AVX} APPEND PROPERTY COMPILE_FLAGS " -mavx2" )
  target_link_libraries(${LIB_NAME_BASE} m)
endif()

