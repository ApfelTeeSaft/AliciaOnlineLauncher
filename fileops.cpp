#include "common.h"
#include "fileops.h"
#include "memory.h"

// Create file mapping (matches assembly at 0x402255)
FileMapping* CreateFileMapping(LPCWSTR filename)
{
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    return CreateFileMappingFromHandle(hFile);
}

FileMapping* CreateFileMappingFromHandle(HANDLE hFile)
{
    if (hFile == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return nullptr;
    }

    HANDLE hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        CloseHandle(hFile);
        return nullptr;
    }

    void* pData = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!pData) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return nullptr;
    }

    FileMapping* mapping = new FileMapping();
    mapping->data = pData;
    mapping->size = fileSize.LowPart;
    mapping->fileHandle = hFile;
    mapping->mappingHandle = hMapping;

    return mapping;
}

// Close file mapping (matches assembly at 0x4022EF)
void CloseFileMapping(FileMapping* mapping)
{
    if (!mapping) return;

    bool success = true;

    if (mapping->data) {
        if (!UnmapViewOfFile(mapping->data)) {
            success = false;
        }
        mapping->data = nullptr;
    }

    if (mapping->mappingHandle) {
        if (!CloseHandle(mapping->mappingHandle)) {
            success = false;
        }
        mapping->mappingHandle = nullptr;
    }

    if (mapping->fileHandle) {
        if (!CloseHandle(mapping->fileHandle)) {
            success = false;
        }
        mapping->fileHandle = nullptr;
    }

    delete mapping;
}

bool ValidateFileMapping(FileMapping* mapping)
{
    return (mapping && mapping->data && mapping->size > 0);
}

// Get AppData path (matches assembly at 0x402679)
std::wstring GetAppDataPath()
{
    wchar_t path[MAX_PATH];
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path);

    if (SUCCEEDED(hr)) {
        wchar_t shortPath[MAX_PATH];
        if (GetShortPathNameW(path, shortPath, MAX_PATH)) {
            return std::wstring(shortPath);
        }
        return std::wstring(path);
    }

    return L"";
}

std::wstring GetTempPath()
{
    wchar_t tempPath[MAX_PATH];
    DWORD result = GetTempPathW(MAX_PATH, tempPath);

    if (result > 0 && result < MAX_PATH) {
        return std::wstring(tempPath);
    }

    return L"";
}

std::wstring GetModuleDirectory()
{
    wchar_t modulePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, modulePath, MAX_PATH)) {
        std::wstring path(modulePath);
        size_t lastSlash = path.find_last_of(L'\\');
        if (lastSlash != std::wstring::npos) {
            return path.substr(0, lastSlash);
        }
    }
    return L"";
}

// Create temp file (matches assembly at 0x401602)
std::wstring CreateTempFile(LPCWSTR basePath, LPCWSTR prefix, LPCWSTR filename)
{
    wchar_t tempPath[MAX_PATH];
    wchar_t tempFile[MAX_PATH];

    if (!basePath) {
        if (!GetTempPathW(MAX_PATH, tempPath)) {
            return L"";
        }
        basePath = tempPath;
    }

    if (filename && wcslen(filename) > 0) {
        swprintf_s(tempFile, L"%s%s%s.tmp", basePath, prefix ? prefix : L"", filename);
    }
    else {
        if (!GetTempFileNameW(basePath, prefix ? prefix : L"tmp", 0, tempFile)) {
            return L"";
        }
    }

    return std::wstring(tempFile);
}

std::wstring CreateTempFileName(LPCWSTR prefix, DWORD uniqueId)
{
    std::wstring tempPath = GetTempPath();
    if (tempPath.empty()) {
        return L"";
    }

    wchar_t filename[MAX_PATH];
    swprintf_s(filename, L"%s%s_%08X.tmp", tempPath.c_str(),
        prefix ? prefix : L"launcher", uniqueId);

    return std::wstring(filename);
}

// Create directory recursively (matches assembly at 0x401702)
bool CreateDirectoryRecursive(const std::wstring& path)
{
    if (path.empty()) return false;

    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    size_t lastSlash = path.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos && lastSlash > 0) {
        std::wstring parent = path.substr(0, lastSlash);

        // Skip drive letters (C:)
        if (!(parent.length() == 2 && parent[1] == L':')) {
            if (!CreateDirectoryRecursive(parent)) {
                return false;
            }
        }
    }

    return CreateDirectoryW(path.c_str(), NULL) != FALSE;
}

