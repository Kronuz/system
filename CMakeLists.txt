# system: process / system resource introspection extracted from Xapiand.
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
#
# Two compiled units (system.cc + memory_stats.cc) plus their headers. Portable feature
# detection (no config.h). Depends on io (EINTR-safe proc reads) and strings; logging is
# an optional header seam (SYSTEM_TRACE_HEADER) that defaults to a no-op, so the
# error-string helper is only needed if a host opts into real logging.

cmake_minimum_required(VERSION 3.16)
project(system LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# --- Dependencies (the Kronuz family, tracked at tip) ---
#   io      -> io::open/read/unchecked_fcntl (io.hh)   [system.cc, the proc/fd probes]
#   strings -> strings::format               (strings.hh)
FetchContent_Declare(io      GIT_REPOSITORY https://github.com/Kronuz/io.git      GIT_TAG main)
FetchContent_Declare(strings GIT_REPOSITORY https://github.com/Kronuz/strings.git GIT_TAG main)
FetchContent_MakeAvailable(io strings)

# --- Library target ---
add_library(system STATIC system.cc memory_stats.cc)
target_include_directories(system PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(system PUBLIC cxx_std_20)
target_link_libraries(system PUBLIC io::io strings::strings)
add_library(system::system ALIAS system)

# Build the test only when this repo is the top-level project.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	enable_testing()
	add_executable(system_test test/test.cc)
	target_link_libraries(system_test PRIVATE system)
	add_test(NAME system COMMAND system_test)

	# A realistic usage example mirroring Xapiand's /:metrics and resource-limit
	# probes. It self-checks and is ALSO registered as a ctest regression test.
	add_executable(system_usage examples/usage.cc)
	target_link_libraries(system_usage PRIVATE system)
	add_test(NAME system_usage COMMAND system_usage)
endif()
