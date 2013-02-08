#/**********************************************************\ 
# 
# Auto-generated CMakeLists.txt for the BlabblePhone project
#
#\**********************************************************/

# Written to work with cmake 2.6
cmake_minimum_required (VERSION 2.6)
set (CMAKE_BACKWARDS_COMPATIBILITY 2.6)

Project(${PLUGIN_NAME})

file (GLOB GENERAL RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    [^.]*.cpp
    [^.]*.h
    [^.]*.cmake
    )

if (WIN32)
INCLUDE_DIRECTORIES(
	${PLUGIN_INCLUDE_DIRS}
	./external/pjsip/pjlib/include
	./external/pjsip/pjlib-util/include
	./external/pjsip/pjmedia/include
	./external/pjsip/pjsip/include
	./external/pjsip/pjnath/include
)

SET(PLUGIN_INTERNAL_DEPS 
	${PLUGIN_INTERNAL_DEPS}
	Iphlpapi.lib
	netapi32.lib
	mswsock.lib
	ws2_32.lib
	$(MSBuildProjectDirectory)\\..\\..\\..\\projects\\Blabble\\external\\libpjproject-SVN-4339-Release.lib
)

ADD_DEFINITIONS(-DPJ_WIN32=1)

endif()

if (UNIX)

INCLUDE_DIRECTORIES(
	${PLUGIN_INCLUDE_DIRS}
)

find_package(PkgConfig)

pkg_check_modules(PJSIP REQUIRED libpjproject)

SET(PLUGIN_INTERNAL_DEPS 
	${PLUGIN_INTERNAL_DEPS}
	${PJSIP_LIBRARIES}
)

#Add GCC debug flags when in debug build
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -O0 -g3" )

endif()

if (APPLE)

FIND_LIBRARY(AUDIOTOOLBOX_LIBRARY AudioToolbox)
FIND_LIBRARY(AUDIOUNIT_LIBRARY AudioUnit)
FIND_LIBRARY(COREAUDIO_LIBRARY CoreAudio)

MARK_AS_ADVANCED(AUDIOTOOLBOX_LIBRARY
				AUDIOUNIT_LIBRARY
				COREAUDIO_LIBRARY)
SET(EXTRA_LIBS ${AUDIOTOOLBOX_LIBRARY} ${AUDIOUNIT_LIBRARY} ${COREAUDIO_LIBRARY})

INCLUDE_DIRECTORIES(
	${PLUGIN_INCLUDE_DIRS}
	/usr/local/include
)

SET(PLUGIN_INTERNAL_DEPS 
	${PLUGIN_INTERNAL_DEPS}
	${EXTRA_LIBS}
)

endif()
	
# Generated files are stored in ${GENERATED} by the project configuration
SET_SOURCE_FILES_PROPERTIES(
    ${GENERATED}
    PROPERTIES
        GENERATED 1
    )

SOURCE_GROUP(Generated FILES
    ${GENERATED}
    )

SET( SOURCES
    ${GENERAL}
    ${GENERATED}
    )

# This will include Win/projectDef.cmake, X11/projectDef.cmake, Mac/projectDef 
# depending on the platform
include_platform()

