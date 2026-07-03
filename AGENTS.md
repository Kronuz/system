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

## Host integration (how Xapiand uses it)

Xapiand compiles `system.cc` + `memory_stats.cc` into its own object list with
`-D SYSTEM_TRACE_HEADER=<xapiand_system_trace.h>`, so the probes' logging lands in
Xapiand's logger; the headers come from this repo's include dir via FetchContent. A host
that doesn't need logging just links the static `system::system` target.
