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
add_library(luaprov-lockdown SHARED ${SOURCES})

set (LIBS
    dbgscriptsupport
    debug ${LUA_LIB}/debug/lua-${LUA_VER}.lib
    optimized ${LUA_LIB}/release/lua-${LUA_VER}.lib)

# luaprov.dll depends on dbgscriptsupport.lib
#
target_link_libraries (luaprov ${LIBS})
target_link_libraries (luaprov-lockdown ${LIBS})

SET_TARGET_PROPERTIES(luaprov-lockdown
  PROPERTIES COMPILE_DEFINITIONS "LOCKDOWN=1")
