# Top level project for rubyprov. This project needs to target VS 2013.
#
project (rubyprov)

cmake_minimum_required (VERSION 3.3)

set (DBGSCRIPTROOT ../..)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRUBY_STATIC")

include (${DBGSCRIPTROOT}/common.cmake)

include_directories (
	${DBGSCRIPTROOT}/include 
	${DBGSCRIPTROOT}/deps/ruby/include/ruby-2.3.0
	${DBGSCRIPTROOT}/deps/ruby/include/ruby-2.3.0/x64-mswin64_120
	)

link_directories(${DBGSCRIPTROOT}/deps/ruby/lib)

set (SOURCES
	version.rc
	dllmain.cpp
	rubyscriptprovider.cpp
	dbgscript.cpp
	thread.cpp
	stackframe.cpp
	util.cpp
	typedobject.cpp
)

# Make a DLL.
#
add_library(rubyprov SHARED ${SOURCES})

# rubyprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (
	rubyprov
	dbgscriptsupport
	dbgeng.lib
	dbghelp.lib
	ws2_32.lib
	iphlpapi.lib
	shlwapi.lib
	)
	
target_link_libraries (
	rubyprov debug
	x64-msvcr120-ruby230_d-static.lib)

target_link_libraries (
	rubyprov optimized
	x64-msvcr120-ruby230-static.lib)
	
add_subdirectory(../support ${CMAKE_CURRENT_BINARY_DIR}/support)
