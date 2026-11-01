# AGENTS

Orientation for anyone (human or agent) working on this library.

## What this is

Process/machine resource introspection extracted verbatim from Xapiand: fd counts,
RAM/virtual memory, disk/inode capacity, and compiler/OS/architecture strings. Two
compiled units (`system.cc`, `memory_stats.cc`) plus their headers. Read
`ARCHITECTURE.md` first.

## File map

```
system.hh / system.cc            fd-count + compiler/OS/architecture probes.
memory_stats.h / memory_stats.cc RAM / virtual-memory / disk / inode probes.
system_trace.h                   No-op L_* logging defaults (redirect via SYSTEM_TRACE_HEADER).
likely.h                         Vendored likely()/unlikely().
test/test.cc                     ctest: asserts plausible, self-consistent probe results.
examples/usage.cc                ctest-wired example mirroring Xapiand's metrics/resource probes.
```

## Dependencies (Kronuz family, via FetchContent at tip)

`io` (EINTR-safe /proc reads), `strings` (format). Logging is NOT a dependency: the
`L_*` family is no-op by default, and `error::` appears only inside L_* arguments, so
errno-names is pulled in only when a host opts into real logging via `SYSTEM_TRACE_HEADER`.

## Conventions / invariants

- **Verbatim behavior.** Byte-for-byte from Xapiand's `system.*` / `memory_stats.*`.
  Do not change what a probe measures or its units.
- **Portable feature detection, no config.h.** `<sys/sysctl.h>` is found via
  `__has_include` (`SYSTEM_HAVE_SYS_SYSCTL_H`); add capabilities the same way.
- **Reads go through `io::`**, not raw syscalls.
- **One seam only: `SYSTEM_TRACE_HEADER`.** `error::` may appear only inside L_*
  arguments; referencing it outside adds a hard dependency -- don't.
- **Functions are in the global namespace** (verbatim from Xapiand).

## Build / test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
```

## Verification and benchmarks

The method shared by the Kronuz libraries is in
[docs/PERFORMANCE.md](https://github.com/Kronuz/cpp-libraries/blob/main/docs/PERFORMANCE.md).

- **Tests.** The command above runs `system` (`test/test.cc`) and `system_usage`
  (`examples/usage.cc`). The probes read the real machine, so there is no exact oracle: the tests
  assert plausible, self-consistent results (positive totals, free at most total, non-empty
  compiler, OS and architecture strings).
- **CI and sanitizers.** CI (`.github/workflows/ci.yml`, which calls the shared
  [cmake.yml](https://github.com/Kronuz/cpp-libraries/blob/main/.github/workflows/cmake.yml))
  runs both on Linux x86-64 and arm64, macOS arm64 and x86-64, and FreeBSD, and in an ASan+UBSan
  build and a TSan build with clang on Linux. The ASan+UBSan job is:

  ```sh
  CC=clang CXX=clang++ cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -D_GLIBCXX_ASSERTIONS" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  cmake --build build-asan && ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure
  ```

- **Not here.** The repository has no benchmark target, differential test, fuzz target or
  mutation script.

## Host integration (how Xapiand uses it)

Xapiand compiles `system.cc` + `memory_stats.cc` into its own object list with
`-D SYSTEM_TRACE_HEADER=<xapiand_system_trace.h>`, so the probes' logging lands in
Xapiand's logger; the headers come from this repo's include dir via FetchContent. A host
that doesn't need logging just links the static `system::system` target.
