#include "common.h"
#include "registry.h"

// Open registry key (matches assembly at 0x40270A)
LSTATUS OpenRegistryKey(LPCWSTR keyPath, REGSAM access, HKEY* phKey)
{
    if (!keyPath || !phKey) {
        return ERROR_INVALID_PARAMETER;
    }

    HKEY rootKey = HKEY_CURRENT_USER;
    std::wstring subKey = keyPath;

    // Check for HKCU or HKLM prefix (matches assembly logic)
    if (wcsncmp(keyPath, L"HKCU\\", 5) == 0) {
        rootKey = HKEY_CURRENT_USER;
        subKey = keyPath + 5;
    }
    else if (wcsncmp(keyPath, L"HKLM\\", 5) == 0) {
        rootKey = HKEY_LOCAL_MACHINE;
        subKey = keyPath + 5;
    }

    return RegOpenKeyExW(rootKey, subKey.c_str(), 0, access, phKey);
}

LSTATUS CreateRegistryKey(LPCWSTR keyPath, HKEY* phKey)
{
    if (!keyPath || !phKey) {
        return ERROR_INVALID_PARAMETER;
    }

    HKEY rootKey = HKEY_CURRENT_USER;
    std::wstring subKey = keyPath;

    // Parse root key prefix
    if (wcsncmp(keyPath, L"HKCU\\", 5) == 0) {
        rootKey = HKEY_CURRENT_USER;
        subKey = keyPath + 5;
    }
    else if (wcsncmp(keyPath, L"HKLM\\", 5) == 0) {
        rootKey = HKEY_LOCAL_MACHINE;
        subKey = keyPath + 5;
    }

    DWORD disposition;
    return RegCreateKeyExW(rootKey, subKey.c_str(), 0, NULL, 0,
        KEY_ALL_ACCESS, NULL, phKey, &disposition);
}

void CloseRegistryKey(HKEY hKey)
{
    if (hKey && hKey != HKEY_CURRENT_USER && hKey != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKey);
    }
}

// Read DWORD from registry (matches assembly at 0x402826)
DWORD ReadRegistryDWord(LPCWSTR keyPath, LPCWSTR valueName, DWORD defaultValue)
{
    HKEY hKey;
    LSTATUS result = OpenRegistryKey(keyPath, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        return defaultValue;
    }

    DWORD value = defaultValue;
    DWORD valueSize = sizeof(value);
    DWORD valueType;

    result = RegQueryValueExW(hKey, valueName, NULL, &valueType,
        reinterpret_cast<LPBYTE>(&value), &valueSize);

    CloseRegistryKey(hKey);

    if (result == ERROR_SUCCESS && valueType == REG_DWORD) {
        return value;
    }

    return defaultValue;
}

bool WriteRegistryDWord(LPCWSTR keyPath, LPCWSTR valueName, DWORD value)
{
    HKEY hKey;
    LSTATUS result = CreateRegistryKey(keyPath, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegSetValueExW(hKey, valueName, 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&value), sizeof(value));

    CloseRegistryKey(hKey);

    return (result == ERROR_SUCCESS);
}

std::wstring ReadRegistryString(LPCWSTR keyPath, LPCWSTR valueName, LPCWSTR defaultValue)
{
    HKEY hKey;
    LSTATUS result = OpenRegistryKey(keyPath, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        return defaultValue ? defaultValue : L"";
    }

    DWORD valueSize = 0;
    DWORD valueType;

    result = RegQueryValueExW(hKey, valueName, NULL, &valueType, NULL, &valueSize);

    if (result != ERROR_SUCCESS || valueType != REG_SZ || valueSize == 0) {
        CloseRegistryKey(hKey);
        return defaultValue ? defaultValue : L"";
    }

    std::vector<wchar_t> buffer(valueSize / sizeof(wchar_t));
    result = RegQueryValueExW(hKey, valueName, NULL, &valueType,
        reinterpret_cast<LPBYTE>(buffer.data()), &valueSize);

    CloseRegistryKey(hKey);

    if (result == ERROR_SUCCESS) {
        return std::wstring(buffer.data());
    }

    return defaultValue ? defaultValue : L"";
}

