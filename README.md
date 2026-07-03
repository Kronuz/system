# system

Process- and machine-level resource introspection extracted from
[Xapiand](https://github.com/Kronuz/Xapiand): file-descriptor counts, RAM / virtual
memory, disk and inode capacity, and compiler / OS / architecture strings.

```cpp
#include "system.hh"
#include "memory_stats.h"

get_open_files_per_proc();   get_max_files_per_proc();      // fd usage vs the rlimit
get_total_ram();             get_current_memory_by_process(); // bytes
get_total_disk_size();       get_free_disk_size();          // bytes
get_total_inodes();          get_free_inodes();
check_compiler();  check_OS();  check_architecture();       // human-readable strings
```

## What's in it

- **`system.hh`** — `get_{open,max}_files_per_proc()`, `get_{open,max}_files_system_wide()`
  (the fd rlimit and current usage), and `check_compiler()` / `check_OS()` /
  `check_architecture()` (build/runtime identification strings).
- **`memory_stats.h`** — `get_total_ram()`, `get_current_memory_by_process(resident)`,
  `get_total_virtual_memory()`, `get_total_inodes()` / `get_free_inodes()`,
  `get_total_disk_size()` / `get_free_disk_size()`, plus `get_current_ram()` /
  `get_total_virtual_used()` on Apple.

Each probe reads the real machine (`/proc`, `sysctl`, `statvfs`, `getrlimit`, ...) and
is `__APPLE__` / `__linux__` / BSD aware.

## Standalone, portable

Feature detection is portable (`__has_include(<sys/sysctl.h>)` instead of a generated
`config.h`). Dependencies are minimal: **io** (EINTR-safe `/proc` reads) and **strings**
(`format`). Logging is an **optional** seam: the `L_*` family is no-op by default
(`system_trace.h`), and the `error::` helper it would log appears only inside those
macros' arguments, so a host only needs it if it opts into real logging via
`SYSTEM_TRACE_HEADER`.

## Build / test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
ctest --test-dir build     # asserts plausible, self-consistent probe results
```

## Use (FetchContent)

```cmake
FetchContent_Declare(system GIT_REPOSITORY https://github.com/Kronuz/system.git GIT_TAG main)
FetchContent_MakeAvailable(system)
target_link_libraries(your_app PRIVATE system::system)
```
