set( LIB_NAME xevd )

# For shared libraries VERSION and SOVERSION can be used to specify the build version and API version respectively
# see https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html
#
set( LIB_SOVERSION ${VERSION_MAJOR}))

file (GLOB XEVD_INC "../inc/*.h")
file (GLOB LIB_BASE_SRC "../src_base/xevd_*.c")
file (GLOB LIB_BASE_INC "../src_base/xevd_*.h" )
file (GLOB LIB_SSE_SRC "../src_base/sse/xevd_*.c")
file (GLOB LIB_SSE_INC "../src_base/sse/xevd_*.h" )
file (GLOB LIB_AVX_SRC "../src_base/avx/xevd_*.c")
file (GLOB LIB_AVX_INC "../src_base/avx/xevd_*.h" )
file (GLOB LIB_API_MAIN_SRC "./xevdm.c")
file (GLOB LIB_MAIN_SRC "./xevdm_*.c")
file (GLOB LIB_MAIN_INC "./xevdm_*.h" )
file (GLOB LIB_MAIN_SSE_SRC "./sse/xevdm_*.c")
file (GLOB LIB_MAIN_SSE_INC "./sse/xevdm_*.h" )

include(GenerateExportHeader)
include_directories("${CMAKE_BINARY_DIR}")

add_library( ${LIB_NAME} STATIC ${LIB_API_MAIN_SRC} ${XEVD_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC} 
                                ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_MAIN_SSE_SRC} ${LIB_MAIN_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC} )
add_library( ${LIB_NAME}_dynamic SHARED ${LIB_API_MAIN_SRC} ${XEVD_INC} ${LIB_BASE_SRC} ${LIB_BASE_INC} ${LIB_MAIN_SRC} ${LIB_MAIN_INC} 
                                        ${LIB_SSE_SRC} ${LIB_SSE_INC} ${LIB_MAIN_SSE_SRC} ${LIB_MAIN_SSE_INC} ${LIB_AVX_SRC} ${LIB_AVX_INC} )

set_target_properties(${LIB_NAME}_dynamic PROPERTIES VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} SOVERSION ${LIB_SOVERSION})

# @todo Consider using WINDOWS_EXPORT_ALL_SYMBOLS instead of generate_export_header
# @see https://cmake.org/cmake/help/latest/prop_tgt/WINDOWS_EXPORT_ALL_SYMBOLS.html#prop_tgt:WINDOWS_EXPORT_ALL_SYMBOLS
#if(MSVC)
    # @see https://blog.kitware.com/create-dlls-on-windows-without-declspec-using-new-cmake-export-all-feature/
    # @see https://cmake.org/cmake/help/v3.3/module/GenerateExportHeader.html
    #
    #set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    #set(BUILD_SHARED_LIBS TRUE)
#endif()

# Generate export macros for libraries
# @see https://gernotklingler.com/blog/creating-using-shared-libraries-different-compilers-different-operating-systems/
generate_export_header(${LIB_NAME}_dynamic           
                       BASE_NAME XEVD
                       EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/xevd_exports.h)

# This will cause the export macros to expand to nothing when building the static library.
set_target_properties(${LIB_NAME} PROPERTIES  COMPILE_FLAGS -DLIBSHARED_AND_STATIC_STATIC_DEFINE)

source_group("base\\header" FILES ${LIB_BASE_INC} ${XEVD_INC})
source_group("base\\source" FILES ${LIB_BASE_SRC} ${LIB_API_SRC})
source_group("base\\sse\\header" FILES ${LIB_SSE_INC})
source_group("base\\sse\\source" FILES ${LIB_SSE_SRC})
source_group("base\\avx\\header" FILES ${LIB_AVX_INC})
source_group("base\\avx\\source" FILES ${LIB_AVX_SRC})
source_group("main\\header" FILES ${LIB_MAIN_INC})
source_group("main\\source" FILES ${LIB_MAIN_SRC} ${LIB_API_MAIN_SRC})
source_group("main\\sse\\header" FILES ${LIB_MAIN_SSE_INC})
source_group("main\\sse\\source" FILES ${LIB_MAIN_SSE_SRC})

