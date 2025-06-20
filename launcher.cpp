#include "common.h"
#include "launcher.h"
#include "encryption.h"
#include "fileops.h"
#include "registry.h"
#include "memory.h"

// Main launcher logic (corresponds to assembly function around 0x402A7A)
int MainLauncherLogic(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd)
{
    if (!SetupEnvironmentVariables()) {
        return -1;
    }

    std::wstring appDataPath = GetAppDataPath();
    if (appDataPath.empty()) {
        return -1;
    }

    appDataPath += L"\\AliciaOnline";

    if (!CheckUpdateRequirement()) {
        if (LaunchUpdateProcess()) {
            return 0;
        }
    }

    return LoadAndExecuteEmbeddedModule();
}

// Setup environment variables (matches assembly around 0x402B18)
bool SetupEnvironmentVariables()
{
    BOOL result = ::SetEnvironmentVariableW(L"AliciaOnlineShortcut", L"Launcher.exe");
    return (result != FALSE);
}

// Check if update is required (matches assembly logic)
bool CheckUpdateRequirement()
{
    DWORD updateFlag = ReadUpdateFlag();
    return (updateFlag != 0);
}

// Launch update process (matches assembly around 0x402B94)
bool LaunchUpdateProcess()
{
    std::wstring appDataPath = GetAppDataPath();
    if (appDataPath.empty()) {
        return false;
    }

    std::wstring launcherPath = appDataPath + L"\\AliciaOnline";
    std::wstring updateExe = L"Install\\Update.exe";

    // Setup shell execute info (matches SHELLEXECUTEINFOW structure in assembly)
    SHELLEXECUTEINFOW execInfo = { 0 };
    execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.lpVerb = L"open";
    execInfo.lpFile = updateExe.c_str();
    execInfo.lpDirectory = launcherPath.c_str();
    execInfo.nShow = SW_SHOW;

    BOOL result = ShellExecuteExW(&execInfo);

    if (result && execInfo.hProcess) {
        CloseHandle(execInfo.hProcess);
        return true;
    }

    return false;
}

// Load and execute embedded module (matches assembly around 0x40287D)
DWORD LoadAndExecuteEmbeddedModule()
{
    HMODULE hCurrentModule = GetModuleHandle(NULL);
    if (!hCurrentModule) {
        return 0;
    }

    std::wstring modulePath = GetModulePath(hCurrentModule);
    if (modulePath.empty()) {
        return 0;
    }

    std::wstring tempFile = CreateTempFile(NULL, L"ml", L"2715");
    if (tempFile.empty()) {
        return 0;
    }

    FileMapping* mapping = CreateFileMapping(modulePath.c_str());
    if (!mapping) {
        return 0;
    }

    DWORD result = ProcessModuleData(mapping->data, mapping->size);

    CloseFileMapping(mapping);
    DeleteTempFile(tempFile);

    return result;
}

// Get module checksum (matches assembly function at 0x4010FF)
DWORD GetModuleChecksum(HMODULE hModule)
{
    wchar_t modulePath[MAX_PATH];
    if (!GetModuleFileNameW(hModule, modulePath, MAX_PATH)) {
        return 0;
    }

    FileMapping* mapping = CreateFileMapping(modulePath);
    if (!mapping) {
        return 0;
    }

    DWORD checksum = 0;
    DWORD secondaryChecksum = 0;
    BYTE* data = static_cast<BYTE*>(mapping->data);
    DWORD size = mapping->size;

    // Process data looking for signature (matches assembly logic)
    for (DWORD i = 8; i < size; ) {
        if (i + 8 <= size) {
            DWORD* dwordPtr = reinterpret_cast<DWORD*>(&data[i]);
            DWORD signature = *dwordPtr ^ SIGNATURE_XOR;

            if (signature == SIGNATURE_CHECK) {
                secondaryChecksum = *(dwordPtr + 1);
                i += 8;
                continue;
            }
        }

        checksum = UpdateCrc32(checksum, data[i]);
        i++;
    }

    CloseFileMapping(mapping);

    return checksum ^ secondaryChecksum;
}

// Process module data (decrypt and validate)
DWORD ProcessModuleData(void* moduleData, DWORD dataSize)
{
    if (!moduleData || dataSize == 0) {
        return 0;
    }

    DWORD checksum = CalculateModuleChecksum(moduleData, dataSize);

    return checksum;
}

std::wstring GetModulePath(HMODULE hModule)
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(hModule, path, MAX_PATH)) {
        return std::wstring(path);
    }
    return L"";
}

bool ValidateModuleIntegrity(HMODULE hModule)
{
    DWORD checksum = GetModuleChecksum(hModule);
    return (checksum != 0);
}

std::wstring BuildLauncherPath()
{
    std::wstring appData = GetAppDataPath();
    if (!appData.empty()) {
        return appData + L"\\AliciaOnline\\Launcher.exe";
    }
    return L"";
}

DWORD HandleUpdateMode()
{
    if (!LaunchUpdateProcess()) {
        return LoadAndExecuteEmbeddedModule();
    }
    return 0;
}