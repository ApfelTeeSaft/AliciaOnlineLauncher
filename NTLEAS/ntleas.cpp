/*
 * NT Locale Emulator Advance (NTLEAS)
 * Reconstructed from assembly/pseudocode for Visual Studio 2022
 *
 * This application allows running Windows programs with specific locale settings
 * by injecting locale emulation DLLs into target processes.
 *
 * TARGET: 32-bit (x86) Windows Application
 */

#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")

 // NT API definitions
typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, * PPROCESS_BASIC_INFORMATION;

// NT API function pointer type
typedef NTSTATUS(WINAPI* NtQueryInformationProcessFunc)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
    );

// Error codes
#define NTLEAS_ERR_UNKNOWN          -10000
#define NTLEAS_ERR_MACHINE_UNSUPPORTED -10001
#define NTLEAS_ERR_MULTIPLE_INSTANCES  -10002
#define NTLEAS_ERR_FILE_NOT_FOUND     -10003
#define NTLEAS_ERR_INVALID_PE         -10004
#define NTLEAS_ERR_PROCESS_CREATE     -10005
#define NTLEAS_ERR_DLL_NOT_FOUND      -10006
#define NTLEAS_ERR_MEMORY_ALLOC       -10007
#define NTLEAS_ERR_CONNECTION_LOST    -10008
#define NTLEAS_ERR_QUERY_PROCESS      -10009

struct NTLEASConfig {
    DWORD processInfo[5];
    char dllName[32];
    DWORD imageBase;
    DWORD entryPoint;
    HANDLE eventHandle;
    HANDLE mappingHandle;
    LPVOID mappedView;
    HMODULE moduleHandle;
};

// Global variables
std::wstring g_ntleaDllName = L"\\ntlea";
WORD g_moduleType = 0x69; // Default module type

int ParseInteger(const std::wstring& str);
std::wstring QuoteString(const std::wstring& str);
std::wstring DuplicateString(const std::wstring& str);
std::wstring FormatCommandLine(const std::wstring& executable, const std::wstring& args);
void ShowError(int errorCode);
bool InitializeConfig(NTLEASConfig* config);
int ValidatePEFile(NTLEASConfig* config, const std::wstring& filePath);
int GetProcessBaseAddress(NTLEASConfig* config, HANDLE process);
bool FindNTLEADLL(std::wstring& dllPath, HMODULE moduleHandle);
void SetProcessDirectory(HANDLE process);
int InjectDLL(NTLEASConfig* config, HANDLE process);
void WaitForSignal();
int HookProcess(NTLEASConfig* config, PROCESS_INFORMATION* procInfo, bool useDebugger);
int DebugProcess(NTLEASConfig* config, PROCESS_INFORMATION* procInfo, bool suspend);

int ParseInteger(const std::wstring& str) {
    if (str.empty()) return 0;

    int sign = 1;
    size_t start = 0;

    if (str[0] == L'-') {
        sign = -1;
        start = 1;
    }

    int result = 0;
    for (size_t i = start; i < str.length(); ++i) {
        if (str[i] >= L'0' && str[i] <= L'9') {
            result = result * 10 + (str[i] - L'0');
        }
    }

    return sign * result;
}

std::wstring QuoteString(const std::wstring& str) {
    std::wstring result;
    result.reserve(str.length() + 2);

    for (wchar_t ch : str) {
        if (ch == L'\'') {
            result += L'"';
        }
        else {
            result += ch;
        }
    }

    return result;
}

std::wstring DuplicateString(const std::wstring& str) {
    return str;
}

std::wstring FormatCommandLine(const std::wstring& executable, const std::wstring& args) {
    return L"\"" + executable + L"\" " + args;
}

