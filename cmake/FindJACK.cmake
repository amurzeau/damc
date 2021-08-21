# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindJACK
-----------

Find the JACK libraries

Set JACK_ROOT cmake or environment variable to the JACK install root directory
to use a specific JACK installation.

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``JACK::jack``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``JACK_INCLUDE_DIRS``
  where to find jack.h, etc.
``JACK_LIBRARIES``
  the libraries to link against to use JACK.
``JACK_FOUND``
  TRUE if found

#]=======================================================================]

if(WIN32)
	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(JACK_DEFAULT_PATHS "C:/Program Files (x86)/JACK2;C:/Program Files/JACK2")
		set(JACK_DEFAULT_NAME "jack")
		set(JACK_DEFAULT_LIB_SUFFIX "lib32;lib")
	else()
		set(JACK_DEFAULT_PATHS "C:/Program Files/JACK2")
		set(JACK_DEFAULT_NAME "jack64;jack")
		set(JACK_DEFAULT_LIB_SUFFIX "lib")
	endif()
	set(CMAKE_FIND_LIBRARY_PREFIXES "lib;")
else()
	set(JACK_DEFAULT_NAME "jack")
	set(JACK_DEFAULT_LIB_SUFFIX "lib")
endif()

find_package (PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules (PC_JACK jack>=0.100.0)
endif()

# Look for the necessary header
find_path(JACK_INCLUDE_DIR
	NAMES jack/jack.h
	PATH_SUFFIXES include includes
	HINTS
		${PC_JACK_INCLUDE_DIRS}
	PATHS
		${JACK_DEFAULT_PATHS}
)
mark_as_advanced(JACK_INCLUDE_DIR)

# Look for the necessary library
find_library(JACK_LIBRARY
	NAMES ${PC_JACK_LIBRARIES} ${JACK_DEFAULT_NAME}
	NAMES_PER_DIR
	PATH_SUFFIXES ${JACK_DEFAULT_LIB_SUFFIX}
	HINTS
		${PC_JACK_LIBRARY_DIRS}
	PATHS
		${JACK_DEFAULT_PATHS}
)
mark_as_advanced(JACK_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JACK
	REQUIRED_VARS JACK_LIBRARY JACK_INCLUDE_DIR)

# Create the imported target
if(JACK_FOUND)
	set(JACK_INCLUDE_DIRS ${JACK_INCLUDE_DIR})
	set(JACK_LIBRARIES ${JACK_LIBRARY})
	if(NOT TARGET JACK::jack)
		add_library(JACK::jack UNKNOWN IMPORTED)
		set_target_properties(JACK::jack PROPERTIES IMPORTED_LOCATION "${JACK_LIBRARY}")
		target_include_directories(JACK::jack INTERFACE "${JACK_INCLUDE_DIR}")
	endif()
else()
	message(STATUS "Set JACK_ROOT cmake or environment variable to the JACK install root directory to use a specific JACK installation.")
endif()
