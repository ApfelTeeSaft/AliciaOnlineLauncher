#include "common.h"
#include "launcher.h"
#include "encryption.h"
#include "memory.h"

DWORD g_crcTable[256];
bool g_crcTableInitialized = false;
const char* g_hexChars = "0123456789ABCDEF";

void* (*g_allocFunc)(int, int, int) = AllocateMemory;
void (*g_freeFunc)(void*) = FreeMemory;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd)
{
    InitializeCrcTable();

    // Get module checksum for validation (matches assembly at 0x4010FF)
    DWORD moduleChecksum = GetModuleChecksum(hInstance);

    // Calculate entry point offset (matches assembly pattern)
    // Original: lea eax, [eax+esi-400000h] where eax is checksum, esi is hInstance
    DWORD_PTR entryOffset = moduleChecksum + reinterpret_cast<DWORD_PTR>(hInstance) - 0x400000;

    return MainLauncherLogic(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

extern "C" int _CRT_INIT(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

// DLL entry point (if building as DLL - not used in this case)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize global state
        g_crcTableInitialized = false;
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}