cmake_minimum_required(VERSION 2.8)

# Set the plugin name to build
project(iec104)

# Supported options:
# -DFLEDGE_INCLUDE
# -DFLEDGE_LIB
# -DFLEDGE_SRC
# -DFLEDGE_INSTALL
#
# If no -D options are given and FLEDGE_ROOT environment variable is set
# then Fledge libraries and header files are pulled from FLEDGE_ROOT path.

set(CMAKE_CXX_FLAGS "-std=c++11 -O3")
set("-I${HOME}lib60870/lib60870-C/src/hal/inc/")
set("-I${HOME}lib60870/lib60870-C/src/inc/api/")

# Generation version header file
set_source_files_properties(version.h PROPERTIES GENERATED TRUE)
add_custom_command(
  OUTPUT version.h
  DEPENDS ${CMAKE_SOURCE_DIR}/VERSION
  COMMAND ${CMAKE_SOURCE_DIR}/mkversion ${CMAKE_SOURCE_DIR}
  COMMENT "Generating version header"
  VERBATIM
)
include_directories(${CMAKE_BINARY_DIR})


# Set plugin type (south, north, filter)
set(PLUGIN_TYPE "south")
# Add here all needed Fledge libraries as list
set(NEEDED_FLEDGE_LIBS common-lib)

# Find source files
file(GLOB SOURCES *.cpp)

# Find Fledge includes and libs, by including FindFledge.cmak file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Fledge)
# If errors: make clean and remove Makefile
if (NOT FLEDGE_FOUND)
	if (EXISTS "${CMAKE_BINARY_DIR}/Makefile")
		execute_process(COMMAND make clean WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		file(REMOVE "${CMAKE_BINARY_DIR}/Makefile")
	endif()
	# Stop the build process
	message(FATAL_ERROR "Fledge plugin '${PROJECT_NAME}' build error.")
endif()
# On success, FLEDGE_INCLUDE_DIRS and FLEDGE_LIB_DIRS variables are set 


# Find the lib60870 files
if (NOT "$ENV{LIB_104}" STREQUAL "")
	set(LIB_104 "$ENV{LIB_104}/lib60870-C")
else()
	set(LIB_104 "${CMAKE_SOURCE_DIR}/lib60870/lib60870-C")
	message(STATUS "LIB_104 set to " ${LIB_104})
endif()

# Add ./include
include_directories(include)
# Add Fledge include dir(s)
include_directories(${FLEDGE_INCLUDE_DIRS})

# Add Fledge lib path
link_directories(${FLEDGE_LIB_DIRS})
# Create shared library
link_directories(${LIB_104}/build/src)

if (FLEDGE_SRC)
	message(STATUS "Using third-party includes " ${FLEDGE_SRC}/C/thirdparty)
	include_directories(${FLEDGE_SRC}/C/thirdparty/rapidjson/include)
endif()

# Add other include paths
if (NOT EXISTS "${LIB_104}/src")
	message(FATAL_ERROR "LIB_104 ${LIB_104}/src \n"
			"does not appear to be pointing at a valid lib60870 source tree")
	return()
endif()
include_directories(${LIB_104}/src/hal/inc)
include_directories(${LIB_104}/src/common/inc)
include_directories(${LIB_104}/src/inc/api)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES} version.h)

# Add Fledge library names
target_link_libraries(${PROJECT_NAME} ${NEEDED_FLEDGE_LIBS})

# Add the lib60870
find_library(LIB60870 lib60870.a "${LIB_104}/build")
if (NOT LIB60870)
	message(FATAL_ERROR "The 104 library lib60870 was not found in ${LIB_104}/build/src\n"
			"Please build the library and set the environment variable LIB_104 to root of lib60870 source tree")
	return()
endif()

target_link_libraries(${PROJECT_NAME} -L${LIB_104}/build/src -llib60870)

# Add additional libraries
target_link_libraries(${PROJECT_NAME} -lpthread -ldl)

# Set the build version 
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)

set(FLEDGE_INSTALL "" CACHE INTERNAL "")
# Install library
if (FLEDGE_INSTALL)
	message(STATUS "Installing ${PROJECT_NAME} in ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME})
endif()
