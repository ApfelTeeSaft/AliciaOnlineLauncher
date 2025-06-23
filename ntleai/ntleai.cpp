#include "ntleai.h"

// Global variables
HANDLE g_hHeap = NULL;
DWORD g_dwTlsIndex = 0;
UINT g_CodePage = 0;
DWORD g_LocaleId = 0;
DWORD g_TimeZoneOffset = 0;
DWORD g_MaxWaitTime = 100;
LPTOP_LEVEL_EXCEPTION_FILTER g_lpPrevExceptionFilter = NULL;
LPWSTR g_lpMemPath = NULL;
LPVOID g_lpModulePath = NULL;

// Thread synchronization
LONG g_lDestination = 0;
LONG g_lAddend = 0;
LONG g_lCriticalSection = 0;
DWORD g_dwCurrentThreadId1 = 0;
DWORD g_dwCurrentThreadId2 = 0;
DWORD g_dwThreadCounter1 = 0;
DWORD g_dwThreadCounter2 = 0;

// Configuration flags
DWORD g_dwFlags = 0;
DWORD g_dwDetectedCodePage1 = 0;
DWORD g_dwDetectedCodePage2 = 0;
WORD g_wOriginalLangId = 0;
WORD g_wOriginalCodePage = 0;

// Function pointers for original APIs
FUNC_RtlUnicodeToMultiByteN g_pfnOrigRtlUnicodeToMultiByteN = NULL;
FUNC_RtlMultiByteToUnicodeN g_pfnOrigRtlMultiByteToUnicodeN = NULL;
FUNC_WideCharToMultiByte g_pfnOrigWideCharToMultiByte = NULL;
FUNC_MultiByteToWideChar g_pfnOrigMultiByteToWideChar = NULL;
FUNC_CompareStringA g_pfnOrigCompareStringA = NULL;
FUNC_GetCPInfo g_pfnOrigGetCPInfo = NULL;
FUNC_GetTimeZoneInformation g_pfnOrigGetTimeZoneInformation = NULL;
FUNC_VerQueryValueA g_pfnOrigVerQueryValueA = NULL;

// Window class names for hooking
const WCHAR* g_WideClassNames[MAX_CLASSNAMES] = {
    L"BUTTON", L"COMBOBOX", L"ComboLBox", L"EDIT", L"LISTBOX",
    L"MDICLIENT", L"RichEdit", L"RICHEDIT_CLASS", L"SCROLLBAR",
    L"STATIC", L"SysTreeView32", L"SysListView32", L"SysAnimate32",
    L"SysHeader32", L"tooltips_class32"
};

const CHAR* g_AnsiClassNames[MAX_CLASSNAMES] = {
    "BUTTON", "COMBOBOX", "ComboLBox", "EDIT", "LISTBOX",
    "MDICLIENT", "RichEdit", "RICHEDIT_CLASS", "SCROLLBAR",
    "STATIC", "SysTreeView32", "SysListView32", "SysAnimate32",
    "SysHeader32", "tooltips_class32"
};

// COM Interface IDs
const IID CLSID_CPDetector = { 0x275C23E2, 0x3747, 0x11D0, { 0x9F, 0xEA, 0x00, 0xAA, 0x00, 0x3F, 0x86, 0x46 } };
const IID IID_ICodePageDetector = { 0x275C23E1, 0x3747, 0x11D0, { 0x9F, 0xEA, 0x00, 0xAA, 0x00, 0x3F, 0x86, 0x46 } };

// Initialize Window Procedure table for class hooking
WNDPROC* InitializeWndProcTable()
{
    DWORD dwLastError = GetLastError();
    WNDPROC* pWndProcTable = (WNDPROC*)TlsGetValue(g_dwTlsIndex);
    SetLastError(dwLastError);

    if (!pWndProcTable)
    {
        pWndProcTable = (WNDPROC*)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, HEAP_SIZE);
        if (!pWndProcTable)
            return NULL;

        TlsSetValue(g_dwTlsIndex, pWndProcTable);

        WNDPROC* pCurrentEntry = pWndProcTable + 10; // Offset to start of table

        for (int i = 0; i < MAX_CLASSNAMES; i++)
        {
            WNDCLASSA wndClassA;
            if (GetClassInfoA(NULL, g_AnsiClassNames[i], &wndClassA))
            {
                *(pCurrentEntry - 1) = wndClassA.lpfnWndProc;
            }

            WNDCLASSW wndClassW;
            if (GetClassInfoW(NULL, g_WideClassNames[i], &wndClassW))
            {
                *pCurrentEntry = wndClassW.lpfnWndProc;
            }

            pCurrentEntry += 2;
        }

        SetLastError(0);
    }

    return pWndProcTable;
}

