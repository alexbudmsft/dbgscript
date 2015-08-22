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

set (RB_LIBS
	bigdecimal
	bubblebabble
	complex
	console
	continuation
	coverage
	cparse
	date_core
	digest
	etc
	fcntl
	fiber
	generator
	md5
	nkf
	nonblock
	objspace
	parser
	pathname
	psych
	rational
	ripper
	rmd160
	sdbm
	sha1
	sha2
	sizeof
	socket
	stringio
	strscan
	thread
	win32ole
	libenc
	libtrans
)

# Generate the list of files to link, per flavor.
#
set (LINK_LIST "")
foreach(x ${RB_LIBS})
	list (APPEND LINK_LIST debug debug/${x} optimized release/${x})
endforeach()

# Make a DLL.
#
add_library(rubyprov SHARED ${SOURCES})

# rubyprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (rubyprov
	dbgscriptsupport
	dbgeng.lib
	dbghelp.lib
	ws2_32.lib
	iphlpapi.lib
	shlwapi.lib
	${LINK_LIST}
	debug x64-msvcr120-ruby230_d-static.lib
	optimized x64-msvcr120-ruby230-static.lib
	)

# TODO Factor out for both retail and dbg
#
SET_TARGET_PROPERTIES(rubyprov
	PROPERTIES LINK_FLAGS_DEBUG "debug/extinit.obj debug/encinit.obj")
	
SET_TARGET_PROPERTIES(rubyprov
	PROPERTIES LINK_FLAGS_RELWITHDEBINFO "release/extinit.obj release/encinit.obj")
	
add_subdirectory(../support ${CMAKE_CURRENT_BINARY_DIR}/support)
