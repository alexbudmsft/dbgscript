# Warnings as errors
# Disable C4127: Conditional expression is constant.
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX /W4 /wd4127 -DUNICODE -D_UNICODE")