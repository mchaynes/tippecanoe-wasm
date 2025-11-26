#!/bin/bash
#
# Build tippecanoe for WebAssembly
#
# Prerequisites:
#   - Emscripten SDK installed and activated
#   - Run: source /path/to/emsdk/emsdk_env.sh
#
# Usage:
#   ./build-wasm.sh          # Build both multi-threaded and single-threaded
#   ./build-wasm.sh multi    # Build only multi-threaded
#   ./build-wasm.sh single   # Build only single-threaded
#   ./build-wasm.sh clean    # Clean build artifacts

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if emscripten is available
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}Error: emcc not found. Please activate the Emscripten SDK first.${NC}"
    echo ""
    echo "Example:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
    echo ""
    exit 1
fi

echo -e "${GREEN}Emscripten version:${NC}"
emcc --version | head -n1

# Parse arguments
BUILD_TARGET="${1:-all}"

case "$BUILD_TARGET" in
    multi)
        echo -e "${YELLOW}Building multi-threaded version...${NC}"
        emmake make -f Makefile.emscripten multi
        ;;
    single)
        echo -e "${YELLOW}Building single-threaded version...${NC}"
        emmake make -f Makefile.emscripten single
        ;;
    all)
        echo -e "${YELLOW}Building both versions...${NC}"
        emmake make -f Makefile.emscripten all
        ;;
    clean)
        echo -e "${YELLOW}Cleaning build artifacts...${NC}"
        emmake make -f Makefile.emscripten clean
        exit 0
        ;;
    *)
        echo "Usage: $0 [multi|single|all|clean]"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Build complete!${NC}"
echo ""
echo "Output files in wasm/dist/:"
ls -la wasm/dist/ 2>/dev/null || echo "(no files yet)"
echo ""
echo "To use in a browser with multi-threading, serve with these headers:"
echo "  Cross-Origin-Opener-Policy: same-origin"
echo "  Cross-Origin-Embedder-Policy: require-corp"
