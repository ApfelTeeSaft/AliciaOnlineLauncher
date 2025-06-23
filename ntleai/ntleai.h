#pragma once

#ifndef NTLEAI_H
#define NTLEAI_H

#include <windows.h>
#include <psapi.h>
#include <ole2.h>

#ifndef HAS_VERSION_H
// Manual declarations for version functions if version.h is not available
extern "C" {
    BOOL APIENTRY VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen);
    ULONGLONG APIENTRY VerSetConditionMask(ULONGLONG ConditionMask, DWORD TypeMask, BYTE Condition);
    BOOL APIENTRY VerifyVersionInfoA(LPOSVERSIONINFOEXA lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask);
}

// Version comparison constants
#ifndef VER_MAJORVERSION
#define VER_MAJORVERSION            0x0000002
#define VER_MINORVERSION            0x0000001
#define VER_SERVICEPACKMAJOR        0x0000020
#define VER_SERVICEPACKMINOR        0x0000010
#define VER_GREATER_EQUAL           3
#endif

#endif // HAS_VERSION_H

// Try to include dbghelp.h, if not available, declare what we need
#ifndef _IMAGEHLP_
// Manual declarations for debug help functions
typedef enum _MINIDUMP_TYPE {
    MiniDumpNormal = 0x00000000
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, * PMINIDUMP_EXCEPTION_INFORMATION;

typedef struct _MINIDUMP_USER_STREAM_INFORMATION* PMINIDUMP_USER_STREAM_INFORMATION;
typedef struct _MINIDUMP_CALLBACK_INFORMATION* PMINIDUMP_CALLBACK_INFORMATION;
#endif

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "ole32.lib")

// Only link psapi if we need it
#ifdef PSAPI_VERSION
#pragma comment(lib, "psapi.lib")
#endif

// Constants
#define MAX_CLASSNAMES 15
#define HEAP_SIZE 0x9C
#define CONFIG_BUFFER_SIZE 0x410
#define CRASH_BUFFER_SIZE 0x240

// Configuration structure
typedef struct {
    UINT codePage;
    DWORD localeId;
    DWORD timeZoneOffset;
    DWORD maxWaitTime;
    DWORD flags;
} NTLEAConfig;

// Function pointer types for hooked functions
typedef int (WINAPI* FUNC_RtlUnicodeToMultiByteN)(LPSTR, int, int*, LPCWCH, unsigned int);
typedef int (WINAPI* FUNC_RtlMultiByteToUnicodeN)(LPWSTR, unsigned int, DWORD*, LPCCH, int);
typedef int (WINAPI* FUNC_WideCharToMultiByte)(int, int, int, int, int, int, int, int);
typedef int (WINAPI* FUNC_MultiByteToWideChar)(int, int, int, int, int, int);
typedef int (WINAPI* FUNC_CompareStringA)(int, int, int, int, int, int);
typedef int (WINAPI* FUNC_GetCPInfo)(int, int);
typedef int (WINAPI* FUNC_GetTimeZoneInformation)(DWORD*);
typedef int (WINAPI* FUNC_VerQueryValueA)(int, LPCSTR, LPVOID*, DWORD*);

// Global variables
extern HANDLE g_hHeap;
extern DWORD g_dwTlsIndex;
extern UINT g_CodePage;
extern DWORD g_LocaleId;
extern DWORD g_TimeZoneOffset;
extern DWORD g_MaxWaitTime;
extern LPTOP_LEVEL_EXCEPTION_FILTER g_lpPrevExceptionFilter;
extern LPWSTR g_lpMemPath;
extern LPVOID g_lpModulePath;

// Additional configuration variables
extern DWORD g_dwFlags;
extern DWORD g_dwDetectedCodePage1;
extern DWORD g_dwDetectedCodePage2;
extern WORD g_wOriginalLangId;
extern WORD g_wOriginalCodePage;

// Function pointers for original APIs
extern FUNC_RtlUnicodeToMultiByteN g_pfnOrigRtlUnicodeToMultiByteN;
extern FUNC_RtlMultiByteToUnicodeN g_pfnOrigRtlMultiByteToUnicodeN;
extern FUNC_WideCharToMultiByte g_pfnOrigWideCharToMultiByte;
extern FUNC_MultiByteToWideChar g_pfnOrigMultiByteToWideChar;
extern FUNC_CompareStringA g_pfnOrigCompareStringA;
extern FUNC_GetCPInfo g_pfnOrigGetCPInfo;
extern FUNC_GetTimeZoneInformation g_pfnOrigGetTimeZoneInformation;
extern FUNC_VerQueryValueA g_pfnOrigVerQueryValueA;

// Window class names for hooking
extern const WCHAR* g_WideClassNames[MAX_CLASSNAMES];
extern const CHAR* g_AnsiClassNames[MAX_CLASSNAMES];

