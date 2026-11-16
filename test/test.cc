// ctest for system: the probes query the real machine, so we assert plausible,
// self-consistent results (positive totals, free <= total, non-empty strings) rather
// than exact values. Built with default (no-op) logging, proving the io/strings deps
// are the only ones.

#include "system.hh"
#include "memory_stats.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

static int failures = 0;
#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                          \
		}                                                                        \
	} while (0)

// The oracle: probe every descriptor with fcntl, up to a cap far above what the test opens.
static std::size_t scan_open_files() {
	std::size_t cnt = 0;
	const std::size_t top = std::min<std::size_t>(get_max_files_per_proc(), 4096);
	for (std::size_t fd = 0; fd < top; ++fd) {
		if (::fcntl(static_cast<int>(fd), F_GETFD) != -1) {
			++cnt;
		}
	}
	return cnt;
}

static void test_open_files_count() {
	constexpr std::size_t N = 17;
	const std::size_t base = get_open_files_per_proc();
	CHECK(base == scan_open_files());                            // the listing agrees with the poll-free oracle

	std::vector<int> fds;
	for (std::size_t i = 0; i < N; ++i) {
		fds.push_back(::open("/dev/null", O_RDONLY));
		CHECK(fds.back() >= 0);
		CHECK(get_open_files_per_proc() == base + i + 1);        // exactly one more per open
	}
	CHECK(get_open_files_per_proc() == scan_open_files());

	for (std::size_t i = 0; i < N; ++i) {
		::close(fds[i]);
		CHECK(get_open_files_per_proc() == base + N - i - 1);    // and exactly one less per close
	}
	CHECK(get_open_files_per_proc() == base);

	// The highest descriptor counts too, even far above the rest.
	if (get_max_files_per_proc() > 2048) {
		const int high = ::dup2(0, 2000);
		CHECK(high == 2000);
		CHECK(get_open_files_per_proc() == base + 1);
		::close(high);
		CHECK(get_open_files_per_proc() == base);
	}
}

int main() {
	// --- system.hh: file-descriptor + platform introspection ---
	CHECK(get_max_files_per_proc() > 0);
	CHECK(get_open_files_per_proc() >= 1);                       // at least stdin/out/err
	CHECK(get_open_files_per_proc() <= get_max_files_per_proc());
	test_open_files_count();
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
