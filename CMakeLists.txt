cmake_minimum_required (VERSION 2.6)

project (GitCpp)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
elseif (CMAKE_COMPILER_IS_GNUCXX )
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

SET (BUILD_SHARED_LIBS on)
SET (BUILD_CLAR off)
SET (STATIC_CRT off)

# Command to output information to the console
# Useful for displaying errors, warnings, and debugging
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

# Sub-directories where more CMakeLists.txt exist
add_subdirectory(libgit2)
add_subdirectory(jstd)
set(LIBGIT2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libgit2/include)
set(LIBGIT2_LIBRARIES git2)
set(LIBGIT2_FOUND 1)

set(GITCPP_SOURCES_CPP
	"Git.cpp")
set(GITCPP_SOURCES_H
	"Git.h")

source_group("include" FILES ${GITCPP_SOURCES_H})
source_group("src" FILES ${GITCPP_SOURCES_CPP})

include_directories(. ${LIBGIT2_INCLUDE_DIR})
add_definitions(-DJ_CREATING_LIB)
add_library(GitCpp SHARED ${GITCPP_SOURCES_CPP}
						  ${GITCPP_SOURCES_H})

# set (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} libgit2\cmake)

target_link_libraries(GitCpp ${LIBGIT2_LIBRARIES} JStd)
