cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME "uv")

include(../DetectOS.cmake)

set(SOURCE_FILES
	../include/uv.h
	../include/tree.h
	../include/uv-errno.h
	../include/uv-threadpool.h
	../include/uv-version.h
	fs-poll.c
	heap-inl.h
	inet.c
	queue.h
	threadpool.c
	uv-common.c
	uv-common.h
	version.c
	)

if(DETECTED_OS_WIN)
	set(SOURCE_FILES ${SOURCE_FILES}
		../include/uv-win.h
		win/async.c
		win/atomicops-inl.h
		win/core.c
		win/detect-wakeup.c
		win/dl.c
		win/error.c
		win/fs.c
		win/fs-event.c
		win/getaddrinfo.c
		win/getnameinfo.c
		win/handle.c
		win/handle-inl.h
		win/internal.h
		win/loop-watcher.c
		win/pipe.c
		win/thread.c
		win/poll.c
		win/process.c
		win/process-stdio.c
		win/req.c
		win/req-inl.h
		win/signal.c
		win/snprintf.c
		win/stream.c
		win/stream-inl.h
		win/tcp.c
		win/tty.c
		win/timer.c
		win/udp.c
		win/util.c
		win/winapi.c
		win/winapi.h
		win/winsock.c
		win/winsock.h
		)
else()
	set(SOURCE_FILES ${SOURCE_FILES}
		../include/uv-unix.h
		../include/uv-linux.h
		../include/uv-sunos.h
		../include/uv-darwin.h
		../include/uv-bsd.h
		../include/uv-aix.h
		unix/async.c
		unix/atomic-ops.h
		unix/core.c
		unix/dl.c
		unix/fs.c
		unix/getaddrinfo.c
		unix/getnameinfo.c
		unix/internal.h
		unix/loop.c
		unix/loop-watcher.c
		unix/pipe.c
		unix/poll.c
		unix/process.c
		unix/signal.c
		unix/spinlock.h
		unix/stream.c
		unix/tcp.c
		unix/thread.c
		unix/timer.c
		unix/tty.c
		unix/udp.c
		)
endif()

if(DETECTED_OS_LINUX OR DETECTED_OS_MAC OR DETECTED_OS_ANDROID OR DETECTED_OS_ZOS)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/proctitle.c
		)
endif()

if(DETECTED_OS_MAC)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/darwin.c
		unix/fsevents.c
		unix/darwin-proctitle.c
		)
endif()

if(DETECTED_OS_LINUX)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/linux-core.c
		unix/linux-inotify.c
		unix/linux-syscalls.c
		unix/linux-syscalls.h
		unix/procfs-exepath.c
		unix/sysinfo-loadavg.c
		unix/sysinfo-memory.c
		)
endif()

if(DETECTED_OS_ANDROID)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/linux-core.c
		unix/linux-inotify.c
		unix/linux-syscalls.c
		unix/linux-syscalls.h
		unix/pthread-fixes.c
		unix/android-ifaddrs.c
		unix/procfs-exepath.c
		unix/sysinfo-loadavg.c
		unix/sysinfo-memory.c
		)
endif()

if(DETECTED_OS_SOLARIS)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/no-proctitle.c
		unix/sunos.c
		)
endif()

if(DETECTED_OS_AIX)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/aix.c
		)
endif()

if(DETECTED_OS_FREEBSD)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/freebsd.c
		)
endif()

if(DETECTED_OS_OPENBSD)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/openbsd.c
		)
endif()

if(DETECTED_OS_NETBSD)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/netbsd.c
		)
endif()

if(DETECTED_OS_FREEBSD OR DETECTED_OS_OPENBSD OR DETECTED_OS_NETBSD)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/posix-hrtime.c
		)
endif()

if(DETECTED_OS_MAC OR DETECTED_OS_FREEBSD OR DETECTED_OS_OPENBSD OR DETECTED_OS_NETBSD)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/bsd-ifaddrs.c
		unix/kqueue.c
		)
endif()

if(DETECTED_OS_ZOS)
	set(SOURCE_FILES ${SOURCE_FILES}
		unix/pthread-fixes.c
		unix/no-fsevents.c
		unix/os390.c
		unix/os390-syscalls.c
		)
endif()

add_library(${TARGET_NAME} ${SOURCE_FILES})
target_include_directories(${TARGET_NAME} PUBLIC "../include/" PRIVATE "./")

if(BUILD_SHARED_LIBS)
	target_compile_definitions(${TARGET_NAME}
		PRIVATE BUILDING_UV_SHARED
		INTERFACE USING_UV_SHARED
		)
endif()

if(NOT DETECTED_OS_WIN)
	if(DETECTED_OS_SOLARIS)
		target_compile_options(${TARGET_NAME} PUBLIC -pthreads)
		target_link_libraries(${TARGET_NAME} -pthread)
	elseif(NOT DETECTED_OS_SOLARIS AND NOT DETECTED_OS_ANDROID AND NOT DETECTED_OS_ZOS)
		target_compile_options(${TARGET_NAME} PUBLIC -pthread)
		target_link_libraries(${TARGET_NAME} -pthread)
	endif()
