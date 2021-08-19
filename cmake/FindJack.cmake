# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindJack
-----------

Find the Jack libraries

Set Jack_ROOT cmake or environment variable to the Jack install root directory
to use a specific Jack installation.

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
		set(JACK_DEFAULT_LIB_SUFFIX "lib32;lib")
	else()
		set(JACK_DEFAULT_PATHS "C:/Program Files/JACK2")
		set(JACK_DEFAULT_NAME "libjack64;libjack64.lib")
		set(JACK_DEFAULT_LIB_SUFFIX "lib")
	endif()
else()
	set(JACK_DEFAULT_LIB_SUFFIX "lib")
endif()

find_package (PkgConfig)
if(PKG_CONFIG_FOUND)
	pkg_check_modules (PC_JACK jack>=0.100.0)
endif()

# Look for the necessary header
find_path(Jack_INCLUDE_DIR
	NAMES jack/jack.h
	PATH_SUFFIXES include includes
	HINTS
		${PC_JACK_INCLUDE_DIRS}
	PATHS
		${JACK_DEFAULT_PATHS}
)
mark_as_advanced(Jack_INCLUDE_DIR)

# Look for the necessary library
find_library(Jack_LIBRARY
	NAMES jack ${PC_JACK_LIBRARIES} ${JACK_DEFAULT_NAME} ${JACK_DEFAULT_NAME}.lib
	NAMES_PER_DIR
	PATH_SUFFIXES ${JACK_DEFAULT_LIB_SUFFIX}
	HINTS
		${PC_JACK_LIBRARY_DIRS}
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
        set_target_properties(Jack::Jack PROPERTIES IMPORTED_LOCATION "${Jack_LIBRARY}")
        target_include_directories(Jack::Jack INTERFACE "${Jack_INCLUDE_DIR}")
    endif()
endif()
