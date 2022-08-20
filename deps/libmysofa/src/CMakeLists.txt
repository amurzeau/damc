macro(use_c99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=gnu99 -pedantic ${CMAKE_C_FLAGS}")
    endif ()
  else ()
    set (CMAKE_C_STANDARD 99)
  endif ()
endmacro(use_c99)

use_c99()

configure_file (config.h.in ${PROJECT_SOURCE_DIR}/src/config.h)

set(libsrc
	hdf/superblock.c
	hdf/dataobject.c
	hdf/btree.c
	hdf/fractalhead.c
	hdf/gunzip.c
	hdf/gcol.c
	hdf/reader.h
	hrtf/kdtree.h
	hrtf/mysofa.h
	hrtf/tools.h
	hrtf/check.c
	hrtf/spherical.c
	hrtf/lookup.c
	hrtf/tools.c
	hrtf/kdtree.c
	hrtf/neighbors.c
	hrtf/interpolate.c
	hrtf/reader.c
	hrtf/resample.c
	hrtf/loudness.c
	hrtf/minphase.c
	hrtf/easy.c
	hrtf/cache.c
	resampler/speex_resampler.c
	resampler/speex_resampler.h
	resampler/stack_alloc.h
	resampler/arch.h
	config.h)
add_library(mysofa ${libsrc})
target_link_libraries (mysofa zlib)
target_include_directories(mysofa PUBLIC "${CMAKE_CURRENT_LIST_DIR}/hrtf/")
target_compile_definitions(mysofa PRIVATE mysofa_shared_EXPORTS)
if(UNIX)
	target_link_libraries (mysofa m)
        target_compile_definitions(mysofa PUBLIC MYSOFA_EXPORT=)
endif()
SET_TARGET_PROPERTIES(mysofa PROPERTIES POSITION_INDEPENDENT_CODE ON)
install(TARGETS mysofa
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})


install(FILES hrtf/mysofa.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

SET(PKG_CONFIG_PRIVATELIBS "${PKG_CONFIG_PRIVATELIBS}" PARENT_SCOPE)

