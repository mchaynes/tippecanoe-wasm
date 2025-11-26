// Pre-initialization script for tippecanoe WASM
// This runs before the WASM module is loaded

Module['noInitialRun'] = true;

// Allow configuration of memory limit from JavaScript
if (typeof Module.TIPPECANOE_MAX_MEMORY === 'undefined') {
    Module.TIPPECANOE_MAX_MEMORY = 2 * 1024 * 1024 * 1024; // 2GB default
}

// Setup virtual filesystem paths - use both preRun and onRuntimeInitialized for safety
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function() {
    // Create /tmp for temp files
    try {
        FS.mkdir('/tmp');
    } catch (e) {
        // Already exists
    }

    // Create /work for IDBFS (if needed)
    try {
        FS.mkdir('/work');
    } catch (e) {
        // Already exists
    }
});

// Also ensure directories exist after runtime init
var origOnRuntimeInitialized = Module['onRuntimeInitialized'];
Module['onRuntimeInitialized'] = function() {
    try {
        if (!FS.analyzePath('/tmp').exists) {
            FS.mkdir('/tmp');
        }
    } catch (e) {
        // Ignore
    }
    if (origOnRuntimeInitialized) {
        origOnRuntimeInitialized();
    }
};

// Handle stdout/stderr for progress messages
Module['print'] = Module['print'] || function(text) {
    if (Module.onStdout) {
        Module.onStdout(text);
    } else {
        console.log(text);
    }
};

Module['printErr'] = Module['printErr'] || function(text) {
    if (Module.onStderr) {
        Module.onStderr(text);
    } else {
        console.error(text);
    }
};
