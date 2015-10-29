# Top level project for dbgscript.
#
project (dbgscript)

cmake_minimum_required (VERSION 3.3)

include (common.cmake)

include_directories (${CMAKE_HOME_DIRECTORY}/include)

set (SOURCES src/dllmain.cpp src/cmdline.cpp src/version.rc)

# Make a DLL.
#
add_library(dbgscript SHARED ${SOURCES})

target_link_libraries (dbgscript dbgscriptsupport dbgeng.lib)

# Subprojects
#
add_subdirectory(src/support)
add_subdirectory(src/pythonprov)
add_subdirectory(src/luaprov)