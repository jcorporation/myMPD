cmake_minimum_required(VERSION 2.6)

project (mympd_plugins)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE -ldl")

#mympd_coverextract shared library
add_library(mympd_coverextract SHARED mympd_coverextract.cpp)

install(DIRECTORY DESTINATION share/mympd/lib)
install(TARGETS mympd_coverextract DESTINATION share/mympd/lib/)


#testcoverextract executable
add_executable(test_coverextract ../src/plugins/test_coverextract.c)
