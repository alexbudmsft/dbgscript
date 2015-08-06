# Top level project for rubyprov. This project needs to target VS 2013.
#
project (rubyprov)

cmake_minimum_required (VERSION 3.3)

set (DBGSCRIPTROOT ../..)

include (${DBGSCRIPTROOT}/common.cmake)

include_directories (
	${DBGSCRIPTROOT}/include 
	${DBGSCRIPTROOT}/deps/ruby/include)

link_directories(${DBGSCRIPTROOT}/deps/ruby/lib)

set (SOURCES
	dllmain.cpp
	rubyscriptprovider.cpp
	dbgscript.cpp
	thread.cpp
	stackframe.cpp
)

# Make a DLL.
#
add_library(rubyprov SHARED ${SOURCES})

# rubyprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (rubyprov dbgscriptsupport dbgeng.lib x64-msvcr120-ruby220.lib)

add_subdirectory(../support ${CMAKE_CURRENT_BINARY_DIR}/support)