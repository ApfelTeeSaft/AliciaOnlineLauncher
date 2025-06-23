#pragma once
#include "common.h"

// Custom memory allocation (matching assembly signatures)
void* AllocateMemory(int width, int height, int unused);
void FreeMemory(void* block);

// Memory utility functions
void* AllocateAndZeroMemory(size_t size);
void* ReallocateMemory(void* ptr, size_t newSize);
void SecureZeroMemory(void* ptr, size_t size);

// Memory block operations
void* CopyMemoryBlock(const void* source, size_t size);
bool CompareMemoryBlocks(const void* block1, const void* block2, size_t size);
void FillMemoryBlock(void* block, BYTE value, size_t size);

// String buffer management
StringBuffer* AllocateStringBuffer(DWORD length);
void FreeStringBuffer(StringBuffer* buffer);
StringBuffer* DuplicateStringBuffer(const StringBuffer* source);
bool ResizeStringBuffer(StringBuffer* buffer, DWORD newLength);

// Memory debugging and validation
bool ValidateMemoryBlock(const void* ptr, size_t expectedSize);
void DumpMemoryBlock(const void* ptr, size_t size, const char* label);

#ifdef _DEBUG
#define SAFE_ALLOC(size) AllocateAndZeroMemory(size)
#define SAFE_FREE_EX(ptr) { FreeMemory(ptr); ptr = nullptr; }
#else
#define SAFE_ALLOC(size) malloc(size)
#define SAFE_FREE_EX(ptr) { free(ptr); ptr = nullptr; }
#endif