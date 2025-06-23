#include "ntleai.h"

// Exception handling functions
HMODULE GenerateExceptionInfo(LPWSTR lpBuffer)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Try to load psapi.dll dynamically to avoid link dependency
    HMODULE hPsapi = LoadLibraryW(L"psapi.dll");
    if (hPsapi)
    {
        typedef DWORD(WINAPI* PFNGETMODULEBASENAMEW)(HANDLE, HMODULE, LPWSTR, DWORD);
        PFNGETMODULEBASENAMEW pfnGetModuleBaseNameW =
            (PFNGETMODULEBASENAMEW)GetProcAddress(hPsapi, "GetModuleBaseNameW");

        if (pfnGetModuleBaseNameW)
        {
            lpBuffer[0] = L'_';
            LPWSTR lpName = lpBuffer + 1;

            DWORD dwLen = pfnGetModuleBaseNameW(GetCurrentProcess(), NULL, lpName, 260);

            wsprintfW(lpName + dwLen,
                L"_%04u-%02u-%02u_%02u-%02u-%02u",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
        }

        FreeLibrary(hPsapi);
    }
    else
    {
        // Fallback if psapi is not available
        lpBuffer[0] = L'_';
        wsprintfW(lpBuffer + 1,
            L"ntleai_%04u-%02u-%02u_%02u-%02u-%02u",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);
    }

    return hPsapi;
}

LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    HMODULE hDbgHelp = LoadLibraryW(L"dbghelp.dll");
    if (hDbgHelp)
    {
        typedef BOOL(WINAPI* PFNMINIDUMPWRITEDUMP)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
            PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION,
            PMINIDUMP_CALLBACK_INFORMATION);

        PFNMINIDUMPWRITEDUMP pfnMiniDumpWriteDump =
            (PFNMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

        if (pfnMiniDumpWriteDump)
        {
            WCHAR szPath[520];
            LPCWSTR lpBasePath = L".";
            LPCWSTR lpProcessName = L"0";

            if (g_lpModulePath)
                lpBasePath = (LPCWSTR)g_lpModulePath;
            if (g_lpMemPath)
                lpProcessName = (LPCWSTR)g_lpMemPath;

            wsprintfW(szPath, L"%s\\crashinfo_%s", lpBasePath, lpProcessName);

            int nLen = lstrlenW(szPath);
            GenerateExceptionInfo(szPath + nLen);
            lstrcatW(szPath, L".dmp");

            HANDLE hFile = CreateFileW(szPath, GENERIC_WRITE, 0, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                MINIDUMP_EXCEPTION_INFORMATION mei;
                mei.ThreadId = GetCurrentThreadId();
                mei.ExceptionPointers = ExceptionInfo;
                mei.ClientPointers = FALSE;

                pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                    hFile, MiniDumpNormal, &mei, NULL, NULL);

                CloseHandle(hFile);
            }
        }

        FreeLibrary(hDbgHelp);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

LPTOP_LEVEL_EXCEPTION_FILTER InitializeExceptionHandling(HMODULE hModule)
{
    // Initialize module path string
    if (!g_lpMemPath)
    {
        SIZE_T cbSize = (lstrlenW(L"ntleai") + 1) * sizeof(WCHAR);
        g_lpMemPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cbSize);
        if (g_lpMemPath)
            lstrcpyW(g_lpMemPath, L"ntleai");
    }

    // Initialize module directory path
    if (!g_lpModulePath)
    {
        g_lpModulePath = HeapAlloc(GetProcessHeap(), 0, CONFIG_BUFFER_SIZE);
        if (g_lpModulePath)
        {
            WCHAR* lpPath = (WCHAR*)g_lpModulePath;
            DWORD dwLen = GetModuleFileNameW(hModule, lpPath, 0x208);

            // Find last backslash and terminate string there
            for (WCHAR* p = lpPath + dwLen; p > lpPath; p--)
            {
                if (*p == L'\\')
                {
                    *p = L'\0';
                    break;
                }
            }
        }
    }

    return SetUnhandledExceptionFilter(ExceptionFilter);
}

