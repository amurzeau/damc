# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindJack
-----------

Find the Jack libraries

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Jack::Jack``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``Jack_INCLUDE_DIRS``
  where to find Jack.h, etc.
``Jack_LIBRARIES``
  the libraries to link against to use Jack.
``Jack_FOUND``
  TRUE if found

#]=======================================================================]

if(WIN32)
	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(JACK_DEFAULT_PATHS "C:/Program Files (x86)/JACK2;C:/Program Files/JACK2")
		set(JACK_DEFAULT_NAME "libjack")
		set(JACK_DEFAULT_LIB_SUFFIX "lib;lib32")
	else()
		set(JACK_DEFAULT_PATHS "C:/Program Files/JACK2")
		set(JACK_DEFAULT_NAME "libjack64")
		set(JACK_DEFAULT_LIB_SUFFIX "lib")
	endif()
else()
	set(JACK_DEFAULT_LIB_SUFFIX "lib")
endif()

message(STATUS "Using hint ${JACK_ROOT_DIR}")
message(STATUS "Using path ${JACK_DEFAULT_PATHS}")

# Look for the necessary header
find_path(Jack_INCLUDE_DIR
	NAMES jack/jack.h
	PATH_SUFFIXES include includes
	HINTS
		${JACK_ROOT_DIR}
		ENV JACK_ROOT_DIR
	PATHS
		${JACK_DEFAULT_PATHS}
)
mark_as_advanced(Jack_INCLUDE_DIR)

# Look for the necessary library
find_library(Jack_LIBRARY
	NAMES jack ${JACK_DEFAULT_NAME} ${JACK_DEFAULT_NAME}.lib
	NAMES_PER_DIR
	PATH_SUFFIXES ${JACK_DEFAULT_LIB_SUFFIX}
	HINTS
		${JACK_ROOT_DIR}
		ENV JACK_ROOT_DIR
	PATHS
		${JACK_DEFAULT_PATHS}
)
mark_as_advanced(Jack_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jack
    REQUIRED_VARS Jack_INCLUDE_DIR Jack_LIBRARY)

# Create the imported target
if(Jack_FOUND)
    set(Jack_INCLUDE_DIRS ${Jack_INCLUDE_DIR})
    set(Jack_LIBRARIES ${Jack_LIBRARY})
    if(NOT TARGET Jack::Jack)
        add_library(Jack::Jack UNKNOWN IMPORTED)
        set_target_properties(Jack::Jack PROPERTIES
            IMPORTED_LOCATION             "${Jack_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Jack_INCLUDE_DIR}")
    endif()
endif()
