import { test, expect } from '@playwright/test';
import { createServer } from 'http';
import { readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const wasmDir = join(__dirname, '..');

// Simple static file server
function startServer(port) {
    return new Promise((resolve) => {
        const server = createServer((req, res) => {
            // Strip query string from URL
            const urlPath = req.url.split('?')[0];
            let filePath = join(wasmDir, urlPath === '/' ? '/test/test.html' : urlPath);

            try {
                const content = readFileSync(filePath);
                const ext = filePath.split('.').pop();
                const mimeTypes = {
                    'html': 'text/html',
                    'js': 'application/javascript',
                    'wasm': 'application/wasm',
                    'json': 'application/json',
                    'geojson': 'application/json',
                };
                res.writeHead(200, {
                    'Content-Type': mimeTypes[ext] || 'application/octet-stream',
                    'Cross-Origin-Opener-Policy': 'same-origin',
                    'Cross-Origin-Embedder-Policy': 'require-corp',
                });
                res.end(content);
            } catch (e) {
                console.error('404:', req.url, e.message);
                res.writeHead(404);
                res.end('Not found: ' + req.url);
            }
        });

        server.listen(port, () => {
            resolve(server);
        });
    });
}

test.describe('PMTiles WASM', () => {
    let server;
    const PORT = 8765;

    test.beforeAll(async () => {
        server = await startServer(PORT);
    });

    test.afterAll(async () => {
        server?.close();
    });

    test('converts GeoJSON to valid PMTiles', async ({ page }) => {
        // Collect console messages
        const logs = [];
        page.on('console', msg => {
            logs.push(msg.text());
            console.log('Browser:', msg.text());
        });

        await page.goto(`http://localhost:${PORT}/test/test.html`);

        // Wait for test to complete (with timeout)
        await page.waitForFunction(() => window.testResult !== undefined, { timeout: 60000 });

        // Get test result
        const result = await page.evaluate(() => window.testResult);

        console.log('\n--- Test Logs ---');
        logs.forEach(l => console.log(l));
        console.log('-----------------\n');

        expect(result.success).toBe(true);
        if (!result.success) {
            throw new Error(result.error);
        }
    });
});