// String conversion with locale handling
int CDECL ConvertUnicodeToMultiByteInternal(LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpBuffer, int cbBuffer)
{
    int cbRequired = WideCharToMultiByte(0, 0, lpWideCharStr, cchWideChar, NULL, 0, NULL, NULL);
    if (cbRequired <= 0)
        return cbBuffer;

    LPSTR lpTempBuffer = (LPSTR)HeapAlloc(g_hHeap, 0, cbRequired);
    if (!lpTempBuffer)
        return cbBuffer;

    WideCharToMultiByte(0, 0, lpWideCharStr, cchWideChar, lpTempBuffer, cbRequired, NULL, NULL);
    MemoryCopy(lpBuffer, lpTempBuffer, cbBuffer);
    HeapFree(g_hHeap, 0, lpTempBuffer);

    return cbBuffer;
}

int CDECL ConvertMultiByteToUnicodeInternal(LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpBuffer, int cchBuffer)
{
    int cchRequired = MultiByteToWideChar(0, 0, lpMultiByteStr, cbMultiByte, NULL, 0);
    if (cchRequired <= 0)
        return cchBuffer;

    LPWSTR lpTempBuffer = (LPWSTR)HeapAlloc(g_hHeap, 0, cchRequired * sizeof(WCHAR));
    if (!lpTempBuffer)
        return cchBuffer;

    MultiByteToWideChar(0, 0, lpMultiByteStr, cbMultiByte, lpTempBuffer, cchRequired);
    MemoryCopy(lpBuffer, lpTempBuffer, cchBuffer * sizeof(WCHAR));
    HeapFree(g_hHeap, 0, lpTempBuffer);

    return cchBuffer;
}

// Hooked API functions
int WINAPI Hook_RtlUnicodeToMultiByteN(LPSTR lpMultiByteStr, int cbMultiByte, int* pcbConverted, LPCWCH lpWideCharStr, unsigned int cchWideChar)
{
    // Thread synchronization
    while (!InterlockedCompareExchange(&g_lAddend, 1, 0))
    {
        if (GetCurrentThreadId() == g_dwCurrentThreadId1)
            break;
        Sleep(0);
    }

    if (g_dwThreadCounter2)
        return g_pfnOrigRtlUnicodeToMultiByteN(lpMultiByteStr, cbMultiByte, pcbConverted, lpWideCharStr, cchWideChar);

    DWORD dwCurrentThreadId = GetCurrentThreadId();
    g_dwThreadCounter2++;
    g_dwCurrentThreadId1 = dwCurrentThreadId;

    int result = WideCharToMultiByte(0, 0, lpWideCharStr, cchWideChar >> 1, lpMultiByteStr, cbMultiByte, NULL, NULL);

    if (!result)
    {
        DWORD dwLastError = GetLastError();
        SetLastError(0);
        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            result = ConvertUnicodeToMultiByteInternal(lpWideCharStr, cchWideChar >> 1, lpMultiByteStr, cbMultiByte);
        }
    }

    g_dwThreadCounter2--;
    InterlockedDecrement(&g_lAddend);

    if (pcbConverted)
        *pcbConverted = result;

    return 0;
}

int WINAPI Hook_RtlMultiByteToUnicodeN(LPWSTR lpWideCharStr, unsigned int cbWideChar, DWORD* pcbConverted, LPCCH lpMultiByteStr, int cbMultiByte)
{
    // Similar implementation to Hook_RtlUnicodeToMultiByteN but for reverse conversion
    while (!InterlockedCompareExchange(&g_lDestination, 1, 0))
    {
        if (GetCurrentThreadId() == g_dwCurrentThreadId2)
            break;
        Sleep(0);
    }

    if (g_dwThreadCounter1)
        return g_pfnOrigRtlMultiByteToUnicodeN(lpWideCharStr, cbWideChar, pcbConverted, lpMultiByteStr, cbMultiByte);

    DWORD dwCurrentThreadId = GetCurrentThreadId();
    g_dwThreadCounter1++;
    g_dwCurrentThreadId2 = dwCurrentThreadId;

    int result = MultiByteToWideChar(0, 0, lpMultiByteStr, cbMultiByte, lpWideCharStr, cbWideChar >> 1);

    if (!result)
    {
        DWORD dwLastError = GetLastError();
        SetLastError(0);
        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            result = ConvertMultiByteToUnicodeInternal(lpMultiByteStr, cbMultiByte, lpWideCharStr, cbWideChar >> 1);
        }
    }

    g_dwThreadCounter1--;
    InterlockedDecrement(&g_lDestination);

    if (pcbConverted)
        *pcbConverted = result * 2;

    return 0;
}

BOOL WINAPI Hook_IsDBCSLeadByteEx(BYTE TestChar)
{
    return IsDBCSLeadByteEx(g_CodePage, TestChar);
}

