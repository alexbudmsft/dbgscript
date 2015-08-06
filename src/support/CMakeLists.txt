project (dbgscriptsupport)

include_directories (${CMAKE_HOME_DIRECTORY}/include)

add_library(
	dbgscriptsupport
	symcache.cpp
	util.cpp
	outputcallback.cpp
	dsstackframe.cpp
)