include_directories( ${LIB_NAME} PUBLIC . .. ../inc ./sse ../src_base ../src_base/sse ../src_base/avx)

set_target_properties(${LIB_NAME} PROPERTIES FOLDER lib
                                               ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)

set_target_properties(${LIB_NAME} PROPERTIES FOLDER lib
                                               ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)
                                               
set( SSE ${BASE_INC_FILES} ${LIB_SSE_SRC} ${LIB_MAIN_SSE_SRC})
set( AVX ${LIB_AVX_SRC} )
set_target_properties(${LIB_NAME}_dynamic PROPERTIES OUTPUT_NAME ${LIB_NAME})

if( MSVC )                      
  target_compile_definitions( ${LIB_NAME} PUBLIC ANY _CRT_SECURE_NO_WARNINGS )
  set_target_properties(${LIB_NAME}_dynamic PROPERTIES FOLDER lib
                                               RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
elseif( UNIX OR MINGW )
  set_property( SOURCE ${SSE} APPEND PROPERTY COMPILE_FLAGS "-msse4.1" )
  set_property( SOURCE ${AVX} APPEND PROPERTY COMPILE_FLAGS " -mavx2" )
  
  set_target_properties(${LIB_NAME}_dynamic PROPERTIES FOLDER lib
                                                       LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  target_compile_definitions( ${LIB_NAME} PUBLIC ANY LINUX )  
  target_link_libraries(${LIB_NAME} m)
endif()

# Install rules
#
# Set DCMAKE_INSTALL_PREFIX to change default install prefix
# e.g cmake .. -DCMAKE_INSTALL_PREFIX='C:\Users\Samsung\git\xevd_internal\build-windows\install'

# List the headers we want to declare as public for installation.
set(XEVD_PUBLIC_HEADERS "${XEVD_INC}")
set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${XEVD_PUBLIC_HEADERS}")

set(XEVD_PRIVATE_HEADERS "${LIB_BASE_INC}" "${LIB_SSE_INC}" "${LIB_AVX_INC}")

# Install static library and public headers
#
# Static library (libxevdb.a or xevdb.lib) will be installed to <prefix>/lib/xevdb
# Public headers will be installed to <prefix>/include/xevdb
#
include(GNUInstallDirs)
install(TARGETS ${LIB_NAME}
        ARCHIVE COMPONENT Development DESTINATION ${CMAKE_INSTALL_LIBDIR}/${LIB_NAME}
        PUBLIC_HEADER COMPONENT Development DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
)

# Install shared library
#
# On non-DLL platforms shared library (libxevdb.so) will be installed to <prefix>/lib/xevdb.
# On DLL platforms the shred DLL (xevdb.dll) will be installed to <prefix>/bin and its import library will be installed to <prefix>/lib/xevdb/import
#
install(TARGETS ${LIB_NAME}_dynamic
        RUNTIME COMPONENT Libraries DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY 
            COMPONENT Libraries DESTINATION ${CMAKE_INSTALL_LIBDIR} 
            NAMELINK_COMPONENT Development DESTINATION ${CMAKE_INSTALL_LIBDIR} 
        ARCHIVE COMPONENT Development DESTINATION ${CMAKE_INSTALL_LIBDIR}/${LIB_NAME}/import
)

install( FILES
    ${PROJECT_BINARY_DIR}/xevd_exports.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
)

if( NOT MSVC )
    configure_file(
        "${CMAKE_SOURCE_DIR}/pkgconfig/${LIB_NAME}.pc.in"
        "${CMAKE_BINARY_DIR}/${LIB_NAME}.pc"
        IMMEDIATE @ONLY)
    
    install( FILES
        "${CMAKE_BINARY_DIR}/${LIB_NAME}.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()
