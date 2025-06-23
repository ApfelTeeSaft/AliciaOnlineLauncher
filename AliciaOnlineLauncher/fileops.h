#pragma once
#include "common.h"

// File mapping operations
FileMapping* CreateFileMapping(LPCWSTR filename);
FileMapping* CreateFileMappingFromHandle(HANDLE hFile);
void CloseFileMapping(FileMapping* mapping);
bool ValidateFileMapping(FileMapping* mapping);

// Path operations
std::wstring GetAppDataPath();
std::wstring GetTempPath();
std::wstring GetModuleDirectory();
std::wstring CreateTempFile(LPCWSTR basePath, LPCWSTR prefix, LPCWSTR filename);
std::wstring CreateTempFileName(LPCWSTR prefix, DWORD uniqueId);

// Directory operations
bool CreateDirectoryRecursive(const std::wstring& path);
bool DirectoryExists(const std::wstring& path);
bool EnsureDirectoryExists(const std::wstring& path);

// File I/O operations
HANDLE CreateFileForRead(LPCWSTR filename);
HANDLE CreateFileForWrite(LPCWSTR filename);
bool WriteDataToFile(HANDLE hFile, const void* data, DWORD size);
bool ReadDataFromFile(HANDLE hFile, void* buffer, DWORD size, DWORD* bytesRead);

// File manipulation
bool CopyFileToTemp(LPCWSTR sourceFile, std::wstring& tempFile);
bool DeleteTempFile(const std::wstring& tempFile);
DWORD GetFileSize(HANDLE hFile);
bool FileExists(LPCWSTR filename);

// Special file operations (from assembly analysis)
StringBuffer* LoadFileToStringBuffer(LPCWSTR filename);
void FreeStringBuffer(StringBuffer* buffer);
bool WriteStringBufferToFile(const StringBuffer* buffer, LPCWSTR filename);

// Module file operations
bool ExtractEmbeddedResource(HMODULE hModule, DWORD resourceId, const std::wstring& outputPath);
bool LoadModuleFromFile(const std::wstring& filePath, HMODULE* phModule);
void UnloadModule(HMODULE hModule);