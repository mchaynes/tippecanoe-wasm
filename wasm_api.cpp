#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <vector>
#include <cstring>
#include "pmtiles_direct.hpp"

// Forward declaration of main
extern int main(int argc, char **argv);

// Storage for output PMTiles data
static std::string g_output_buffer;
static bool g_output_ready = false;

// Progress callback (called from JavaScript)
static emscripten::val g_progress_callback = emscripten::val::null();

extern "C" {

// Run tippecanoe with command-line arguments
// Returns 0 on success, non-zero on failure
EMSCRIPTEN_KEEPALIVE
int tippecanoe_run(int argc, char **argv) {
	g_output_ready = false;
	g_output_buffer.clear();

	// Initialize the direct PMTiles writer
	init_direct_pmtiles_writer();

	// Call the main function
	int result = main(argc, argv);

	return result;
}

// Get the size of the output PMTiles data
EMSCRIPTEN_KEEPALIVE
size_t tippecanoe_get_output_size() {
	return g_output_buffer.size();
}

// Get a pointer to the output PMTiles data
EMSCRIPTEN_KEEPALIVE
const char *tippecanoe_get_output() {
	return g_output_buffer.data();
}

// Free the output buffer
EMSCRIPTEN_KEEPALIVE
void tippecanoe_free_output() {
	g_output_buffer.clear();
	g_output_buffer.shrink_to_fit();
	cleanup_direct_pmtiles_writer();
}

// Report progress (can be called from various points in the code)
EMSCRIPTEN_KEEPALIVE
void tippecanoe_report_progress(const char *phase, int percent, const char *message) {
	if (!g_progress_callback.isNull()) {
		g_progress_callback(std::string(phase), percent, std::string(message));
	}
}

}  // extern "C"

// Set the output buffer (called from C++ code when finalize completes)
// This needs C++ linkage because it uses std::string
void tippecanoe_set_output(const std::string &data) {
	g_output_buffer = data;
	g_output_ready = true;
}

// Embind bindings for a cleaner JavaScript API
namespace {

// Wrapper class for JavaScript API
class TippecanoeWasm {
public:
	TippecanoeWasm() {}

	// Run with arguments (CLI-like interface)
	int run(const std::vector<std::string> &args) {
		// Build argc/argv from the vector
		std::vector<char *> argv;
		argv.push_back(const_cast<char *>("tippecanoe"));

		for (const auto &arg : args) {
			argv.push_back(const_cast<char *>(arg.c_str()));
		}
		argv.push_back(nullptr);

		return tippecanoe_run(argv.size() - 1, argv.data());
	}

	// Run with newline-separated arguments string (easier for JS interop)
	int runArgs(const std::string &argsStr) {
		std::vector<std::string> args;
		std::string current;

		for (char c : argsStr) {
			if (c == '\n') {
				if (!current.empty()) {
					args.push_back(current);
					current.clear();
				}
			} else {
				current += c;
			}
		}
		if (!current.empty()) {
			args.push_back(current);
		}

		return run(args);
	}

	// Get the output as a JavaScript typed array
	emscripten::val getOutput() {
		if (g_output_buffer.empty()) {
			return emscripten::val::null();
		}

		// Create a Uint8Array view of the output
		return emscripten::val(
		    emscripten::typed_memory_view(g_output_buffer.size(),
		                                  reinterpret_cast<const uint8_t *>(g_output_buffer.data())));
	}

	// Copy output to a new Uint8Array (safer, data is owned by JS)
	emscripten::val copyOutput() {
		if (g_output_buffer.empty()) {
			return emscripten::val::null();
		}

		// Create a new Uint8Array and copy data
		emscripten::val uint8Array = emscripten::val::global("Uint8Array").new_(g_output_buffer.size());
		emscripten::val heap = emscripten::val::module_property("HEAPU8");
		emscripten::val memory_view = heap["buffer"];

		// Use set() to copy the data
		uint8Array.call<void>("set",
		                      emscripten::typed_memory_view(g_output_buffer.size(),
		                                                    reinterpret_cast<const uint8_t *>(g_output_buffer.data())));

		return uint8Array;
	}

	size_t getOutputSize() {
		return g_output_buffer.size();
	}

	void freeOutput() {
		tippecanoe_free_output();
	}

	void setProgressCallback(emscripten::val callback) {
		g_progress_callback = callback;
	}
};

}  // namespace

EMSCRIPTEN_BINDINGS(tippecanoe_wasm) {
	emscripten::class_<TippecanoeWasm>("Tippecanoe")
	    .constructor<>()
	    .function("run", &TippecanoeWasm::run)
	    .function("runArgs", &TippecanoeWasm::runArgs)
	    .function("getOutput", &TippecanoeWasm::getOutput)
	    .function("copyOutput", &TippecanoeWasm::copyOutput)
	    .function("getOutputSize", &TippecanoeWasm::getOutputSize)
	    .function("freeOutput", &TippecanoeWasm::freeOutput)
	    .function("setProgressCallback", &TippecanoeWasm::setProgressCallback);

	emscripten::register_vector<std::string>("VectorString");
}

#endif  // __EMSCRIPTEN__
