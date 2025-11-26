#ifndef PLATFORM_WASM_HPP
#define PLATFORM_WASM_HPP

#ifdef __EMSCRIPTEN__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// WASM doesn't have real mmap, so we emulate it with malloc+read
// This is used by tippecanoe for reading large files

inline void *wasm_mmap_read(int fd, size_t length) {
	void *ptr = malloc(length);
	if (ptr == nullptr) {
		return (void *)-1;  // MAP_FAILED equivalent
	}

	// Save current position and seek to beginning
	off_t saved_pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);

	// Read the entire file into memory
	size_t total_read = 0;
	while (total_read < length) {
		ssize_t n = read(fd, (char *)ptr + total_read, length - total_read);
		if (n <= 0) {
			break;
		}
		total_read += n;
	}

	// Restore position
	lseek(fd, saved_pos, SEEK_SET);

	if (total_read != length) {
		free(ptr);
		return (void *)-1;
	}

	return ptr;
}

inline int wasm_munmap(void *addr, size_t length) {
	(void)length;  // unused
	free(addr);
	return 0;
}

// madvise is a no-op in WASM
inline int wasm_madvise(void *addr, size_t length, int advice) {
	(void)addr;
	(void)length;
	(void)advice;
	return 0;
}

// Helper to get file size
inline size_t wasm_get_file_size(int fd) {
	struct stat st;
	if (fstat(fd, &st) != 0) {
		return 0;
	}
	return st.st_size;
}

// WASM temp directory - uses Emscripten's virtual filesystem
// This can be mounted to IDBFS for persistence
#define WASM_TEMP_DIR "/tmp"

// Check if we're in the WASM environment
inline bool is_wasm_environment() {
	return true;
}

#else  // Not WASM

#include <sys/mman.h>

inline void *wasm_mmap_read(int fd, size_t length) {
	return mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
}

inline int wasm_munmap(void *addr, size_t length) {
	return munmap(addr, length);
}

inline int wasm_madvise(void *addr, size_t length, int advice) {
	return madvise(addr, length, advice);
}

inline bool is_wasm_environment() {
	return false;
}

#endif  // __EMSCRIPTEN__

#endif  // PLATFORM_WASM_HPP