void ShowError(int errorCode) {
    const char* message;

    switch (errorCode) {
    case NTLEAS_ERR_QUERY_PROCESS:
        message = "Err: NTLEAS failed query process information.";
        break;
    case NTLEAS_ERR_CONNECTION_LOST:
        message = "Err: NTLEAS may be lost connection with hook process.";
        break;
    case NTLEAS_ERR_MEMORY_ALLOC:
        message = "Err: NTLEAS was failed to virtual allocate memory.";
        break;
    case NTLEAS_ERR_DLL_NOT_FOUND:
        message = "Err: NTLEAS could not find inject ntleai.dll.";
        break;
    case NTLEAS_ERR_PROCESS_CREATE:
        message = "Err: NTLEAS could not create specified process of Exe.";
        break;
    case NTLEAS_ERR_INVALID_PE:
        message = "Err: NTLEAS detect that the given is an invalid PE file.";
        break;
    case NTLEAS_ERR_FILE_NOT_FOUND:
        message = "Err: NTLEAS could not find or open specified PE file.";
        break;
    case NTLEAS_ERR_MULTIPLE_INSTANCES:
        message = "Err: NTLEAS could not startup two instances at one time.";
        break;
    case NTLEAS_ERR_MACHINE_UNSUPPORTED:
        message = "Err: NTLEAS detect that the PE Machine could not support.";
        break;
    default:
        message = "<err_unknown>!";
        break;
    }

    HWND foreground = GetForegroundWindow();
    MessageBoxA(foreground, message, "NT Locale Emulator Advance", MB_OK);
}

// Initialize configuration from shared memory or defaults
bool InitializeConfig(NTLEASConfig* config) {
    HANDLE fileMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 0x34, "RshFileMap000");

    if (GetLastError() != 0) {
        LPVOID view = MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0x34);
        if (view) {
            memcpy(config->processInfo, view, 20);
            strcpy_s(config->dllName, (char*)view + 20);
            UnmapViewOfFile(view);
        }
    }
    else {
        // Set defaults
        config->processInfo[0] = 0;
        config->processInfo[1] = 932;   // Japanese ANSI codepage
        config->processInfo[2] = 1041;  // Japanese LCID
        config->processInfo[3] = 0xFFFFFDE4; // Timezone offset
        config->processInfo[4] = 100;   // Some flag
        config->dllName[0] = 0;
    }

    CloseHandle(fileMapping);
    return true;
}

// Validate PE file and extract information
int ValidatePEFile(NTLEASConfig* config, const std::wstring& filePath) {
    DWORD binaryType;
    if (!GetBinaryTypeW(filePath.c_str(), &binaryType)) {
        return NTLEAS_ERR_INVALID_PE;
    }

    HANDLE file = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return NTLEAS_ERR_FILE_NOT_FOUND;
    }

    // Read PE header
    BYTE buffer[1024];
    DWORD bytesRead;
    if (!ReadFile(file, buffer, sizeof(buffer), &bytesRead, nullptr)) {
        CloseHandle(file);
        return NTLEAS_ERR_FILE_NOT_FOUND;
    }

    CloseHandle(file);

    // Validate PE signature
    if (*(WORD*)buffer != IMAGE_DOS_SIGNATURE) {
        return NTLEAS_ERR_FILE_NOT_FOUND;
    }

    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(buffer + ((IMAGE_DOS_HEADER*)buffer)->e_lfanew);
    WORD machine = ntHeaders->FileHeader.Machine;

    if (machine != IMAGE_FILE_MACHINE_I386 && machine != IMAGE_FILE_MACHINE_AMD64) {
        return NTLEAS_ERR_MACHINE_UNSUPPORTED;
    }

    config->imageBase = ntHeaders->OptionalHeader.ImageBase;
    config->entryPoint = ntHeaders->OptionalHeader.AddressOfEntryPoint;

    // Create synchronization objects
    config->eventHandle = CreateEventA(nullptr, FALSE, FALSE, "RcpEvent000");
    config->mappingHandle = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 4, "RcpFileMap000");
    config->mappedView = MapViewOfFile(config->mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 4);

    return (machine == IMAGE_FILE_MACHINE_I386) ? 0 : NTLEAS_ERR_MACHINE_UNSUPPORTED;
}