LPVOID CleanupExceptionHandling()
{
    SetUnhandledExceptionFilter(g_lpPrevExceptionFilter);

    if (g_lpMemPath)
    {
        HeapFree(GetProcessHeap(), 0, g_lpMemPath);
        g_lpMemPath = NULL;
    }

    if (g_lpModulePath)
    {
        HeapFree(GetProcessHeap(), 0, g_lpModulePath);
        g_lpModulePath = NULL;
    }

    return NULL;
}

// Worker thread that handles configuration loading and API hooking
DWORD WINAPI WorkerThread(LPVOID lpThreadParameter)
{
    HANDLE hEvent = OpenEventA(SYNCHRONIZE, FALSE, "RcpEvent000");
    HANDLE hFileMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, 4, "RcpFileMap000");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Read configuration from shared memory
        LPVOID lpView = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 4);
        if (lpView)
        {
            // Parse configuration data
            WORD* pData = (WORD*)lpView;

            // Skip to null-terminated string
            while (*pData) pData++;
            pData++;

            // Copy configuration string
            BYTE* pConfig = (BYTE*)pData;
            BYTE* pDest = (BYTE*)&g_CodePage; // Placeholder for config area
            do {
                *pDest++ = *pConfig;
            } while (*pConfig++);

            // Parse configuration flags
            DWORD dwFlags = *(DWORD*)pConfig;
            g_dwFlags = dwFlags & 0x0F;

            // Extract configuration values
            pConfig += sizeof(DWORD);
            g_CodePage = *(DWORD*)pConfig; pConfig += sizeof(DWORD);
            g_LocaleId = *(DWORD*)pConfig; pConfig += sizeof(DWORD);
            g_TimeZoneOffset = *(DWORD*)pConfig; pConfig += sizeof(DWORD);

            DWORD dwMaxWait = *(DWORD*)pConfig;
            g_MaxWaitTime = dwMaxWait ? dwMaxWait : 100;

            UnmapViewOfFile(lpView);
            CloseHandle(hFileMapping);

            // Get codepage detection info
            g_dwDetectedCodePage1 = GetLocaleInfo(g_CodePage);
            g_dwDetectedCodePage2 = GetLocaleInfo(GetACP());

            // Clean up exception handling if not persistent
            if (!(g_dwFlags & 0x8000))
                CleanupExceptionHandling();
        }
    }
    else
    {
        // Use default configuration
        CloseHandle(hFileMapping);
        g_CodePage = 932;  // Japanese Shift-JIS
        g_LocaleId = 1041; // Japanese locale
        g_TimeZoneOffset = -540; // JST offset
        g_MaxWaitTime = 100;
    }

    // Check Windows version compatibility
    if (!CheckWindowsVersion())
        return -1;

    // Install API hooks
    NTLEAConfig config = { 0 };
    config.codePage = g_CodePage;
    config.localeId = g_LocaleId;
    config.timeZoneOffset = g_TimeZoneOffset;
    config.maxWaitTime = g_MaxWaitTime;
    config.flags = g_dwFlags;

    InstallAPIHooks(&config);

    // Signal initialization complete
    if (hEvent)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    return 0;
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Initialize exception handling
        g_lpPrevExceptionFilter = InitializeExceptionHandling(hinstDLL);

        // Create private heap
        g_hHeap = HeapCreate(0, 0, 0);
        if (!g_hHeap)
            return FALSE;

        // Allocate TLS slot
        g_dwTlsIndex = TlsAlloc();
        if (g_dwTlsIndex == TLS_OUT_OF_INDEXES)
        {
            HeapDestroy(g_hHeap);
            return FALSE;
        }

        // Initialize configuration
        g_MaxWaitTime = 100;
        g_TimeZoneOffset = -480; // Default PST offset

        // Start worker thread
        HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
        if (hThread)
            CloseHandle(hThread);

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        // Cleanup TLS data
        if (g_dwTlsIndex != TLS_OUT_OF_INDEXES)
        {
            LPVOID lpTlsData = TlsGetValue(g_dwTlsIndex);
            if (lpTlsData)
                HeapFree(g_hHeap, 0, lpTlsData);
            TlsFree(g_dwTlsIndex);
        }

        // Cleanup heap
        if (g_hHeap)
            HeapDestroy(g_hHeap);

        // Cleanup exception handling
        CleanupExceptionHandling();
        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        // No per-thread initialization needed
        break;
    }

    return TRUE;
}