LPSTR WINAPI Hook_CharPrevExA(LPCSTR lpStart, LPCSTR lpCurrentChar)
{
    return CharPrevExA((WORD)g_CodePage, lpStart, lpCurrentChar, 0);
}

LPSTR WINAPI Hook_CharNextExA(LPCSTR lpCurrentChar)
{
    return CharNextExA((WORD)g_CodePage, lpCurrentChar, 0);
}

int WINAPI Hook_VerQueryValueA(int pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, DWORD* puLen)
{
    // Critical section for thread safety
    while (!InterlockedCompareExchange(&g_lCriticalSection, 1, 0))
        Sleep(0);

    int result = 1;

    if (lstrcmpA(lpSubBlock, "\\VarFileInfo\\Translation") == 0)
    {
        if (g_pfnOrigVerQueryValueA && *puLen >= 4)
        {
            WORD* pTranslation = (WORD*)*lplpBuffer;
            g_wOriginalLangId = pTranslation[0];
            g_wOriginalCodePage = pTranslation[1];
            pTranslation[0] = (WORD)g_LocaleId;
            pTranslation[1] = (WORD)g_CodePage;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        // Handle string file info queries
        int len = lstrlenA(lpSubBlock);
        if (len > 2 && lpSubBlock[0] == '\\' && lpSubBlock[1] == 'S')
        {
            InterlockedDecrement(&g_lCriticalSection);

            // Find the last backslash
            LPCSTR lpLastBackslash = lpSubBlock + len;
            while (lpLastBackslash > lpSubBlock && *lpLastBackslash != '\\')
                lpLastBackslash--;

            // Format the query string with original locale info
            CHAR szQuery[64];
            wsprintfA(szQuery, "\\StringFileInfo\\%04x%04x\\%s",
                g_wOriginalLangId, g_wOriginalCodePage, lpLastBackslash + 1);

            // Call original function with formatted query
            if (g_pfnOrigVerQueryValueA)
                return g_pfnOrigVerQueryValueA(pBlock, szQuery, lplpBuffer, puLen);
            else
                return 0;
        }
        else
        {
            InterlockedDecrement(&g_lCriticalSection);
            if (g_pfnOrigVerQueryValueA)
                return g_pfnOrigVerQueryValueA(pBlock, lpSubBlock, lplpBuffer, puLen);
            else
                return 0;
        }
    }

    InterlockedDecrement(&g_lCriticalSection);
    return result;
}

// Function hooking mechanism
int HookFunction(LPCSTR lpProcName, LPVOID lpNewFunction, HMODULE hModule)
{
    FARPROC pfnOriginal;

    if (hModule)
        pfnOriginal = GetProcAddress(hModule, lpProcName);
    else
        pfnOriginal = (FARPROC)lpProcName;

    if (!pfnOriginal)
        return 0;

    DWORD dwOldProtect;
    if (!VirtualProtect(pfnOriginal, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return 0;

    // Create JMP instruction
    BYTE* pCode = (BYTE*)pfnOriginal;
    pCode[0] = 0xE9; // JMP opcode
    *(DWORD*)(pCode + 1) = (DWORD)lpNewFunction - (DWORD)pfnOriginal - 5;

    FlushInstructionCache(GetCurrentProcess(), pfnOriginal, 5);
    VirtualProtect(pfnOriginal, 5, dwOldProtect, &dwOldProtect);

    return (int)pfnOriginal + 5;
}

// Install all API hooks
int InstallAPIHooks(NTLEAConfig* pConfig)
{
    HMODULE hNtdll = LoadLibraryA("ntdll.dll");
    if (!hNtdll)
        return 0;

    // Hook NTDLL functions
    HookFunction("RtlUnicodeToMultiByteN", (LPVOID)Hook_RtlUnicodeToMultiByteN, hNtdll);
    HookFunction("RtlMultiByteToUnicodeN", (LPVOID)Hook_RtlMultiByteToUnicodeN, hNtdll);

    // Hook KERNEL32 functions
    HookFunction((LPCSTR)GetACP, (LPVOID)GetACP, NULL);
    HookFunction((LPCSTR)GetOEMCP, (LPVOID)GetOEMCP, NULL);
    HookFunction((LPCSTR)IsDBCSLeadByte, (LPVOID)Hook_IsDBCSLeadByteEx, NULL);

    // Hook USER32 functions  
    HookFunction((LPCSTR)CharPrevA, (LPVOID)Hook_CharPrevExA, NULL);
    HookFunction((LPCSTR)CharNextA, (LPVOID)Hook_CharNextExA, NULL);

    // Hook VERSION functions
    HookFunction((LPCSTR)VerQueryValueA, (LPVOID)Hook_VerQueryValueA, NULL);

    return 1;
}

// Check Windows version compatibility
BOOL CheckWindowsVersion()
{
#ifdef HAS_VERSION_H
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.dwMajorVersion = 5;
    osvi.dwMinorVersion = 1;
    osvi.wServicePackMajor = 2;
    osvi.wServicePackMinor = 0;

    DWORDLONG dwlConditionMask = 0;
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

    return VerifyVersionInfoA(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR, dwlConditionMask);
#else
    // Fallback version check using GetVersionEx (deprecated but works)
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    // Suppress deprecation warning for legacy compatibility
#pragma warning(push)
#pragma warning(disable: 4996)
    BOOL versionResult = GetVersionEx(&osvi);
#pragma warning(pop)

    if (versionResult)
    {
        // Check for Windows XP SP2 or later (5.1.2600+)
        return (osvi.dwMajorVersion > 5) ||
            (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1);
    }

    return TRUE; // Assume compatible if we can't determine
#endif
}

// Get locale information using simplified COM approach
int GetLocaleInfo(UINT CodePage)
{
    int result = -1;

    // Try COM interface with simplified error handling
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrInit))
    {
        LPVOID pInterface = NULL;

        HRESULT hr = CoCreateInstance(CLSID_CPDetector, NULL, CLSCTX_INPROC_SERVER,
            IID_ICodePageDetector, &pInterface);

        if (SUCCEEDED(hr) && pInterface)
        {
            __try
            {
                // Allocate buffer as in original IDA code
                BYTE buffer[0x240];
                ZeroMemory(buffer, sizeof(buffer));

                // Get vtable and call method at offset 0x10 (index 4)
                LPVOID* vtable = *(LPVOID**)pInterface;
                if (vtable && vtable[4])
                {
                    // Call the detection method
                    typedef HRESULT(__stdcall* DetectCodePageFunc)(LPVOID pThis, UINT codePage, LPVOID buffer);
                    DetectCodePageFunc pfnDetect = (DetectCodePageFunc)vtable[4];

                    hr = pfnDetect(pInterface, CodePage, buffer);
                    if (SUCCEEDED(hr))
                    {
                        // Extract result from buffer[568] as in IDA
                        result = buffer[568];
                    }
                }

                // Release the interface
                if (vtable && vtable[2])
                {
                    typedef ULONG(__stdcall* ReleaseFunc)(LPVOID pThis);
                    ReleaseFunc pfnRelease = (ReleaseFunc)vtable[2];
                    pfnRelease(pInterface);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // COM call failed, result stays -1
            }
        }

        CoUninitialize();
    }

    // Fallback to manual codepage mapping if COM failed
    if (result == -1)
    {
        switch (CodePage)
        {
        case 932:  result = 128; break; // Japanese Shift-JIS
        case 936:  result = 134; break; // Simplified Chinese GBK
        case 949:  result = 129; break; // Korean
        case 950:  result = 136; break; // Traditional Chinese Big5
        case 1252: result = 0;   break; // Western European
        case 1251: result = 204; break; // Cyrillic
        case 1250: result = 238; break; // Central European
        case 1253: result = 161; break; // Greek
        case 1254: result = 162; break; // Turkish
        case 1255: result = 177; break; // Hebrew
        case 1256: result = 178; break; // Arabic
        case 874:  result = 222; break; // Thai
        case 1258: result = 163; break; // Vietnamese
        default:   result = 1;   break; // Default charset
        }
    }

    return result;
}

// Utility functions
LPVOID MemoryCopy(LPVOID lpDest, LPCVOID lpSrc, SIZE_T cbSize)
{
    if ((DWORD_PTR)lpDest >= (DWORD_PTR)lpSrc)
    {
        // Copy backwards to handle overlapping memory
        BYTE* pDest = (BYTE*)lpDest + cbSize - 1;
        BYTE* pSrc = (BYTE*)lpSrc + cbSize - 1;
        while (cbSize--)
            *pDest-- = *pSrc--;
    }
    else
    {
        // Copy forwards
        BYTE* pDest = (BYTE*)lpDest;
        BYTE* pSrc = (BYTE*)lpSrc;
        while (cbSize--)
            *pDest++ = *pSrc++;
    }
    return lpDest;
}

LPVOID MemorySet(LPVOID lpDest, int nValue, SIZE_T cbSize)
{
    if (cbSize)
    {
        DWORD dwPattern = (BYTE)nValue * 0x01010101;
        DWORD dwCount = cbSize >> 2;

        // Set DWORDs
        DWORD* pDest = (DWORD*)lpDest;
        while (dwCount--)
            *pDest++ = dwPattern;

        // Set remaining bytes
        BYTE* pByteDest = (BYTE*)pDest;
        cbSize &= 3;
        while (cbSize--)
            *pByteDest++ = (BYTE)nValue;
    }
    return lpDest;
}