// Get process base address using NtQueryInformationProcess
int GetProcessBaseAddress(NTLEASConfig* config, HANDLE process) {
    HMODULE ntdll = GetModuleHandleA("ntdll");
    if (!ntdll) return NTLEAS_ERR_QUERY_PROCESS;

    NtQueryInformationProcessFunc NtQueryInformationProcess =
        (NtQueryInformationProcessFunc)GetProcAddress(ntdll, "NtQueryInformationProcess");

    if (!NtQueryInformationProcess) return NTLEAS_ERR_QUERY_PROCESS;

    PROCESS_BASIC_INFORMATION pbi = {};
    NTSTATUS status = NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);

    if (NT_SUCCESS(status)) {
        DWORD imageBase = 0;
        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(process, (BYTE*)pbi.PebBaseAddress + 8, &imageBase, sizeof(imageBase), &bytesRead)) {
            config->entryPoint += imageBase;
        }
        else {
            config->entryPoint += config->imageBase;
        }
    }
    else {
        config->entryPoint += config->imageBase;
    }

    return 0;
}

// Find NTLEA DLL in various locations
bool FindNTLEADLL(std::wstring& dllPath, HMODULE moduleHandle) {
    wchar_t path[MAX_PATH];
    WIN32_FIND_DATAW findData;

    GetWindowsDirectoryW(path, MAX_PATH);
    wcscat_s(path, g_ntleaDllName.c_str());

    HANDLE findHandle = FindFirstFileW(path, &findData);
    if (findHandle != INVALID_HANDLE_VALUE) {
        FindClose(findHandle);
        dllPath = path;
        return true;
    }

    GetCurrentDirectoryW(MAX_PATH, path);
    wcscat_s(path, g_ntleaDllName.c_str());

    findHandle = FindFirstFileW(path, &findData);
    if (findHandle != INVALID_HANDLE_VALUE) {
        FindClose(findHandle);
        dllPath = path;
        return true;
    }

    if (GetModuleFileNameW(moduleHandle, path, MAX_PATH)) {
        wchar_t* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash) {
            *lastSlash = 0;
            wcscat_s(path, g_ntleaDllName.c_str());

            findHandle = FindFirstFileW(path, &findData);
            if (findHandle != INVALID_HANDLE_VALUE) {
                FindClose(findHandle);
                dllPath = path;
                return true;
            }
        }
    }

    return false;
}

void SetProcessDirectory(HANDLE process) {
    HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
    if (!kernel32) return;

    typedef BOOL(WINAPI* QueryFullProcessImageNameWFunc)(HANDLE, DWORD, LPWSTR, PDWORD);
    QueryFullProcessImageNameWFunc QueryFullProcessImageNameW =
        (QueryFullProcessImageNameWFunc)GetProcAddress(kernel32, "QueryFullProcessImageNameW");

    if (QueryFullProcessImageNameW) {
        wchar_t imagePath[MAX_PATH];
        DWORD size = MAX_PATH;

        if (QueryFullProcessImageNameW(process, 0, imagePath, &size)) {
            wchar_t* lastSlash = wcsrchr(imagePath, L'\\');
            if (lastSlash) {
                *lastSlash = 0;
                SetCurrentDirectoryW(imagePath);
            }
        }
    }
}

