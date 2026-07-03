/*
 * Default (no-op) logging hooks for the standalone `system` library.
 *
 * system.cc and memory_stats.cc instrument their probes through a small L_* family
 * that is no-op by default, so the library builds with zero dependency on any logging
 * or error-string header. The functions' return values do not depend on them: they
 * only emit diagnostics (a failed sysctl / proc read) a host may want to surface.
 *
 * The macros used:
 *
 *   - L_ERR(...)          — a failed system probe, with errno name/description in args.
 *   - L_WARNING(...)      — a recoverable problem.
 *   - L_WARNING_ONCE(...) — like L_WARNING but a host may de-duplicate it.
 *   - L_INFO(...)         — an informational notice.
 *   - L_CALL(...)         — entry trace.
 *   - L_DEBUG(...)        — low-severity diagnostic.
 *   - L_NOTHING(...)      — explicit no-op (the default target for the others).
 *
 * Because error::name(errno) / error::description(errno) appear ONLY inside these
 * macros' arguments, a no-op expansion drops them unevaluated -- so the standalone
 * library needs no errno-names dependency. A host that wants real logging provides its
 * own versions, two ways:
 *
 *   1. Point SYSTEM_TRACE_HEADER at a header that defines them (responsible for pulling
 *      in whatever error:: it references), e.g.
 *        c++ -DSYSTEM_TRACE_HEADER='"my_system_trace.h"' ...
 *
 *   2. Define the macros directly before including system.hh / memory_stats.h.
 *
 * Each macro is `#ifndef`-guarded, so defining any subset is fine.
 */

#pragma once

#ifndef L_NOTHING
#define L_NOTHING(...)
#endif

#ifndef L_CALL
#define L_CALL L_NOTHING
#endif

#ifndef L_DEBUG
#define L_DEBUG L_NOTHING
#endif

#ifndef L_ERR
#define L_ERR L_NOTHING
#endif

#ifndef L_INFO
#define L_INFO L_NOTHING
#endif

#ifndef L_WARNING
#define L_WARNING L_NOTHING
#endif

#ifndef L_WARNING_ONCE
#define L_WARNING_ONCE L_NOTHING
#endif
