#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <memory>

// Application constants
#define APP_NAME L"AliciaOnline Launcher"
#define APP_VERSION L"1.0.0.0"
#define ENCRYPTION_KEY_BASE 0xB11924E1
#define CRC_POLYNOMIAL 0xEDB88320
#define STREAM_KEY_INIT 0x96438AF7
#define TRANSFORM_CONSTANT 0x7C5D0F85
#define CHUNK_CONSTANT_1 0x2611501
#define CHUNK_CONSTANT_2 0x3938731

// Magic signatures from assembly
#define SIGNATURE_XOR 0xC56210EC
#define SIGNATURE_CHECK 0xFC5ABF8A

// File and registry constants
#define ALICIA_REGISTRY_PATH L"AliciaOnlineShortcut"
#define UPDATE_EXE_PATH L"Install\\Update.exe"
#define SETUP_DATA_FILE L"\\AliciaOnline\\Setup\\0.dat"

// Common structures
struct FileMapping {
    void* data;
    DWORD size;
    HANDLE fileHandle;
    HANDLE mappingHandle;
};

struct StringBuffer {
    void* buffer;
    DWORD length;
};

// Global variables
extern DWORD g_crcTable[256];
extern bool g_crcTableInitialized;
extern const char* g_hexChars;

// Memory allocation function pointers (matching assembly pattern)
extern void* (*g_allocFunc)(int, int, int);
extern void (*g_freeFunc)(void*);

// Utility macros
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#define SAFE_FREE(p) { if(p) { free(p); (p) = nullptr; } }
#define SAFE_CLOSE_HANDLE(h) { if(h && h != INVALID_HANDLE_VALUE) { CloseHandle(h); h = nullptr; } }