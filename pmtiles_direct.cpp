#include "pmtiles_direct.hpp"
#include "mvt.hpp"
#include "write_json.hpp"
#include <algorithm>
#include <functional>

// Simple hash function for tile content (FNV-1a)
static uint64_t fnv1a_hash(const std::string &data) {
	uint64_t hash = 14695981039346656037ULL;
	for (char c : data) {
		hash ^= static_cast<uint8_t>(c);
		hash *= 1099511628211ULL;
	}
	return hash;
}

DirectPMTilesWriter::DirectPMTilesWriter() {
	// Reserve some initial capacity
	entries.reserve(10000);
	tile_buffer.reserve(1024 * 1024);  // 1MB initial
}

DirectPMTilesWriter::~DirectPMTilesWriter() {
}

std::string DirectPMTilesWriter::hash_tile(const std::string &data) {
	// Use a simple hash for deduplication
	uint64_t h = fnv1a_hash(data);
	return std::to_string(h);
}

std::string DirectPMTilesWriter::compress_directory(const std::string &data, uint8_t compression) {
	if (compression == pmtiles::COMPRESSION_NONE) {
		return data;
	} else if (compression == pmtiles::COMPRESSION_GZIP) {
		std::string output;
		compress(data, output, true);
		return output;
	}
	return data;
}

void DirectPMTilesWriter::add_tile(int z, int x, int y, const std::string &compressed_data) {
	std::lock_guard<std::mutex> lock(writer_mutex);

	uint64_t tile_id = pmtiles::zxy_to_tileid(z, x, y);
	std::string hash = hash_tile(compressed_data);

	addressed_tiles_count++;

	// Check if we've already seen this tile content
	auto it = hash_to_offset_len.find(hash);
	if (it != hash_to_offset_len.end()) {
		// Duplicate content - reuse existing offset
		uint64_t existing_offset = it->second.first;
		uint32_t existing_length = it->second.second;

		// Check for run-length optimization (consecutive tile IDs with same content)
		if (!entries.empty() &&
		    tile_id == entries.back().tile_id + 1 &&
		    entries.back().offset == existing_offset) {
			// Extend the run
			entries.back().run_length++;
		} else {
			// Create new entry pointing to existing data
			entries.emplace_back(tile_id, existing_offset, existing_length, 1);
		}
	} else {
		// New content - append to buffer
		uint64_t offset = tile_buffer.size();
		uint32_t length = compressed_data.size();

		tile_buffer.append(compressed_data);

		// Record for deduplication
		hash_to_offset_len[hash] = std::make_pair(offset, length);
		tile_contents_count++;

		// Check for run-length optimization with previous entry
		if (!entries.empty() &&
		    tile_id == entries.back().tile_id + 1 &&
		    entries.back().offset + entries.back().length == offset &&
		    entries.back().length == length) {
			// This could be a run, but only if content is identical
			// Since this is new content, we need a new entry
			entries.emplace_back(tile_id, offset, length, 1);
		} else {
			entries.emplace_back(tile_id, offset, length, 1);
		}
	}
}

// Helper to generate metadata JSON (similar to pmtiles_file.cpp)
static std::string metadata_to_pmtiles_json(const metadata &m) {
	std::string buf;
	json_writer state(&buf);

	state.json_write_hash();
	state.json_write_newline();

	auto out = [&state](const std::string &k, const std::string &v) {
		state.json_comma_newline();
		state.json_write_string(k);
		state.json_write_string(v);
	};

	out("name", m.name);
	out("format", m.format);
	out("type", m.type);
	out("description", m.description);
	out("version", std::to_string(m.version));
	if (m.attribution.size() > 0) {
		out("attribution", m.attribution);
	}

	if (m.vector_layers_json.size() > 0) {
		state.json_comma_newline();
		state.json_write_string("vector_layers");
		state.json_write_json(m.vector_layers_json);
	}

	if (m.tilestats_json.size() > 0) {
		state.json_comma_newline();
		state.json_write_string("tilestats");
		state.json_write_json(m.tilestats_json);
	}

	state.json_write_newline();
	state.json_end_hash();
	state.json_write_newline();

	// Compress the metadata
	std::string compressed;
	compress(buf, compressed, true);
	return compressed;
}

