# Top level project for pythonprov.
#
project (pythonprov)

add_definitions(-DPy_NO_ENABLE_SHARED)

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
target_link_libraries (pythonprov dbgscriptsupport ws2_32.lib)

target_link_libraries (
	pythonprov debug
	python36_d.lib)

target_link_libraries (
	pythonprov optimized
	python36.lib)

set_target_properties(pythonprov PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/LTCG /INCREMENTAL:NO")
