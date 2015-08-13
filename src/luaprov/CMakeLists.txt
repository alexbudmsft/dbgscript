# Top level project for luaprov.
#
project (luaprov)

set (LUA_DIR ${CMAKE_HOME_DIRECTORY}/deps/lua-5.3.1/src)

include_directories (
	${CMAKE_HOME_DIRECTORY}/include 
	${LUA_DIR})

link_directories(${LUA_DIR})

set (SOURCES
	version.rc
	dllmain.cpp
	luascriptprovider.cpp
	)

# Make a DLL.
#
add_library(luaprov SHARED ${SOURCES})

# luaprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (luaprov dbgscriptsupport lua5.3.1)
