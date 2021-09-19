cmake_minimum_required(VERSION 2.8)
project(uv-test)

set(TEST_TARGET_NAME "run-tests")
set(BENCH_TARGET_NAME "run-benchmarks")

include(../DetectOS.cmake)

set(SOURCE_FILES_COMMON
  blackhole-server.c
  echo-server.c
  runner.c
  runner.h
  task.h
)

if(DETECTED_OS_WIN)
  set(SOURCE_FILES_RUNNER
    runner-win.c
    runner-win.h
    ../src/win/snprintf.c
  )
else()
  set(SOURCE_FILES_RUNNER
    runner-unix.c
    runner-unix.h
  )
endif()


# Unit tests
set(SOURCE_FILES_TEST
  run-tests.c
  test-get-loadavg.c
  test-active.c
  test-async.c
  test-async-null-cb.c
  test-callback-stack.c
  test-callback-order.c
  test-close-fd.c
  test-close-order.c
  test-connection-fail.c
  test-cwd-and-chdir.c
  test-default-loop-close.c
  test-delayed-accept.c
  test-eintr-handling.c
  test-error.c
  test-embed.c
  test-emfile.c
  test-env-vars.c
  test-fail-always.c
  test-fork.c
  test-fs.c
  test-fs-copyfile.c
  test-fs-event.c
  test-get-currentexe.c
  test-get-memory.c
  test-get-passwd.c
  test-getaddrinfo.c
  test-gethostname.c
  test-getnameinfo.c
  test-getsockname.c
  test-handle-fileno.c
  test-homedir.c
  test-hrtime.c
  test-idle.c
  test-ipc.c
  test-ipc-send-recv.c
  test-list.h
  test-loop-handles.c
  test-loop-alive.c
  test-loop-close.c
  test-loop-stop.c
  test-loop-time.c
  test-loop-configure.c
  test-walk-handles.c
  test-watcher-cross-stop.c
  test-multiple-listen.c
  test-osx-select.c
  test-pass-always.c
  test-ping-pong.c
  test-pipe-bind-error.c
  test-pipe-connect-error.c
  test-pipe-connect-multiple.c
  test-pipe-connect-prepare.c
  test-pipe-getsockname.c
  test-pipe-pending-instances.c
  test-pipe-sendmsg.c
  test-pipe-server-close.c
  test-pipe-close-stdout-read-stdin.c
  test-pipe-set-non-blocking.c
  test-platform-output.c
  test-poll.c
  test-poll-close.c
  test-poll-close-doesnt-corrupt-stack.c
  test-poll-closesocket.c
  test-poll-oob.c
  test-process-title.c
  test-queue-foreach-delete.c
  test-ref.c
  test-run-nowait.c
  test-run-once.c
  test-semaphore.c
  test-shutdown-close.c
  test-shutdown-eof.c
  test-shutdown-twice.c
  test-signal.c
  test-signal-multiple-loops.c
  test-socket-buffer-size.c
  test-spawn.c
  test-fs-poll.c
  test-stdio-over-pipes.c
  test-tcp-alloc-cb-fail.c
  test-tcp-bind-error.c
  test-tcp-bind6-error.c
  test-tcp-close.c
  test-tcp-close-accept.c
  test-tcp-close-while-connecting.c
  test-tcp-create-socket-early.c
  test-tcp-connect-error-after-write.c
  test-tcp-shutdown-after-write.c
  test-tcp-flags.c
  test-tcp-connect-error.c
  test-tcp-connect-timeout.c
  test-tcp-connect6-error.c
  test-tcp-open.c
  test-tcp-write-to-half-open-connection.c
  test-tcp-write-after-connect.c
  test-tcp-writealot.c
  test-tcp-write-fail.c
  test-tcp-try-write.c
  test-tcp-unexpected-read.c
  test-tcp-oob.c
  test-tcp-read-stop.c
  test-tcp-write-queue-order.c
  test-threadpool.c
  test-threadpool-cancel.c
  test-thread-equal.c
  test-tmpdir.c
  test-mutexes.c
  test-thread.c
  test-barrier.c
  test-condvar.c
  test-timer-again.c
  test-timer-from-check.c
  test-timer.c
  test-tty.c
  test-udp-alloc-cb-fail.c
  test-udp-bind.c
  test-udp-create-socket-early.c
  test-udp-dgram-too-big.c
  test-udp-ipv6.c
  test-udp-open.c
  test-udp-options.c
  test-udp-send-and-recv.c
  test-udp-send-hang-loop.c
  test-udp-send-immediate.c
  test-udp-send-unreachable.c
  test-udp-multicast-join.c
  test-udp-multicast-join6.c
  test-dlerror.c
  test-udp-multicast-ttl.c
  test-ip4-addr.c
  test-ip6-addr.c
  test-udp-multicast-interface.c
  test-udp-multicast-interface6.c
  test-udp-try-send.c
)
add_executable(${TEST_TARGET_NAME} ${SOURCE_FILES_TEST} ${SOURCE_FILES_COMMON} ${SOURCE_FILES_RUNNER})
target_link_libraries(${TEST_TARGET_NAME} uv)
if(NOT DETECTED_OS_WIN32 AND NOT DETECTED_OS_ZOS)
  target_compile_definitions(${TEST_TARGET_NAME} PRIVATE _GNU_SOURCE)
endif()

if(DETECTED_OS_MAC OR DETECTED_OS_FREEBSD OR DETECTED_OS_LINUX OR DETECTED_OS_NETBSD OR DETECTED_OS_OPENBSD)
  target_link_libraries(${TEST_TARGET_NAME} util)
endif()

# make test-fs.c compile
if(DETECTED_OS_SOLARIS)
  target_compile_definitions(${TEST_TARGET_NAME} PRIVATE __EXTENSIONS__ _XOPEN_SOURCE=500)
elseif(DETECTED_OS_AIX)
  target_compile_definitions(${TEST_TARGET_NAME} PRIVATE _ALL_SOURCE _XOPEN_SOURCE=500)
endif()


# Benchmark tests
set(SOURCE_FILES_BENCH
  benchmark-async.c
  benchmark-async-pummel.c
  benchmark-fs-stat.c
  benchmark-getaddrinfo.c
  benchmark-list.h
  benchmark-loop-count.c
  benchmark-million-async.c
  benchmark-million-timers.c
  benchmark-multi-accept.c
  benchmark-ping-pongs.c
  benchmark-pound.c
  benchmark-pump.c
  benchmark-sizes.c
  benchmark-spawn.c
  benchmark-thread.c
  benchmark-tcp-write-batch.c
  benchmark-udp-pummel.c
  dns-server.c
  run-benchmarks.c
)
add_executable(${BENCH_TARGET_NAME} ${SOURCE_FILES_BENCH} ${SOURCE_FILES_COMMON} ${SOURCE_FILES_RUNNER})
target_link_libraries(${BENCH_TARGET_NAME} uv)
if(NOT DETECTED_OS_WIN32)
  target_compile_definitions(${TEST_TARGET_NAME} PRIVATE _GNU_SOURCE)
endif()