std::string DirectPMTilesWriter::finalize(const metadata &m, bool tile_compression) {
	std::lock_guard<std::mutex> lock(writer_mutex);

	if (entries.empty()) {
		// Create an empty PMTiles file
		pmtiles::headerv3 header;
		header.root_dir_offset = 127;
		header.root_dir_bytes = 0;
		header.json_metadata_offset = 127;
		header.json_metadata_bytes = 0;
		header.leaf_dirs_offset = 127;
		header.leaf_dirs_bytes = 0;
		header.tile_data_offset = 127;
		header.tile_data_bytes = 0;
		header.addressed_tiles_count = 0;
		header.tile_entries_count = 0;
		header.tile_contents_count = 0;
		header.clustered = true;
		header.internal_compression = pmtiles::COMPRESSION_GZIP;
		header.tile_compression = tile_compression ? pmtiles::COMPRESSION_GZIP : pmtiles::COMPRESSION_NONE;
		header.tile_type = pmtiles::TILETYPE_MVT;
		header.min_zoom = 0;
		header.max_zoom = 0;
		header.min_lon_e7 = -180 * 10000000;
		header.min_lat_e7 = -90 * 10000000;
		header.max_lon_e7 = 180 * 10000000;
		header.max_lat_e7 = 90 * 10000000;
		header.center_zoom = 0;
		header.center_lon_e7 = 0;
		header.center_lat_e7 = 0;
		return header.serialize();
	}

	// Sort entries by tile_id
	std::stable_sort(entries.begin(), entries.end(), pmtiles::entryv3_cmp());

	// Build directory structure
	// Note: Entry offsets are relative to the tile data section, not absolute file offsets.
	// The header.tile_data_offset tells readers where the tile data section starts.
	auto compress_func = [this](const std::string &data, uint8_t compression) {
		return this->compress_directory(data, compression);
	};

	std::string root_bytes;
	std::string leaves_bytes;
	int num_leaves;
	std::tie(root_bytes, leaves_bytes, num_leaves) = pmtiles::make_root_leaves(
	    compress_func, pmtiles::COMPRESSION_GZIP, entries);

	// Generate metadata JSON
	std::string json_metadata = metadata_to_pmtiles_json(m);

	// Build header
	pmtiles::headerv3 header;

	header.root_dir_offset = 127;
	header.root_dir_bytes = root_bytes.size();

	header.json_metadata_offset = header.root_dir_offset + header.root_dir_bytes;
	header.json_metadata_bytes = json_metadata.size();

	header.leaf_dirs_offset = header.json_metadata_offset + header.json_metadata_bytes;
	header.leaf_dirs_bytes = leaves_bytes.size();

	header.tile_data_offset = header.leaf_dirs_offset + header.leaf_dirs_bytes;
	header.tile_data_bytes = tile_buffer.size();

	header.addressed_tiles_count = addressed_tiles_count;
	header.tile_entries_count = entries.size();
	header.tile_contents_count = tile_contents_count;

	header.clustered = true;
	header.internal_compression = pmtiles::COMPRESSION_GZIP;
	header.tile_compression = tile_compression ? pmtiles::COMPRESSION_GZIP : pmtiles::COMPRESSION_NONE;

	if (m.format == "pbf") {
		header.tile_type = pmtiles::TILETYPE_MVT;
	} else if (m.format == "png") {
		header.tile_type = pmtiles::TILETYPE_PNG;
	} else if (m.format == "jpg" || m.format == "jpeg") {
		header.tile_type = pmtiles::TILETYPE_JPEG;
	} else if (m.format == "webp") {
		header.tile_type = pmtiles::TILETYPE_WEBP;
	} else {
		header.tile_type = pmtiles::TILETYPE_MVT;  // Default to MVT
	}

	header.min_zoom = m.minzoom;
	header.max_zoom = m.maxzoom;
	header.min_lon_e7 = m.minlon * 10000000;
	header.min_lat_e7 = m.minlat * 10000000;
	header.max_lon_e7 = m.maxlon * 10000000;
	header.max_lat_e7 = m.maxlat * 10000000;
	header.center_zoom = m.center_z;
	header.center_lon_e7 = m.center_lon * 10000000;
	header.center_lat_e7 = m.center_lat * 10000000;

	// Assemble the complete PMTiles file
	std::string result;
	result.reserve(127 + root_bytes.size() + json_metadata.size() + leaves_bytes.size() + tile_buffer.size());

	result.append(header.serialize());
	result.append(root_bytes);
	result.append(json_metadata);
	result.append(leaves_bytes);
	result.append(tile_buffer);

	return result;
}

#ifdef __EMSCRIPTEN__

// Global instance for WASM builds
DirectPMTilesWriter *g_pmtiles_writer = nullptr;

void init_direct_pmtiles_writer() {
	if (g_pmtiles_writer != nullptr) {
		delete g_pmtiles_writer;
	}
	g_pmtiles_writer = new DirectPMTilesWriter();
}

std::string finalize_direct_pmtiles(const metadata &m, bool tile_compression) {
	if (g_pmtiles_writer == nullptr) {
		return "";
	}
	return g_pmtiles_writer->finalize(m, tile_compression);
}

void cleanup_direct_pmtiles_writer() {
	if (g_pmtiles_writer != nullptr) {
		delete g_pmtiles_writer;
		g_pmtiles_writer = nullptr;
	}
}

#endif  // __EMSCRIPTEN__
