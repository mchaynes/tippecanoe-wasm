#include "platform.hpp"
#include <cstdlib>
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/threading.h>
#endif
#else
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/mount.h>
#endif
#endif

#include "errors.hpp"

#ifdef __EMSCRIPTEN__

// WASM-specific implementations

long get_num_avail_cpus() {
#ifdef __EMSCRIPTEN_PTHREADS__
	// When compiled with pthread support, query available cores
	return emscripten_num_logical_cores();
#else
	// Single-threaded build
	return 1;
#endif
}

long get_page_size() {
	// WebAssembly uses 64KB pages
	return 65536;
}

// Configurable memory limit (can be set from JavaScript)
static size_t wasm_memory_limit = 2ULL * 1024 * 1024 * 1024;  // 2GB default

EM_JS(size_t, get_js_memory_limit, (), {
	// Allow JavaScript to configure memory limit via Module
	if (typeof Module !== 'undefined' && Module.TIPPECANOE_MAX_MEMORY) {
		return Module.TIPPECANOE_MAX_MEMORY;
	}
	// Default to 2GB
	return 2 * 1024 * 1024 * 1024;
});

size_t calc_memsize() {
	return get_js_memory_limit();
}

size_t get_max_open_files() {
	// Emscripten's virtual filesystem doesn't have the same limits
	// Return a reasonable number for the virtual FS
	return 1024;
}

#else

// Native implementations

long get_num_avail_cpus() {
	return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_page_size() {
	return sysconf(_SC_PAGESIZE);
}

size_t calc_memsize() {
	size_t mem;

#ifdef __APPLE__
	int64_t hw_memsize;
	size_t len = sizeof(int64_t);
	if (sysctlbyname("hw.memsize", &hw_memsize, &len, NULL, 0) < 0) {
		perror("sysctl hw.memsize");
		exit(EXIT_MEMORY);
	}
	mem = hw_memsize;
#else
	long long pagesize = sysconf(_SC_PAGESIZE);
	long long pages = sysconf(_SC_PHYS_PAGES);
	if (pages < 0 || pagesize < 0) {
		perror("sysconf _SC_PAGESIZE or _SC_PHYS_PAGES");
		exit(EXIT_MEMORY);
	}

	mem = (long long) pages * pagesize;
#endif

	return mem;
}

size_t get_max_open_files() {
	struct rlimit rl;
	if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
		perror("getrlimit");
		exit(EXIT_PTHREAD);
	}
	return rl.rlim_cur;
}

#endif  // __EMSCRIPTEN__