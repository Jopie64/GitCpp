cmake_minimum_required (VERSION 2.6)

if (TARGET GitCpp)
	message("Only build GitCpp once.")
	return()
endif()

project (GitCpp)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 
# Here it is setting the Visual Studio warning level to 4
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
SET (BUILD_SHARED_LIBS off)
SET (BUILD_CLAR off)
SET (STATIC_CRT off)

# Command to output information to the console
# Useful for displaying errors, warnings, and debugging
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

# Sub-directories where more CMakeLists.txt exist
# Note: To include libgit2 library symbols in the gitcpp, see here:
# http://stackoverflow.com/questions/11429055/cmake-how-create-a-single-shared-library-from-all-static-libraries-of-subprojec

add_subdirectory(libgit2)
add_subdirectory(JStd)
set(LIBGIT2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libgit2/include)
set(LIBGIT2_LIBRARIES git2)
set(LIBGIT2_FOUND 1)

set(GITCPP_SOURCES_CPP
	"Git.cpp")
set(GITCPP_SOURCES_H
	"Git.h")

source_group("include" FILES ${GITCPP_SOURCES_H})
source_group("src" FILES ${GITCPP_SOURCES_CPP})

add_library(GitCpp STATIC ${GITCPP_SOURCES_CPP}
						  ${GITCPP_SOURCES_H})

# set (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} libgit2\cmake)

target_link_libraries(GitCpp PUBLIC ${LIBGIT2_LIBRARIES} JStd)
target_include_directories(GitCpp PUBLIC . ${LIBGIT2_INCLUDE_DIR})
