#ifndef PMTILES_DIRECT_HPP
#define PMTILES_DIRECT_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include "pmtiles/pmtiles.hpp"
#include "mbtiles.hpp"

// Direct PMTiles writer that bypasses SQLite
// Used primarily for WASM builds but also available for native builds

class DirectPMTilesWriter {
public:
	DirectPMTilesWriter();
	~DirectPMTilesWriter();

	// Add a tile (thread-safe)
	// Returns true if the tile was added (new content), false if it was a duplicate
	void add_tile(int z, int x, int y, const std::string &compressed_data);

	// Finalize and write the complete PMTiles archive
	// Returns the complete PMTiles data as a string
	std::string finalize(const metadata &m, bool tile_compression);

	// Get statistics
	size_t get_tile_count() const { return addressed_tiles_count; }
	size_t get_unique_tile_count() const { return tile_contents_count; }

private:
	// Hash function for tile content deduplication
	std::string hash_tile(const std::string &data);

	// Compress function for directories
	std::string compress_directory(const std::string &data, uint8_t compression);

	// Tile storage: hash -> (offset_in_buffer, length)
	std::unordered_map<std::string, std::pair<uint64_t, uint32_t>> hash_to_offset_len;

	// Directory entries
	std::vector<pmtiles::entryv3> entries;

	// Tile data buffer
	std::string tile_buffer;

	// Statistics
	size_t addressed_tiles_count = 0;
	size_t tile_contents_count = 0;

	// Thread safety
	std::mutex writer_mutex;
};

// Global instance for WASM builds
#ifdef __EMSCRIPTEN__
extern DirectPMTilesWriter *g_pmtiles_writer;

// Initialize the global writer
void init_direct_pmtiles_writer();

// Finalize and get output
std::string finalize_direct_pmtiles(const metadata &m, bool tile_compression);

// Clean up
void cleanup_direct_pmtiles_writer();
#endif

#endif  // PMTILES_DIRECT_HPP
