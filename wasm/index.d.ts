/**
 * Tippecanoe WASM - Generate PMTiles in the browser
 */

export interface TippecanoeOptions {
    /**
     * Enable multi-threading (requires SharedArrayBuffer and COOP/COEP headers)
     * @default true
     */
    threads?: boolean;

    /**
     * Maximum memory in bytes
     * @default 2147483648 (2GB)
     */
    maxMemory?: number;

    /**
     * Callback for stdout messages
     */
    onStdout?: (text: string) => void;

    /**
     * Callback for stderr messages
     */
    onStderr?: (text: string) => void;
}

export interface ProgressInfo {
    phase: string;
    percent: number;
    message: string;
}

export interface RunOptions {
    /**
     * Progress callback
     */
    onProgress?: (info: ProgressInfo) => void;
}

export interface TippecanoeResult {
    /**
     * The generated PMTiles data
     */
    pmtiles: Uint8Array | null;

    /**
     * Statistics about the generation
     */
    stats: {
        outputSize: number;
    };
}

export interface TippecanoeInstance {
    /**
     * Run tippecanoe with CLI-like arguments
     *
     * @example
     * const result = await tippecanoe.run(
     *   ['-o', 'output.pmtiles', '-z14', 'input.geojson'],
     *   new Map([['input.geojson', geojsonData]])
     * );
     *
     * @param args - Command line arguments
     * @param files - Input files as Map of filename to data
     * @param options - Run options
     */
    run(
        args: string[],
        files?: Map<string, Uint8Array | string | ArrayBuffer>,
        options?: RunOptions
    ): Promise<TippecanoeResult>;

    /**
     * Clean up resources
     */
    dispose(): void;
}

/**
 * Create a Tippecanoe instance
 *
 * @example
 * import { createTippecanoe } from 'tippecanoe-wasm';
 *
 * const tippecanoe = await createTippecanoe();
 * const geojson = JSON.stringify({
 *   type: 'FeatureCollection',
 *   features: [...]
 * });
 *
 * const result = await tippecanoe.run(
 *   ['-o', 'output.pmtiles', '-zg', 'data.geojson'],
 *   new Map([['data.geojson', geojson]])
 * );
 *
 * // Use result.pmtiles...
 * tippecanoe.dispose();
 */
export function createTippecanoe(options?: TippecanoeOptions): Promise<TippecanoeInstance>;
