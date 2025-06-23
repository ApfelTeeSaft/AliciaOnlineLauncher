#pragma once
#include "common.h"

// Registry key operations
LSTATUS OpenRegistryKey(LPCWSTR keyPath, REGSAM access, HKEY* phKey);
LSTATUS CreateRegistryKey(LPCWSTR keyPath, HKEY* phKey);
void CloseRegistryKey(HKEY hKey);

// Registry value operations
DWORD ReadRegistryDWord(LPCWSTR keyPath, LPCWSTR valueName, DWORD defaultValue = 0);
bool WriteRegistryDWord(LPCWSTR keyPath, LPCWSTR valueName, DWORD value);
std::wstring ReadRegistryString(LPCWSTR keyPath, LPCWSTR valueName, LPCWSTR defaultValue = L"");
bool WriteRegistryString(LPCWSTR keyPath, LPCWSTR valueName, LPCWSTR value);

// Application-specific registry functions
DWORD ReadUpdateFlag();
bool WriteUpdateFlag(DWORD value);
bool CheckRegistryForUpdates();

// Registry path helpers
std::wstring BuildHKCUPath(LPCWSTR subPath);
std::wstring BuildHKLMPath(LPCWSTR subPath);
bool TryReadFromHKCU(LPCWSTR subPath, LPCWSTR valueName, DWORD* pValue);
bool TryReadFromHKLM(LPCWSTR subPath, LPCWSTR valueName, DWORD* pValue);

// Environment variable integration
bool SetEnvironmentFromRegistry();
std::wstring GetEnvironmentVariableString(LPCWSTR varName);
bool SetEnvironmentVariableString(LPCWSTR varName, LPCWSTR value);