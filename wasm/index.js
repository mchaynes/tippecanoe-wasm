/**
 * Tippecanoe WASM - Generate PMTiles in the browser
 *
 * Usage:
 *   import { createTippecanoe } from 'tippecanoe-wasm';
 *
 *   const tippecanoe = await createTippecanoe();
 *   const result = await tippecanoe.run(
 *     ['-o', 'output.pmtiles', '-z14', 'input.geojson'],
 *     new Map([['input.geojson', geojsonData]])
 *   );
 */

// Detect if we have SharedArrayBuffer support (needed for multi-threading)
function hasSharedArrayBuffer() {
    try {
        new SharedArrayBuffer(1);
        return true;
    } catch (e) {
        return false;
    }
}

// Load the appropriate WASM module
async function loadModule(options = {}) {
    const useThreads = options.threads !== false && hasSharedArrayBuffer();

    // Dynamic import based on threading support
    // Cache bust version - increment when rebuilding WASM
    const v = 6;
    let TippecanoeModule;
    if (useThreads) {
        TippecanoeModule = (await import(`./dist/tippecanoe.js?v=${v}`)).default;
    } else {
        TippecanoeModule = (await import(`./dist/tippecanoe-st.js?v=${v}`)).default;
    }

    const moduleOptions = {
        ...options,
        TIPPECANOE_MAX_MEMORY: options.maxMemory || 2 * 1024 * 1024 * 1024,
    };

    return await TippecanoeModule(moduleOptions);
}

/**
 * Create a Tippecanoe instance
 * @param {Object} options - Configuration options
 * @param {boolean} [options.threads=true] - Enable multi-threading (requires SharedArrayBuffer)
 * @param {number} [options.maxMemory] - Maximum memory in bytes (default: 2GB)
 * @param {Function} [options.onStdout] - Callback for stdout messages
 * @param {Function} [options.onStderr] - Callback for stderr messages
 * @returns {Promise<TippecanoeInstance>}
 */
export async function createTippecanoe(options = {}) {
    const Module = await loadModule(options);

    return new TippecanoeInstance(Module, options);
}

class TippecanoeInstance {
    constructor(Module, options) {
        this.Module = Module;
        this.options = options;
        this._tippecanoe = new Module.Tippecanoe();
    }

    /**
     * Run tippecanoe with CLI-like arguments
     * @param {string[]} args - Command line arguments (e.g., ['-o', 'out.pmtiles', 'input.geojson'])
     * @param {Map<string, Uint8Array|string>} [files] - Input files as a Map of filename -> data
     * @param {Object} [runOptions] - Run-time options
     * @param {Function} [runOptions.onProgress] - Progress callback
     * @returns {Promise<TippecanoeResult>}
     */
    async run(args, files = new Map(), runOptions = {}) {
        const { FS } = this.Module;

        // Write input files to virtual filesystem
        for (const [filename, data] of files) {
            const dirname = filename.includes('/') ? filename.substring(0, filename.lastIndexOf('/')) : '';
            if (dirname) {
                this._mkdirp(dirname);
            }

            if (typeof data === 'string') {
                FS.writeFile(filename, data);
            } else if (data instanceof Uint8Array) {
                FS.writeFile(filename, data);
            } else if (data instanceof ArrayBuffer) {
                FS.writeFile(filename, new Uint8Array(data));
            } else {
                throw new Error(`Unsupported data type for file ${filename}`);
            }
        }

        // Set up progress callback
        if (runOptions.onProgress) {
            this._tippecanoe.setProgressCallback((phase, percent, message) => {
                runOptions.onProgress({ phase, percent, message });
            });
        }

        // Find output filename from args
        let outputFile = null;
        for (let i = 0; i < args.length; i++) {
            if (args[i] === '-o' || args[i] === '--output') {
                outputFile = args[i + 1];
                break;
            }
            if (args[i].startsWith('-o')) {
                outputFile = args[i].substring(2);
                break;
            }
            if (args[i].startsWith('--output=')) {
                outputFile = args[i].substring(9);
                break;
            }
        }

        // Run tippecanoe - use runArgs with newline-separated string
        const argsStr = args.join('\n');
        const exitCode = this._tippecanoe.runArgs(argsStr);

        if (exitCode !== 0) {
            throw new Error(`Tippecanoe exited with code ${exitCode}`);
        }

        // Read output file
        let pmtiles = null;
        if (outputFile) {
            try {
                pmtiles = FS.readFile(outputFile);
                FS.unlink(outputFile);
            } catch (e) {
                // Try getting from direct writer
                pmtiles = this._tippecanoe.copyOutput();
            }
        } else {
            pmtiles = this._tippecanoe.copyOutput();
        }

        // Clean up input files
        for (const [filename] of files) {
            try {
                FS.unlink(filename);
            } catch (e) {
                // Ignore
            }
        }

        // Free output buffer
        this._tippecanoe.freeOutput();

        return {
            pmtiles: pmtiles,
            stats: {
                outputSize: pmtiles ? pmtiles.length : 0,
            },
        };
    }

    /**
     * Create directory and parents
     * @private
     */
    _mkdirp(path) {
        const { FS } = this.Module;
        const parts = path.split('/').filter(p => p);
        let current = '';

        for (const part of parts) {
            current += '/' + part;
            try {
                FS.mkdir(current);
            } catch (e) {
                // Already exists
            }
        }
    }

    /**
     * Clean up resources
     */
    dispose() {
        this._tippecanoe.freeOutput();
    }
}

// Export types for TypeScript
export { TippecanoeInstance };
