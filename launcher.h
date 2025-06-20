#pragma once
#include "common.h"

// Main entry points
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd);

// Core launcher functions
int MainLauncherLogic(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd);
DWORD GetModuleChecksum(HMODULE hModule);
DWORD LoadAndExecuteEmbeddedModule();
bool LaunchUpdateProcess();

// Module processing functions
DWORD ProcessModuleData(void* moduleData, DWORD dataSize);
bool ValidateModuleIntegrity(HMODULE hModule);
void* DecryptModuleSection(void* data, DWORD size, DWORD key);

// Path and environment setup
bool SetupEnvironmentVariables();
std::wstring BuildLauncherPath();
std::wstring GetModulePath(HMODULE hModule);

// Update and installation logic
bool CheckUpdateRequirement();
bool ExecuteInstallation(const std::wstring& setupPath);
DWORD HandleUpdateMode();