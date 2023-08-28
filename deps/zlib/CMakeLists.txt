cmake_minimum_required(VERSION 3.13)

add_library(zlib
  adler32.c
  compress.c
  crc32.c
  crc32.h
  deflate.c
  deflate.h
  inffast.c
  inffast.h
  inffixed.h
  inflate.c
  inflate.h
  inftrees.c
  inftrees.h
  trees.c
  trees.h
  uncompr.c
  zconf.h
  zlib.h
  zutil.c
  zutil.h
)

target_include_directories(zlib PUBLIC ".")
target_compile_definitions(zlib PUBLIC ZLIB_CONST)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(zlib PUBLIC ZLIB_DLL)
endif()
