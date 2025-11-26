// Minimal SQLite3 stub for WASM builds
// Provides type definitions and no-op functions to satisfy compilation
// Actual SQLite functionality is bypassed in WASM mode

#ifndef SQLITE3_STUB_H
#define SQLITE3_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

// Return codes
#define SQLITE_OK           0
#define SQLITE_ERROR        1
#define SQLITE_BUSY         5
#define SQLITE_LOCKED       6
#define SQLITE_NOMEM        7
#define SQLITE_READONLY     8
#define SQLITE_INTERRUPT    9
#define SQLITE_IOERR       10
#define SQLITE_CORRUPT     11
#define SQLITE_NOTFOUND    12
#define SQLITE_FULL        13
#define SQLITE_CANTOPEN    14
#define SQLITE_ROW        100
#define SQLITE_DONE       101

// Config options
#define SQLITE_CONFIG_SERIALIZED 3

// Types
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

// Stub implementations - these should never be called in WASM mode
// but need to exist for compilation

static inline int sqlite3_open(const char *filename, sqlite3 **ppDb) {
    (void)filename;
    *ppDb = (sqlite3 *)0;
    return SQLITE_ERROR;  // Always fail - WASM shouldn't use this
}

static inline int sqlite3_close(sqlite3 *db) {
    (void)db;
    return SQLITE_OK;
}

static inline int sqlite3_exec(sqlite3 *db, const char *sql,
    int (*callback)(void*,int,char**,char**), void *arg, char **errmsg) {
    (void)db; (void)sql; (void)callback; (void)arg;
    if (errmsg) *errmsg = (char *)"SQLite not available in WASM";
    return SQLITE_ERROR;
}

static inline int sqlite3_prepare_v2(sqlite3 *db, const char *sql, int nByte,
    sqlite3_stmt **ppStmt, const char **pzTail) {
    (void)db; (void)sql; (void)nByte; (void)pzTail;
    *ppStmt = (sqlite3_stmt *)0;
    return SQLITE_ERROR;
}

static inline int sqlite3_step(sqlite3_stmt *stmt) {
    (void)stmt;
    return SQLITE_DONE;
}

static inline int sqlite3_finalize(sqlite3_stmt *stmt) {
    (void)stmt;
    return SQLITE_OK;
}

static inline int sqlite3_bind_int(sqlite3_stmt *stmt, int i, int val) {
    (void)stmt; (void)i; (void)val;
    return SQLITE_OK;
}

static inline int sqlite3_bind_text(sqlite3_stmt *stmt, int i, const char *val, int n, void (*del)(void*)) {
    (void)stmt; (void)i; (void)val; (void)n; (void)del;
    return SQLITE_OK;
}

static inline int sqlite3_bind_blob(sqlite3_stmt *stmt, int i, const void *val, int n, void (*del)(void*)) {
    (void)stmt; (void)i; (void)val; (void)n; (void)del;
    return SQLITE_OK;
}

static inline int sqlite3_column_int(sqlite3_stmt *stmt, int i) {
    (void)stmt; (void)i;
    return 0;
}

static inline const unsigned char *sqlite3_column_text(sqlite3_stmt *stmt, int i) {
    (void)stmt; (void)i;
    return (const unsigned char *)"";
}

static inline const void *sqlite3_column_blob(sqlite3_stmt *stmt, int i) {
    (void)stmt; (void)i;
    return (const void *)0;
}

static inline int sqlite3_column_bytes(sqlite3_stmt *stmt, int i) {
    (void)stmt; (void)i;
    return 0;
}

static inline const char *sqlite3_errmsg(sqlite3 *db) {
    (void)db;
    return "SQLite not available in WASM";
}

static inline char *sqlite3_mprintf(const char *fmt, ...) {
    (void)fmt;
    return (char *)0;
}

static inline void sqlite3_free(void *p) {
    (void)p;
}

static inline int sqlite3_config(int op, ...) {
    (void)op;
    return SQLITE_OK;
}

#ifdef __cplusplus
}
#endif

#endif // SQLITE3_STUB_H
