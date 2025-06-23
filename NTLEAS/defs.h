// defs.h - Additional definitions and structures for NTLEAS
#pragma once

#include <windows.h>

// Process information structure for NtQueryInformationProcess
typedef LONG NTSTATUS;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13,
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, * PPROCESS_BASIC_INFORMATION;

// SHLWAPI definitions that might not be available
#ifndef ASSOCF_NONE
#define ASSOCF_NONE                     0x00000000
#define ASSOCF_INIT_NOREMAPCLSID        0x00000001
#define ASSOCF_INIT_BYEXENAME           0x00000002
#define ASSOCF_OPEN_BYEXENAME           0x00000002
#define ASSOCF_INIT_DEFAULTTOSTAR       0x00000004
#define ASSOCF_INIT_DEFAULTTOFOLDER     0x00000008
#define ASSOCF_NOUSERSETTINGS           0x00000010
#define ASSOCF_NOTRUNCATE               0x00000020
#define ASSOCF_VERIFY                   0x00000040
#define ASSOCF_REMAPRUNDLL              0x00000080
#define ASSOCF_NOFIXUPS                 0x00000100
#define ASSOCF_IGNOREBASECLASS          0x00000200
#define ASSOCF_INIT_IGNOREUNKNOWN       0x00000400

typedef DWORD ASSOCF;

typedef enum {
    ASSOCSTR_COMMAND = 1,
    ASSOCSTR_EXECUTABLE,
    ASSOCSTR_FRIENDLYDOCNAME,
    ASSOCSTR_FRIENDLYAPPNAME,
    ASSOCSTR_NOOPEN,
    ASSOCSTR_SHELLNEWVALUE,
    ASSOCSTR_DDECOMMAND,
    ASSOCSTR_DDEIFEXEC,
    ASSOCSTR_DDEAPPLICATION,
    ASSOCSTR_DDETOPIC,
    ASSOCSTR_INFOTIP,
    ASSOCSTR_QUICKTIP,
    ASSOCSTR_TILEINFO,
    ASSOCSTR_CONTENTTYPE,
    ASSOCSTR_DEFAULTICON,
    ASSOCSTR_SHELLEXTENSION,
    ASSOCSTR_DROPTARGET,
    ASSOCSTR_DELEGATEEXECUTE,
    ASSOCSTR_SUPPORTED_URI_PROTOCOLS,
    ASSOCSTR_PROGID,
    ASSOCSTR_APPID,
    ASSOCSTR_APPPUBLISHER,
    ASSOCSTR_APPICONREFERENCE,
    ASSOCSTR_MAX
} ASSOCSTR;

#endif

// Function pointer types for dynamic loading
typedef NTSTATUS(WINAPI* PFN_NtQueryInformationProcess)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
    );

typedef BOOL(WINAPI* PFN_QueryFullProcessImageNameW)(
    HANDLE hProcess,
    DWORD dwFlags,
    LPWSTR lpExeName,
    PDWORD lpdwSize
    );

// Debug event constants
#ifndef DBG_CONTINUE
#define DBG_CONTINUE                    0x00010002L
#define DBG_EXCEPTION_NOT_HANDLED       0x80010001L
#endif

// Constants for image file machines
#ifndef IMAGE_FILE_MACHINE_AMD64
#define IMAGE_FILE_MACHINE_AMD64        0x8664
#endif

// Memory allocation types
#ifndef MEM_COMMIT
#define MEM_COMMIT                      0x1000
#define MEM_RESERVE                     0x2000
#define PAGE_EXECUTE_READWRITE          0x40
#endif

// File mapping access rights
#ifndef FILE_MAP_ALL_ACCESS
#define FILE_MAP_ALL_ACCESS             0xF001F
#endif

// Event access rights
#ifndef EVENT_ALL_ACCESS
#define EVENT_ALL_ACCESS                0x1F0003
#endif

// Process creation flags
#ifndef CREATE_SUSPENDED
#define CREATE_SUSPENDED                0x00000004
#define DEBUG_PROCESS                   0x00000001
#define DEBUG_ONLY_THIS_PROCESS         0x00000002
#endif

// Context flags
#ifndef CONTEXT_CONTROL
#define CONTEXT_i386                    0x00010000
#define CONTEXT_CONTROL                 (CONTEXT_i386 | 0x00000001L)
#define CONTEXT_INTEGER                 (CONTEXT_i386 | 0x00000002L)
#define CONTEXT_SEGMENTS                (CONTEXT_i386 | 0x00000004L)
#define CONTEXT_FLOATING_POINT          (CONTEXT_i386 | 0x00000008L)
#define CONTEXT_DEBUG_REGISTERS         (CONTEXT_i386 | 0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS      (CONTEXT_i386 | 0x00000020L)
#define CONTEXT_FULL                    (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS)
#endif

// Exception codes
#ifndef STATUS_BREAKPOINT
#define STATUS_BREAKPOINT               0x80000003L
#endif

// Additional structures that might be needed
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

// Memory manipulation macros
inline void* memset32(void* dest, DWORD value, size_t count) {
    DWORD* d = (DWORD*)dest;
    for (size_t i = 0; i < count; i++) {
        d[i] = value;
    }
    return dest;
}

// Error handling macros
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

// Safe string operations
#ifdef UNICODE
#define _tcscpy_s wcscpy_s
#define _tcscat_s wcscat_s
#define _tcslen wcslen
#define _tcschr wcschr
#else
#define _tcscpy_s strcpy_s
#define _tcscat_s strcat_s
#define _tcslen strlen
#define _tcschr strchr
#endif