endif()

if(DETECTED_OS_ZOS)
	target_compile_options(${TARGET_NAME} PUBLIC -qxplink)
	target_link_libraries(${TARGET_NAME} -qxplink)
	if(BUILD_SHARED_LIBS)
		target_compile_options(${TARGET_NAME} PRIVATE -qexportall)
		target_link_libraries(${TARGET_NAME} -Wl,DLL)
	endif()
	target_compile_definitions(${TARGET_NAME} PRIVATE _UNIX03_THREADS)
	target_compile_definitions(${TARGET_NAME} PRIVATE _UNIX03_SOURCE)
	target_compile_definitions(${TARGET_NAME} PRIVATE _UNIX03_WITHDRAWN)
	target_compile_definitions(${TARGET_NAME} PRIVATE _OPEN_SYS_IF_EXT)
	target_compile_definitions(${TARGET_NAME} PRIVATE _OPEN_SYS_SOCK_IPV6)
	target_compile_definitions(${TARGET_NAME} PRIVATE _OPEN_MSGQ_EXT)
	target_compile_definitions(${TARGET_NAME} PRIVATE _XOPEN_SOURCE_EXTENDED)
	target_compile_definitions(${TARGET_NAME} PRIVATE _ALL_SOURCE)
	target_compile_definitions(${TARGET_NAME} PRIVATE _LARGE_TIME_API)
	target_compile_definitions(${TARGET_NAME} PRIVATE _OPEN_SYS_FILE_EXT)
	target_compile_definitions(${TARGET_NAME} PRIVATE _AE_BIMODAL)
	target_compile_definitions(${TARGET_NAME} PRIVATE PATH_MAX=255)
endif()

if(DETECTED_OS_WIN)
	target_link_libraries(${TARGET_NAME} iphlpapi.lib)
	target_link_libraries(${TARGET_NAME} psapi.lib)
	target_link_libraries(${TARGET_NAME} shell32.lib)
	target_link_libraries(${TARGET_NAME} user32.lib)
	target_link_libraries(${TARGET_NAME} userenv.lib)
	target_link_libraries(${TARGET_NAME} ws2_32.lib)
	target_compile_definitions(${TARGET_NAME} PRIVATE _WIN32_WINNT=0x0600)
	target_compile_definitions(${TARGET_NAME} PRIVATE _GNU_SOURCE)
else()
	target_link_libraries(${TARGET_NAME} m)
	if(NOT DETECTED_OS_ZOS)
		target_compile_options(${TARGET_NAME} PRIVATE --std=gnu89 -pedantic -fvisibility=hidden)
	endif()
	target_compile_definitions(${TARGET_NAME} PUBLIC _LARGEFILE_SOURCE)
	target_compile_definitions(${TARGET_NAME} PUBLIC _FILE_OFFSET_BITS=64)
endif()

if(DETECTED_OS_LINUX)
	target_link_libraries(${TARGET_NAME} dl rt)
	target_compile_definitions(${TARGET_NAME} PUBLIC _POSIX_C_SOURCE=200112)
	target_compile_definitions(${TARGET_NAME} PRIVATE _GNU_SOURCE)
endif()

if(DETECTED_OS_MAC)
	target_compile_definitions(${TARGET_NAME} PUBLIC _DARWIN_USE_64_BIT_INODE=1)
	target_compile_definitions(${TARGET_NAME} PRIVATE _DARWIN_UNLIMITED_SELECT=1)
endif()

if(DETECTED_OS_ANDROID)
	target_link_libraries(${TARGET_NAME} dl)
endif()

if(DETECTED_OS_SOLARIS)
	target_link_libraries(${TARGET_NAME} kstat nsl sendfile socket)
	target_compile_definitions(${TARGET_NAME} PRIVATE __EXTENSIONS__ _XOPEN_SOURCE=500)
endif()

if(DETECTED_OS_AIX)
	target_link_libraries(${TARGET_NAME} perfstat)
	target_compile_definitions(${TARGET_NAME} PRIVATE _ALL_SOURCE)
	target_compile_definitions(${TARGET_NAME} PRIVATE _XOPEN_SOURCE=500)
	target_compile_definitions(${TARGET_NAME} PRIVATE _LINUX_SOURCE_COMPAT)
	target_compile_definitions(${TARGET_NAME} PRIVATE _THREAD_SAFE)
	target_compile_definitions(${TARGET_NAME} PRIVATE HAVE_SYS_AHAFS_EVPRODS_H)
endif()

if(DETECTED_OS_NETBSD)
	target_link_libraries(${TARGET_NAME} kvm)
endif()

if(BUILD_SHARED_LIBS)
	install(TARGETS ${TARGET_NAME}
		RUNTIME DESTINATION ./
		LIBRARY DESTINATION ./)
endif()

if(WIN32)
	set_target_properties(${TARGET_NAME} PROPERTIES UNITY_BUILD OFF)
endif()
