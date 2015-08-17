# Top level project for luaprov.
#
project (luaprov)

set (LUA_VER 5.3.1)
set (LUA_DIR ${CMAKE_HOME_DIRECTORY}/deps/lua-${LUA_VER})
set (LUA_INC ${LUA_DIR}/src)
set (LUA_LIB ${LUA_DIR}/bin)

include_directories (
	${CMAKE_HOME_DIRECTORY}/include 
	${LUA_INC})

set (SOURCES
	version.rc
	dllmain.cpp
	luascriptprovider.cpp
	dbgscript.cpp
	typedobject.cpp
	classprop.cpp
	util.cpp
	thread.cpp
	stackframe.cpp
	)

# Make a DLL.
#
add_library(luaprov SHARED ${SOURCES})

# luaprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (luaprov dbgscriptsupport)

# Flavour-specific libs.
#
target_link_libraries (
	luaprov debug
	${LUA_LIB}/debug/lua-${LUA_VER}.lib)

target_link_libraries (
	luaprov optimized
	${LUA_LIB}/release/lua-${LUA_VER}.lib)