int InjectDLL(NTLEASConfig* config, HANDLE process) {
    std::wstring dllPath;
    if (!FindNTLEADLL(dllPath, config->moduleHandle)) {
        return NTLEAS_ERR_DLL_NOT_FOUND;
    }

    // Prepare injection data
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    size_t dataSize = pathSize + strlen(config->dllName) + 1 + 16; // Extra space for config data

    LPVOID remoteMemory = VirtualAllocEx(process, nullptr, dataSize, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteMemory) {
        return NTLEAS_ERR_MEMORY_ALLOC;
    }

    *(DWORD*)config->mappedView = (DWORD)remoteMemory;

    std::vector<BYTE> injectionData(dataSize);
    memcpy(injectionData.data(), dllPath.c_str(), pathSize);

    char* configData = (char*)injectionData.data() + pathSize;
    strcpy_s(configData, strlen(config->dllName) + 1, config->dllName);

    DWORD* configInts = (DWORD*)(configData + strlen(config->dllName) + 1);
    memcpy(configInts, config->processInfo, 16);

    WriteProcessMemory(process, remoteMemory, injectionData.data(), dataSize, nullptr);

    DWORD threadId;
    HANDLE remoteThread = CreateRemoteThread(process, nullptr, 0,
        (LPTHREAD_START_ROUTINE)LoadLibraryW, remoteMemory, 0, &threadId);

    if (remoteThread) {
        CloseHandle(remoteThread);
        return 0;
    }

    return NTLEAS_ERR_PROCESS_CREATE;
}

