include_directories (${PROJECT_SOURCE_DIR}/app)

set( EXE_NAME xevd_app )

file (GLOB DEC_SRC "*.c" )
file (GLOB DEC_INC "*.h" )

add_executable (${EXE_NAME} ${DEC_SRC} ${DEC_INC} )
include_directories( ${EXE_NAME} PUBLIC . .. ../inc ../src)
target_link_libraries (${EXE_NAME} xevd)
set_property(TARGET ${EXE_NAME} PROPERTY FOLDER "app")
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if( MSVC )
    target_compile_definitions( ${EXE_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS ANY )
elseif( UNIX OR MINGW )
    target_compile_definitions( ${EXE_NAME} PUBLIC LINUX ANY )
endif()