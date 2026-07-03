# Architecture

`system` is a flat set of free functions, each of which reads one piece of OS/process
resource state. No state, no caching -- every call re-queries the machine. The only
subtlety is per-platform source selection.

![system: each probe reads its data from a per-platform source](assets/architecture.svg)

<!-- Diagram: assets/architecture.svg. Edit the D2 source below and re-render with:
     d2 --theme 0 --pad 20 <this-source>.d2 assets/architecture.svg

```d2
# system: each probe reads its data from a per-platform source.
direction: down
api: "system / memory_stats API\n(fd counts, RAM, disk, inodes, compiler/OS/arch)" { style.fill: "#e8f5ee" }
linux: "Linux" { style.fill: "#eef2f7"; proc: "/proc (via io::)"; sv: "statvfs / getrlimit" }
apple: "macOS / BSD" { style.fill: "#eef2f7"; sysctl: "sysctl MIBs\n(__has_include<sys/sysctl.h>)"; mach: "Mach / statvfs" }
seam: "SYSTEM_TRACE_HEADER\n(L_* + error::; no-op default)" { style.fill: "#faf3e6" }
api -> linux.proc; api -> linux.sv; api -> apple.sysctl; api -> apple.mach
api -> seam: "on probe error" { style.stroke-dash: 3 }
```
-->

## Dependencies

Kronuz libraries, fetched at tip: **`io`** (EINTR-safe `/proc` reads on Linux) and
**`strings`** (`format`). `likely.h` is vendored. Logging is **not** a dependency: the
`L_*` family is no-op by default and its `error::` arguments are dropped unevaluated, so
an errno-names library is pulled in only when a host opts in via `SYSTEM_TRACE_HEADER`.

## Files

```
system.hh / system.cc            fd-count + compiler/OS/architecture probes.
memory_stats.h / memory_stats.cc RAM / virtual-memory / disk / inode probes.
system_trace.h                   No-op L_* logging defaults (redirect via SYSTEM_TRACE_HEADER).
likely.h                         Vendored likely()/unlikely() (system.cc fd loop).
```

Both `.cc` compile into one static library.

## Per-platform sourcing

Each probe picks its data source by platform macro:

- **Linux** reads `/proc` (`/proc/self/fd`, `/proc/meminfo`, `/proc/sys/fs/file-nr`) via
  the `io::` layer, and `statvfs` / `getrlimit` for disk and fd limits.
- **macOS / BSD** use `sysctl` MIBs (`kern.openfiles`, `kern.maxfiles`, `hw.memsize`,
  `vm.*`) and Mach calls. `<sys/sysctl.h>` is detected with
  `__has_include` (`SYSTEM_HAVE_SYS_SYSCTL_H`), since modern glibc dropped that header.

## The logging seam

Logging is the one host coupling and it is an optional compile-time seam. Each `.cc`
includes `SYSTEM_TRACE_HEADER` if defined, else `system_trace.h`, which defines `L_ERR`
/ `L_WARNING` / `L_WARNING_ONCE` / `L_INFO` / `L_CALL` / `L_DEBUG` as no-ops. The
error-string helpers (`error::name` / `description`) are referenced **only inside** the
`L_ERR` arguments, so a no-op expansion never evaluates them -- which is why the
standalone build needs no errno-names dependency.

## Invariants

- **Verbatim behavior.** The probes are byte-for-byte from Xapiand's in-tree
  `system.*` / `memory_stats.*`. Do not change what a function measures or its units
  (bytes, counts).
- **Reads go through `io::`** (the `/proc` paths), keeping EINTR handling consistent.
- **Portable feature detection, no `config.h`.** Add a capability with `__has_include`
  / a platform macro, not a generated define.
