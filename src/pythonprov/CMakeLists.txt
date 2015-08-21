# Top level project for pythonprov.
#
project (pythonprov)

add_definitions(-DPy_NO_ENABLE_SHARED)

include_directories (
	${CMAKE_HOME_DIRECTORY}/include 
	${CMAKE_HOME_DIRECTORY}/deps/python/include)

link_directories(${CMAKE_HOME_DIRECTORY}/deps/python/lib)

set (PY_LIBS
	python36
	_bz2
	_ctypes
	_decimal
	_elementtree
	_hashlib
	_lzma
	_msi
	_overlapped
	_multiprocessing
	_socket
	_sqlite3
	_ssl
	_tkinter
	select
	sqlite3
	ssleay
	libeay
	unicodedata
)

# Generate the list of files to link, per flavor.
#
set (LINK_LIST "")
foreach(x ${PY_LIBS})
	list (APPEND LINK_LIST debug ${x}_d optimized ${x})
endforeach()

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
target_link_libraries (pythonprov
	dbgscriptsupport
	ws2_32
	msi
	rpcrt4
	cabinet
	crypt32
	liblzma
	${LINK_LIST}
	)

set_target_properties(pythonprov PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/LTCG /INCREMENTAL:NO")
