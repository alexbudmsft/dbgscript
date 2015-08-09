# Top level project for pythonprov.
#
project (pythonprov)

include_directories (
	${CMAKE_HOME_DIRECTORY}/include 
	${CMAKE_HOME_DIRECTORY}/deps/python/include)

link_directories(${CMAKE_HOME_DIRECTORY}/deps/python/lib)

set (SOURCES
	version.rc
	dbgscriptio.cpp
	dllmain.cpp
	dbgscript.cpp
	pythonscriptprovider.cpp
	stackframe.cpp
	thread.cpp
	typedobject.cpp
	util.cpp)

# Make a DLL.
#
add_library(pythonprov SHARED ${SOURCES})

# pythonprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (pythonprov dbgscriptsupport)