bool DirectoryExists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool EnsureDirectoryExists(const std::wstring& path)
{
    if (DirectoryExists(path)) {
        return true;
    }
    return CreateDirectoryRecursive(path);
}

HANDLE CreateFileForRead(LPCWSTR filename)
{
    return CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

HANDLE CreateFileForWrite(LPCWSTR filename)
{
    return CreateFileW(filename, GENERIC_WRITE, 0,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

// Write data to file (matches assembly at 0x4017E9)
bool WriteDataToFile(HANDLE hFile, const void* data, DWORD size)
{
    if (hFile == INVALID_HANDLE_VALUE || !data || size == 0) {
        return false;
    }

    const BYTE* bytes = static_cast<const BYTE*>(data);
    DWORD totalWritten = 0;

    while (totalWritten < size) {
        DWORD chunkSize = min(size - totalWritten, 0x10000); // 64KB chunks
        DWORD bytesWritten = 0;

        if (!WriteFile(hFile, bytes + totalWritten, chunkSize, &bytesWritten, NULL)) {
            return false;
        }

        if (bytesWritten != chunkSize) {
            return false;
        }

        totalWritten += bytesWritten;
    }

    return true;
}

bool ReadDataFromFile(HANDLE hFile, void* buffer, DWORD size, DWORD* bytesRead)
{
    if (hFile == INVALID_HANDLE_VALUE || !buffer || size == 0) {
        return false;
    }

    return ReadFile(hFile, buffer, size, bytesRead, NULL) != FALSE;
}

bool CopyFileToTemp(LPCWSTR sourceFile, std::wstring& tempFile)
{
    tempFile = CreateTempFile(NULL, L"launcher", NULL);
    if (tempFile.empty()) {
        return false;
    }

    return CopyFileW(sourceFile, tempFile.c_str(), FALSE) != FALSE;
}

bool DeleteTempFile(const std::wstring& tempFile)
{
    if (tempFile.empty()) {
        return true;
    }

    return DeleteFileW(tempFile.c_str()) != FALSE;
}

DWORD GetFileSize(HANDLE hFile)
{
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(hFile, &fileSize)) {
        return fileSize.LowPart;
    }
    return 0;
}

bool FileExists(LPCWSTR filename)
{
    DWORD attributes = GetFileAttributesW(filename);
    return (attributes != INVALID_FILE_ATTRIBUTES &&
        !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

StringBuffer* LoadFileToStringBuffer(LPCWSTR filename)
{
    HANDLE hFile = CreateFileForRead(filename);
    if (hFile == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    DWORD fileSize = GetFileSize(hFile);
    if (fileSize == 0) {
        CloseHandle(hFile);
        return nullptr;
    }

    StringBuffer* buffer = AllocateStringBuffer(fileSize);
    if (!buffer) {
        CloseHandle(hFile);
        return nullptr;
    }

    DWORD bytesRead = 0;
    bool success = ReadDataFromFile(hFile, buffer->buffer, fileSize, &bytesRead);

    CloseHandle(hFile);

    if (!success || bytesRead != fileSize) {
        FreeStringBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

bool WriteStringBufferToFile(const StringBuffer* buffer, LPCWSTR filename)
{
    if (!buffer || !buffer->buffer || buffer->length == 0) {
        return false;
    }

    HANDLE hFile = CreateFileForWrite(filename);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool success = WriteDataToFile(hFile, buffer->buffer, buffer->length);
    CloseHandle(hFile);

    return success;
}

bool ExtractEmbeddedResource(HMODULE hModule, DWORD resourceId, const std::wstring& outputPath)
{
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) {
        return false;
    }

    HGLOBAL hGlobal = LoadResource(hModule, hResource);
    if (!hGlobal) {
        return false;
    }

    void* pData = LockResource(hGlobal);
    DWORD dataSize = SizeofResource(hModule, hResource);

    if (!pData || dataSize == 0) {
        return false;
    }

    HANDLE hFile = CreateFileForWrite(outputPath.c_str());
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool success = WriteDataToFile(hFile, pData, dataSize);
    CloseHandle(hFile);

    return success;
}

bool LoadModuleFromFile(const std::wstring& filePath, HMODULE* phModule)
{
    if (!phModule) return false;

    *phModule = LoadLibraryW(filePath.c_str());
    return (*phModule != NULL);
}

void UnloadModule(HMODULE hModule)
{
    if (hModule) {
        FreeLibrary(hModule);
    }
}