bool WriteRegistryString(LPCWSTR keyPath, LPCWSTR valueName, LPCWSTR value)
{
    if (!value) return false;

    HKEY hKey;
    LSTATUS result = CreateRegistryKey(keyPath, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD valueSize = (wcslen(value) + 1) * sizeof(wchar_t);
    result = RegSetValueExW(hKey, valueName, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value), valueSize);

    CloseRegistryKey(hKey);

    return (result == ERROR_SUCCESS);
}

DWORD ReadUpdateFlag()
{
    // Try HKCU first, then HKLM (matches assembly logic)
    std::wstring keyPath = L"HKCU\\SOFTWARE\\";
    keyPath += ALICIA_REGISTRY_PATH;

    DWORD value = ReadRegistryDWord(keyPath.c_str(), L"Update", 0);

    if (value == 0) {
        keyPath = L"HKLM\\SOFTWARE\\";
        keyPath += ALICIA_REGISTRY_PATH;
        value = ReadRegistryDWord(keyPath.c_str(), L"Update", 0);
    }

    return value;
}

bool WriteUpdateFlag(DWORD value)
{
    std::wstring keyPath = L"HKCU\\SOFTWARE\\";
    keyPath += ALICIA_REGISTRY_PATH;

    return WriteRegistryDWord(keyPath.c_str(), L"Update", value);
}

bool CheckRegistryForUpdates()
{
    DWORD updateFlag = ReadUpdateFlag();
    return (updateFlag != 0);
}

std::wstring BuildHKCUPath(LPCWSTR subPath)
{
    std::wstring result = L"HKCU\\";
    if (subPath) {
        result += subPath;
    }
    return result;
}

std::wstring BuildHKLMPath(LPCWSTR subPath)
{
    std::wstring result = L"HKLM\\";
    if (subPath) {
        result += subPath;
    }
    return result;
}

bool TryReadFromHKCU(LPCWSTR subPath, LPCWSTR valueName, DWORD* pValue)
{
    if (!pValue) return false;

    std::wstring keyPath = BuildHKCUPath(subPath);
    DWORD value = ReadRegistryDWord(keyPath.c_str(), valueName, 0xFFFFFFFF);

    if (value != 0xFFFFFFFF) {
        *pValue = value;
        return true;
    }

    return false;
}

bool TryReadFromHKLM(LPCWSTR subPath, LPCWSTR valueName, DWORD* pValue)
{
    if (!pValue) return false;

    std::wstring keyPath = BuildHKLMPath(subPath);
    DWORD value = ReadRegistryDWord(keyPath.c_str(), valueName, 0xFFFFFFFF);

    if (value != 0xFFFFFFFF) {
        *pValue = value;
        return true;
    }

    return false;
}

bool SetEnvironmentFromRegistry()
{
    std::wstring launcherPath = ReadRegistryString(
        L"HKCU\\SOFTWARE\\AliciaOnlineShortcut",
        L"LauncherPath",
        L"Launcher.exe"
    );

    BOOL result = ::SetEnvironmentVariableW(L"AliciaOnlineShortcut", launcherPath.c_str());
    return (result != FALSE);
}

std::wstring GetEnvironmentVariableString(LPCWSTR varName)
{
    DWORD size = ::GetEnvironmentVariableW(varName, NULL, 0);
    if (size == 0) {
        return L"";
    }

    std::vector<wchar_t> buffer(size);
    DWORD actualSize = ::GetEnvironmentVariableW(varName, buffer.data(), size);

    if (actualSize > 0 && actualSize < size) {
        return std::wstring(buffer.data());
    }

    return L"";
}

bool SetEnvironmentVariableString(LPCWSTR varName, LPCWSTR value)
{
    BOOL result = ::SetEnvironmentVariableW(varName, value);
    return (result != FALSE);
}