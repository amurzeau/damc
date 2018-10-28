# $Id: $
#
# - Try to find the DirectX SDK
# Once done this will define
#
#  DXSDK_FOUND - system has DirectX SDK
#  DXSDK_ROOT_DIR - path to the DirectX SDK base directory
#  DXSDK_INCLUDE_DIR - the DirectX SDK include directory
#  DXSDK_LIBRARY_DIR - DirectX SDK libraries path
#
#  DXSDK_DSOUND_LIBRARY - Path to dsound.lib
#

if(WIN32)
else(WIN32)
  message(FATAL_ERROR "FindDXSDK.cmake: Unsupported platform ${CMAKE_SYSTEM_NAME}" )
endif(WIN32)

if(MINGW)
	set(DXSDK_ROOT_DIR "")
	set(DXSDK_INCLUDE_DIR "")
	set(DXSDK_LIBRARY_DIR "")
	set(DXSDK_DSOUND_LIBRARY "-ldsound")
	set(DXSDK_FOUND TRUE)
else(MINGW)
	find_path(DXSDK_ROOT_DIR
	  include/dsound.h
	  HINTS
	    $ENV{DXSDK_DIR}
	)

	find_path(DXSDK_INCLUDE_DIR
	  dsound.h
	  PATHS
	    ${DXSDK_ROOT_DIR}/include
	)

	IF(CMAKE_CL_64)
	find_path(DXSDK_LIBRARY_DIR
	  dsound.lib
	  PATHS
	  ${DXSDK_ROOT_DIR}/lib/x64
	)
	ELSE(CMAKE_CL_64)
	find_path(DXSDK_LIBRARY_DIR
	  dsound.lib
	  PATHS
	  ${DXSDK_ROOT_DIR}/lib/x86
	)
	ENDIF(CMAKE_CL_64)

	find_library(DXSDK_DSOUND_LIBRARY
	  dsound.lib
	  PATHS
	  ${DXSDK_LIBRARY_DIR}
	)
endif(MINGW)



# handle the QUIETLY and REQUIRED arguments and set DXSDK_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DXSDK DEFAULT_MSG DXSDK_DSOUND_LIBRARY)

MARK_AS_ADVANCED(
    DXSDK_ROOT_DIR DXSDK_INCLUDE_DIR
    DXSDK_LIBRARY_DIR DXSDK_DSOUND_LIBRARY
)
