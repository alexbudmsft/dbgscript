# Top level project for dbgscript.
#
project (dbgscript)

cmake_minimum_required (VERSION 3.3)

include (common.cmake)

include_directories (${CMAKE_HOME_DIRECTORY}/include)

set (SOURCES src/dllmain.cpp src/cmdline.cpp src/version.rc)
set (LIBS dbgscriptsupport dbgeng)

# Make a DLL.
#
add_library(dbgscript SHARED ${SOURCES})
add_library(dbgscript-lockdown SHARED ${SOURCES})

target_link_libraries (dbgscript ${LIBS})
target_link_libraries (dbgscript-lockdown ${LIBS})

set_target_properties(dbgscript-lockdown PROPERTIES
  COMPILE_DEFINITIONS "LOCKDOWN=1")

# Subprojects
#
add_subdirectory(src/support)
add_subdirectory(src/pythonprov)
add_subdirectory(src/luaprov)