// Wait for synchronization signal
void WaitForSignal() {
    HANDLE event = OpenEventW(EVENT_ALL_ACCESS, FALSE, L"ntleasig");
    if (event) {
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
}

int HookProcess(NTLEASConfig* config, PROCESS_INFORMATION* procInfo, bool useDebugger) {
    int result = GetProcessBaseAddress(config, procInfo->hProcess);
    if (result < 0) return result;

    BYTE originalBytes[2];
    ReadProcessMemory(procInfo->hProcess, (LPVOID)config->entryPoint, originalBytes, 2, nullptr);

    // Write breakpoint
    BYTE breakpoint[] = { 0xEB, 0xFE }; // JMP $-2 (infinite loop)
    WriteProcessMemory(procInfo->hProcess, (LPVOID)config->entryPoint, breakpoint, 2, nullptr);
    FlushInstructionCache(procInfo->hProcess, (LPVOID)config->entryPoint, 2);

    ResumeThread(procInfo->hThread);
    Sleep(32);
    SuspendThread(procInfo->hThread);

    CONTEXT context;
    do {
        context.ContextFlags = CONTEXT_CONTROL;
        if (!GetThreadContext(procInfo->hThread, &context)) {
            return NTLEAS_ERR_CONNECTION_LOST;
        }

        if (context.Eip != config->entryPoint) {
            ResumeThread(procInfo->hThread);
            Sleep(32);
            SuspendThread(procInfo->hThread);
        }
    } while (context.Eip != config->entryPoint);

    if (!config->moduleHandle) {
        WaitForSignal();
    }

    result = InjectDLL(config, procInfo->hProcess);
    if (result < 0) return result;

    WaitForSingleObject(config->eventHandle, 30000);

    if (!config->moduleHandle) {
        WaitForSignal();
    }

    WriteProcessMemory(procInfo->hProcess, (LPVOID)config->entryPoint, originalBytes, 2, nullptr);
    FlushInstructionCache(procInfo->hProcess, (LPVOID)config->entryPoint, 2);

    if (!useDebugger) {
        ResumeThread(procInfo->hThread);
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    HANDLE mutex = CreateMutexA(nullptr, FALSE, "RcpInternalMutex");
    if (GetLastError() != 0) {
        ShowError(NTLEAS_ERR_MULTIPLE_INSTANCES);
        return NTLEAS_ERR_MULTIPLE_INSTANCES;
    }

    NTLEASConfig config = {};
    std::wstring targetFile;
    std::wstring commandLine;
    bool useDebugger = false;
    bool showErrors = true;
    bool setDirectory = false;

    InitializeConfig(&config);

    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv && argc > 1) {
        targetFile = argv[1];

        for (int i = 2; i < argc; ++i) {
            std::wstring arg = argv[i];
            if (arg.empty()) continue;

            wchar_t firstChar = arg[0];
            std::wstring value = arg.substr(1);

            switch (firstChar) {
            case L'A': // Application parameter
                commandLine = FormatCommandLine(argv[1], QuoteString(value));
                break;
            case L'C': // Code page
                config.processInfo[0] = ParseInteger(value);
                break;
            case L'D': // Debug mode
                useDebugger = true;
                break;
            case L'E': // Error handling
                showErrors = (ParseInteger(value) != 0);
                break;
            case L'F': // 
                WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, config.dllName, sizeof(config.dllName), nullptr, nullptr);
                break;
            case L'I': // 
                config.processInfo[4] = ParseInteger(value);
                break;
            case L'L': // Locale ID
                config.processInfo[1] = ParseInteger(value);
                break;
            case L'M': // Module type
                g_moduleType = (WORD)ParseInteger(value);
                break;
            case L'P': // Process flag
                config.processInfo[2] = ParseInteger(value);
                break;
            case L'Q': // 
                config.processInfo[3] = ParseInteger(value);
                break;
            case L'R': // 
                config.processInfo[4] = ParseInteger(value);
                break;
            case L'S': // 
                config.processInfo[5] = ParseInteger(value);
                break;
            case L'T': // 
                setDirectory = true;
                break;
            case L'V': // 
                setDirectory = (ParseInteger(value) != 0);
                break;
            }
        }
    }
    else {
        OPENFILENAMEW ofn = {};
        wchar_t fileName[MAX_PATH] = {};

        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = L"Executable File(*.exe)\0*.exe\0All Files(*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (!GetOpenFileNameW(&ofn)) {
            return 0;
        }

        targetFile = fileName;
    }

    int validationResult = ValidatePEFile(&config, targetFile);
    if (validationResult == NTLEAS_ERR_INVALID_PE) {
        if (commandLine.empty()) {
            wchar_t associatedExe[MAX_PATH];
            if (FindExecutableW(targetFile.c_str(), nullptr, associatedExe) > (HINSTANCE)32) {
                DWORD size = 520;
                wchar_t commandBuffer[520];

                if (AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, associatedExe, nullptr, commandBuffer, &size) == S_OK) {
                    wcscpy_s(commandBuffer, associatedExe);
                }

                size_t len = wcslen(targetFile.c_str());
                LPWSTR quotedFile = (LPWSTR)LocalAlloc(LMEM_FIXED, (len + 3) * sizeof(wchar_t));
                swprintf_s(quotedFile, len + 3, L"\"%s\"", targetFile.c_str());

                commandLine = FormatCommandLine(commandBuffer, quotedFile);
                LocalFree(quotedFile);

                targetFile = commandBuffer;
                validationResult = ValidatePEFile(&config, targetFile);
            }
        }
    }

    if (validationResult < 0) {
        if (showErrors) {
            ShowError(validationResult);
        }
        return validationResult;
    }

    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

    DWORD creationFlags = CREATE_SUSPENDED;
    if (useDebugger) {
        creationFlags = DEBUG_ONLY_THIS_PROCESS;
    }

    std::wstring cmdLine = commandLine.empty() ? targetFile : commandLine;

    if (!CreateProcessW(targetFile.c_str(),
        const_cast<LPWSTR>(cmdLine.c_str()),
        nullptr, nullptr, FALSE, creationFlags,
        nullptr, nullptr, &si, &pi)) {
        return -1;
    }

    if (setDirectory) {
        SetProcessDirectory(pi.hProcess);
    }

    int hookResult;
    if (useDebugger) {
        hookResult = DebugProcess(&config, &pi, false);
    }
    else {
        hookResult = HookProcess(&config, &pi, false);
    }

    CloseHandle(pi.hThread);

    if (hookResult >= 0) {
        CloseHandle(pi.hProcess);
        return 0;
    }
    else {
        TerminateProcess(pi.hProcess, hookResult);
        if (showErrors) {
            ShowError(hookResult);
        }
        return hookResult;
    }
}

// Debug process implementation
int DebugProcess(NTLEASConfig* config, PROCESS_INFORMATION* procInfo, bool suspend) {
    // This would implement the full debug event loop from the original code
    // TODO: ADD
    return HookProcess(config, procInfo, suspend);
}