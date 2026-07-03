// ctest for system: the probes query the real machine, so we assert plausible,
// self-consistent results (positive totals, free <= total, non-empty strings) rather
// than exact values. Built with default (no-op) logging, proving the io/strings deps
// are the only ones.

#include "system.hh"
#include "memory_stats.h"

#include <cstdio>
#include <cstdlib>

static int failures = 0;
#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	// --- system.hh: file-descriptor + platform introspection ---
	CHECK(get_max_files_per_proc() > 0);
	CHECK(get_open_files_per_proc() >= 1);                       // at least stdin/out/err
	CHECK(get_open_files_per_proc() <= get_max_files_per_proc());
	CHECK(get_max_files_system_wide() > 0);
	CHECK(!check_compiler().empty());
	CHECK(!check_OS().empty());
	CHECK(!check_architecture().empty());

	// --- memory_stats.h: memory + disk introspection ---
	CHECK(get_total_ram() > 0);
	CHECK(get_current_memory_by_process() > 0);
	CHECK(get_total_virtual_memory() > 0);

	uint64_t total_disk = get_total_disk_size();
	uint64_t free_disk = get_free_disk_size();
	CHECK(total_disk > 0);
	CHECK(free_disk <= total_disk);                              // free never exceeds total

	uint64_t total_inodes = get_total_inodes();
	uint64_t free_inodes = get_free_inodes();
	CHECK(free_inodes <= total_inodes);

	if (failures != 0) {
		std::fprintf(stderr, "%d check(s) failed\n", failures);
		return EXIT_FAILURE;
	}
	std::puts("all system tests passed");
	return EXIT_SUCCESS;
}
