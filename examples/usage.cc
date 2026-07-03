/*
 * usage: a runnable, self-checking example for the `system` library, shaped like
 * Xapiand's resource reporting path. Xapiand samples these probes for /:metrics
 * gauges and startup/resource-limit checks, so this example asserts the stable
 * invariants instead of environment-specific values.
 */

#include "memory_stats.h"
#include "system.hh"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

namespace {

constexpr uint64_t MiB = 1024 * 1024;

std::size_t page_size()
{
#ifdef _SC_PAGESIZE
	long size = ::sysconf(_SC_PAGESIZE);
	if (size > 0) {
		return static_cast<std::size_t>(size);
	}
#endif
	return 4096;
}

uint64_t choose_probe_size(uint64_t total_ram)
{
	uint64_t size = 64 * MiB;
	if (total_ram > 0) {
		size = std::min<uint64_t>(size, std::max<uint64_t>(8 * MiB, total_ram / 128));
	}
	return size;
}

uint64_t touch_pages(std::vector<unsigned char>& allocation)
{
	uint64_t checksum = 0;
	const std::size_t stride = page_size();
	for (std::size_t pos = 0; pos < allocation.size(); pos += stride) {
		allocation[pos] = static_cast<unsigned char>(((pos / stride) % 251) + 1);
		checksum += allocation[pos];
	}
	allocation.back() = 0xa5;
	checksum += allocation.back();
	return checksum;
}

uint64_t wait_for_rss_growth(uint64_t before)
{
	uint64_t after = get_current_memory_by_process();
	for (int attempt = 0; attempt < 10 && after <= before; ++attempt) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		after = get_current_memory_by_process();
	}
	return after;
}

}  // namespace

int main(int argc, char** argv)
{
	// Xapiand startup/resource-limit checks use these fd probes to decide whether
	// the process and host have enough room for clients, shards, and databases.
	const std::size_t max_files_per_proc = get_max_files_per_proc();
	const std::size_t max_files_per_proc_again = get_max_files_per_proc();
	CHECK(max_files_per_proc > 0);
	CHECK(max_files_per_proc_again == max_files_per_proc);

	const std::size_t open_files_per_proc = get_open_files_per_proc();
	CHECK(open_files_per_proc >= 1);
	CHECK(open_files_per_proc <= max_files_per_proc);

	if (argc > 0 && argv[0] != nullptr) {
		std::ifstream held_file(argv[0], std::ios::binary);
		CHECK(held_file.good());
		const std::size_t open_with_file = get_open_files_per_proc();
		CHECK(open_with_file > open_files_per_proc);
		CHECK(open_with_file <= max_files_per_proc);
		held_file.close();
		CHECK(get_open_files_per_proc() <= open_with_file);
	}

	const std::size_t max_files_system_wide = get_max_files_system_wide();
	const std::size_t max_files_system_wide_again = get_max_files_system_wide();
	CHECK(max_files_system_wide > 0);
	CHECK(max_files_system_wide_again == max_files_system_wide);

	const std::size_t open_files_system_wide = get_open_files_system_wide();
	CHECK(open_files_system_wide <= max_files_system_wide);

	// Xapiand exposes these strings as constant labels on build/runtime metrics.
	const std::string compiler = check_compiler();
	const std::string os = check_OS();
	const std::string architecture = check_architecture();
	CHECK(!compiler.empty());
	CHECK(!os.empty());
	CHECK(!architecture.empty());
	CHECK(check_compiler() == compiler);
	CHECK(check_OS() == os);
	CHECK(check_architecture() == architecture);

	// Xapiand uses the core count to size worker pools. The portable floor is one.
	const unsigned int cores = std::thread::hardware_concurrency();
	CHECK(cores >= 1);
#ifdef _SC_NPROCESSORS_ONLN
	CHECK(::sysconf(_SC_NPROCESSORS_ONLN) >= 1);
#endif

	// Xapiand's /:metrics path samples process RSS/virtual memory and host totals.
	const uint64_t total_ram = get_total_ram();
	CHECK(total_ram > 0);
	CHECK(get_total_ram() == total_ram);

	const uint64_t rss_before = get_current_memory_by_process();
	const uint64_t virtual_before = get_current_memory_by_process(false);
	CHECK(rss_before > 0);
	CHECK(rss_before <= total_ram);
	CHECK(virtual_before >= rss_before);

#ifdef __APPLE__
	const auto [used_ram, free_ram] = get_current_ram();
	CHECK(used_ram > 0);
	CHECK(free_ram >= 0);
	CHECK(static_cast<uint64_t>(used_ram) <= total_ram);
	CHECK(static_cast<uint64_t>(free_ram) <= total_ram);
#endif

	const uint64_t virtual_total = get_total_virtual_memory();
	CHECK(virtual_total == 0 || get_total_virtual_memory() > 0);

	const uint64_t total_disk = get_total_disk_size();
	const uint64_t free_disk = get_free_disk_size();
	CHECK(total_disk > 0);
	CHECK(free_disk <= total_disk);

	const uint64_t total_inodes = get_total_inodes();
	const uint64_t free_inodes = get_free_inodes();
	CHECK(free_inodes <= total_inodes);

	std::vector<unsigned char> allocation(static_cast<std::size_t>(choose_probe_size(total_ram)));
	CHECK(touch_pages(allocation) > 0);
	const uint64_t rss_after = wait_for_rss_growth(rss_before);
	CHECK(allocation.front() != 0);
	CHECK(rss_after > rss_before);
	CHECK(rss_after <= total_ram);
	CHECK(get_current_memory_by_process(false) >= rss_after);

	if (failures == 0) {
		std::puts("system usage: all checks passed (fd limits + metrics gauges + memory/RSS growth + platform labels)");
	}
	return failures == 0 ? 0 : 1;
}
