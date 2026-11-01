# system

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/system/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/system/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

Process and machine introspection for C++20: open files, memory, disk, and inodes on
Linux, macOS, and FreeBSD. Each number is one function call, which asks the platform's
own source (`/proc` and `statvfs` on Linux, `sysctl` and Mach calls on macOS and BSD)
every time, with no state or caching. Reach for it for health endpoints, startup
banners, and resource-limit checks.

## ✨ Highlights

- **File descriptors.** Open and maximum, per process and system-wide.
- **Memory.** Total RAM, the process's resident or virtual size, and total virtual
  memory.
- **Disk.** Total and free bytes and inodes of the filesystem that holds the current
  directory.
- **Identification.** `check_compiler()`, `check_OS()`, and `check_architecture()`
  return printable strings.
- **No generated configuration.** Platform features are detected with `__has_include`;
  logging is an optional header seam (`SYSTEM_TRACE_HEADER`) that defaults to no-ops.

## 🚀 Quick start

```cpp
#include "memory_stats.h"
#include "system.hh"

#include <cstdio>

int main() {
	std::printf("fds   %zu / %zu\n", get_open_files_per_proc(), get_max_files_per_proc());
	std::printf("rss   %llu of %llu bytes\n",
		(unsigned long long)get_current_memory_by_process(), (unsigned long long)get_total_ram());
	std::printf("disk  %llu free\n", (unsigned long long)get_free_disk_size());
	std::printf("%s on %s (%s)\n", check_compiler().c_str(), check_OS().c_str(), check_architecture().c_str());
}
// prints, for example:
// fds   3 / 10240
// rss   1540096 of 17179869184 bytes
// disk  36772392960 free
// Clang v17.0.0 on macOS (aarch64)
```

[`examples/usage.cc`](examples/usage.cc) is a self-checking version that runs under
`ctest`.

## 📦 Installation

Requires a C++20 compiler and CMake 3.16 or newer. It brings
[io](https://github.com/Kronuz/io) and [strings](https://github.com/Kronuz/strings)
through FetchContent and builds one static library. CI builds and tests it with GCC and
Clang on Linux (x86_64, arm64), macOS (arm64, x86_64), and FreeBSD, plus ASan, UBSan,
and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(system
  GIT_REPOSITORY https://github.com/Kronuz/system.git
  GIT_TAG        e329a47f29ec859f40369481b8f1a69175027c98)  # or main, which tracks the tip
FetchContent_MakeAvailable(system)

target_link_libraries(my_app PRIVATE system::system)
```

Then `#include "system.hh"` for descriptors and identification, and
`#include "memory_stats.h"` for memory, disk, and inodes.

## 🎯 When to use it

Health endpoints, startup banners, and checks against resource limits, where one
process- or machine-wide number per call is what you need.

Not for:

- **Per-thread or cgroup-aware accounting.** The numbers are process- and machine-wide.
- **Hot paths.** Every call re-queries the operating system; nothing is cached.
- **Windows.** The probes read `/proc`, `sysctl`, and POSIX `statvfs`.

## 📚 Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md): where each number comes from on each platform,
  and the logging seam.
- [AGENTS.md](AGENTS.md): repository map and invariants.
- [Catalog entry](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#system).

## 🛠️ Building and testing

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) first, and the
[catalog's AGENTS.md](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md) for
the family's rules. CI must pass on every platform.

## 📄 License

MIT; see [LICENSE](LICENSE). Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).
