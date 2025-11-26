#include <stdlib.h>
#include <cstdint>
#include "thread.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/threading.h>
#endif
#endif

// It is harder to profile tippecanoe because of its normal use of multiple threads.
// If you set TIPPECANOE_NO_THREADS in the environment, it will run everything in
// the main thread instead for more straightforward profiling. (The threads will
// then still be created so they will be reaped, but they will immediately return.)

static const char *no_threads = getenv("TIPPECANOE_NO_THREADS");

static void *do_nothing(void *arg) {
	return arg;
}

#ifdef __EMSCRIPTEN__

// Check if threading is available at runtime in WASM
// This handles the case where SharedArrayBuffer is not available
static bool wasm_threading_available() {
#ifdef __EMSCRIPTEN_PTHREADS__
	// Check if SharedArrayBuffer is available (requires COOP/COEP headers)
	return emscripten_has_threading_support();
#else
	// Compiled without pthread support
	return false;
#endif
}

int thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
	// In WASM, check runtime threading availability
	if (no_threads != NULL || !wasm_threading_available()) {
		// Run synchronously in main thread
		void *ret = start_routine(arg);
#ifdef __EMSCRIPTEN_PTHREADS__
		// Still need to create a dummy thread for the join interface
		return pthread_create(thread, attr, do_nothing, ret);
#else
		// No pthread support at all - store result in a fake thread handle
		// The join will just return this value
		*thread = (pthread_t)(uintptr_t)ret;
		return 0;
#endif
	} else {
		return pthread_create(thread, attr, start_routine, arg);
	}
}

#else  // Native (non-WASM)

int thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
	if (no_threads != NULL) {
		void *ret = start_routine(arg);
		return pthread_create(thread, attr, do_nothing, ret);
	} else {
		return pthread_create(thread, attr, start_routine, arg);
	}
}

#endif  // __EMSCRIPTEN__
