# Architecture

`system` is a flat set of free functions, each of which reads one piece of OS/process
resource state. No state, no caching -- every call re-queries the machine. The only
subtlety is per-platform source selection.

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