// Thread synchronization
extern LONG g_lDestination;
extern LONG g_lAddend;
extern LONG g_lCriticalSection;
extern DWORD g_dwCurrentThreadId1;
extern DWORD g_dwCurrentThreadId2;
extern DWORD g_dwThreadCounter1;
extern DWORD g_dwThreadCounter2;

// Function declarations
WNDPROC* InitializeWndProcTable();
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
DWORD WINAPI WorkerThread(LPVOID lpThreadParameter);

// String conversion functions
int CDECL ConvertUnicodeToMultiByteInternal(LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpBuffer, int cbBuffer);
int CDECL ConvertMultiByteToUnicodeInternal(LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpBuffer, int cchBuffer);

// API Hook functions
int WINAPI Hook_RtlUnicodeToMultiByteN(LPSTR lpMultiByteStr, int cbMultiByte, int* pcbConverted, LPCWCH lpWideCharStr, unsigned int cchWideChar);
int WINAPI Hook_RtlMultiByteToUnicodeN(LPWSTR lpWideCharStr, unsigned int cbWideChar, DWORD* pcbConverted, LPCCH lpMultiByteStr, int cbMultiByte);
int WINAPI Hook_WideCharToMultiByteSize(int* pcbRequired, LPCWCH lpWideCharStr, int cchWideChar);
int WINAPI Hook_MultiByteToWideCharSize(int* pcchRequired, LPCCH lpMultiByteStr, int cbMultiByte);
BOOL WINAPI Hook_IsDBCSLeadByteEx(BYTE TestChar);
LPSTR WINAPI Hook_CharPrevExA(LPCSTR lpStart, LPCSTR lpCurrentChar);
LPSTR WINAPI Hook_CharNextExA(LPCSTR lpCurrentChar);
int WINAPI Hook_VerQueryValueA(int pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, DWORD* puLen);
int WINAPI Hook_GetTimeZoneInformation(DWORD* lpTimeZoneInformation);
int WINAPI Hook_CompareStringA(int Locale, int dwCmpFlags, int lpString1, int cchCount1, int lpString2, int cchCount2);
int WINAPI Hook_MultiByteToWideChar(int CodePage, int dwFlags, int lpMultiByteStr, int cbMultiByte, int lpWideCharStr, int cchWideChar);
int WINAPI Hook_WideCharToMultiByte(int CodePage, int dwFlags, int lpWideCharStr, int cchWideChar, int lpMultiByteStr, int cbMultiByte, int lpDefaultChar, int lpUsedDefaultChar);
int WINAPI Hook_GetCPInfo(int CodePage, int lpCPInfo);

// Utility functions
int GetLocaleInfo(UINT CodePage);
BOOL CheckWindowsVersion();
HMODULE GenerateExceptionInfo(LPWSTR lpBuffer);
LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo);
LPTOP_LEVEL_EXCEPTION_FILTER InitializeExceptionHandling(HMODULE hModule);
LPVOID CleanupExceptionHandling();
int InstallAPIHooks(NTLEAConfig* pConfig);
int HookFunction(LPCSTR lpProcName, LPVOID lpNewFunction, HMODULE hModule);
LPVOID MemoryCopy(LPVOID lpDest, LPCVOID lpSrc, SIZE_T cbSize);
LPVOID MemorySet(LPVOID lpDest, int nValue, SIZE_T cbSize);

// Additional hook functions declared in hooks.cpp
void InstallLocaleHooks();
void nullsub_1();
void nullsub_2();

// COM Interface IDs (from IDA analysis)
extern const IID CLSID_CPDetector;
extern const IID IID_ICodePageDetector;

// Forward declaration
typedef struct ICodePageDetector ICodePageDetector;

// COM Interface definition based on IDA disassembly analysis
// The interface calls method at vtable offset 0x10 (4th method after IUnknown)
#ifndef __ICodePageDetector_INTERFACE_DEFINED__
#define __ICodePageDetector_INTERFACE_DEFINED__

#ifdef __cplusplus
extern "C" {
#endif

    // Interface definition based on vtable analysis
    typedef struct ICodePageDetectorVtbl
    {
        HRESULT(STDMETHODCALLTYPE* QueryInterface)(
            ICodePageDetector* This,
            REFIID riid,
            void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(
            ICodePageDetector* This);

        ULONG(STDMETHODCALLTYPE* Release)(
            ICodePageDetector* This);

        // First custom method (offset 0x0C)
        HRESULT(STDMETHODCALLTYPE* Method1)(
            ICodePageDetector* This);

        // Second custom method (offset 0x10) - this is what the code calls
        HRESULT(STDMETHODCALLTYPE* DetectCodePage)(
            ICodePageDetector* This,
            UINT uiCodePage,
            LPVOID lpBuffer);

    } ICodePageDetectorVtbl;

    struct ICodePageDetector
    {
        const ICodePageDetectorVtbl* lpVtbl;
    };

#ifdef __cplusplus
}
#endif

#endif // __ICodePageDetector_INTERFACE_DEFINED__

#endif // NTLEAI_H