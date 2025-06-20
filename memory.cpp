#include "common.h"
#include "memory.h"

// Custom memory allocation (matches assembly at 0x401000)
void* AllocateMemory(int width, int height, int unused)
{
    size_t size = static_cast<size_t>(width) * static_cast<size_t>(height);

    if (size == 0) {
        return nullptr;
    }

    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

// Free memory (matches assembly at 0x401024)
void FreeMemory(void* block)
{
    if (block) {
        free(block);
    }
}

void* AllocateAndZeroMemory(size_t size)
{
    if (size == 0) {
        return nullptr;
    }

    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void* ReallocateMemory(void* ptr, size_t newSize)
{
    if (newSize == 0) {
        FreeMemory(ptr);
        return nullptr;
    }

    return realloc(ptr, newSize);
}

void SecureZeroMemory(void* ptr, size_t size)
{
    if (!ptr || size == 0) return;

    volatile BYTE* bytes = static_cast<volatile BYTE*>(ptr);
    for (size_t i = 0; i < size; i++) {
        bytes[i] = 0;
    }
}

void* CopyMemoryBlock(const void* source, size_t size)
{
    if (!source || size == 0) {
        return nullptr;
    }

    void* dest = malloc(size);
    if (dest) {
        memcpy(dest, source, size);
    }

    return dest;
}

bool CompareMemoryBlocks(const void* block1, const void* block2, size_t size)
{
    if (!block1 || !block2) {
        return (block1 == block2);
    }

    return (memcmp(block1, block2, size) == 0);
}

void FillMemoryBlock(void* block, BYTE value, size_t size)
{
    if (block && size > 0) {
        memset(block, value, size);
    }
}

// Allocate string buffer (matches assembly string buffer pattern)
StringBuffer* AllocateStringBuffer(DWORD length)
{
    if (length == 0) {
        return nullptr;
    }

    StringBuffer* buffer = new StringBuffer();
    if (!buffer) {
        return nullptr;
    }

    // Calculate required size (matches assembly size calculation)
    DWORD requiredSize = length;

    DWORD allocSize = 16;
    while (allocSize < requiredSize) {
        allocSize <<= 1;
    }

    buffer->buffer = malloc(allocSize);
    if (!buffer->buffer) {
        delete buffer;
        return nullptr;
    }

    memset(buffer->buffer, 0, allocSize);
    buffer->length = length;

    return buffer;
}

void FreeStringBuffer(StringBuffer* buffer)
{
    if (!buffer) return;

    if (buffer->buffer) {
        DWORD allocSize = 16;
        while (allocSize < buffer->length) {
            allocSize <<= 1;
        }

        SecureZeroMemory(buffer->buffer, allocSize);
        free(buffer->buffer);
        buffer->buffer = nullptr;
    }

    buffer->length = 0;
    delete buffer;
}

StringBuffer* DuplicateStringBuffer(const StringBuffer* source)
{
    if (!source || !source->buffer || source->length == 0) {
        return nullptr;
    }

    StringBuffer* copy = AllocateStringBuffer(source->length);
    if (copy) {
        memcpy(copy->buffer, source->buffer, source->length);
    }

    return copy;
}

bool ResizeStringBuffer(StringBuffer* buffer, DWORD newLength)
{
    if (!buffer) return false;

    if (newLength == 0) {
        FreeStringBuffer(buffer);
        return true;
    }

    DWORD newAllocSize = 16;
    while (newAllocSize < newLength) {
        newAllocSize <<= 1;
    }

    DWORD currentAllocSize = 16;
    while (currentAllocSize < buffer->length) {
        currentAllocSize <<= 1;
    }

    if (newAllocSize != currentAllocSize) {
        void* newBuffer = realloc(buffer->buffer, newAllocSize);
        if (!newBuffer) {
            return false;
        }

        buffer->buffer = newBuffer;

        if (newLength > buffer->length) {
            BYTE* bytes = static_cast<BYTE*>(buffer->buffer);
            memset(bytes + buffer->length, 0, newLength - buffer->length);
        }
    }

    buffer->length = newLength;
    return true;
}

bool ValidateMemoryBlock(const void* ptr, size_t expectedSize)
{
    if (!ptr || expectedSize == 0) {
        return false;
    }

    __try {
        volatile const BYTE* bytes = static_cast<volatile const BYTE*>(ptr);
        volatile BYTE first = bytes[0];
        volatile BYTE last = bytes[expectedSize - 1];
        (void)first; (void)last;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Dump memory block (for debugging)
void DumpMemoryBlock(const void* ptr, size_t size, const char* label)
{
#ifdef _DEBUG
    if (!ptr || size == 0) return;

    const BYTE* bytes = static_cast<const BYTE*>(ptr);

    OutputDebugStringA(label ? label : "Memory Dump");
    OutputDebugStringA(":\n");

    char buffer[128];
    for (size_t i = 0; i < size; i += 16) {
        // Format hex bytes
        char hexPart[64] = { 0 };
        char asciiPart[32] = { 0 };

        for (size_t j = 0; j < 16 && (i + j) < size; j++) {
            BYTE b = bytes[i + j];
            sprintf_s(hexPart + j * 3, 4, "%02X ", b);
            asciiPart[j] = (b >= 32 && b < 127) ? b : '.';
        }

        sprintf_s(buffer, "%08X: %-48s %s\n",
            static_cast<unsigned int>(i), hexPart, asciiPart);
        OutputDebugStringA(buffer);
    }

    OutputDebugStringA("\n");
#else
    (void)ptr; (void)size; (void)label;
#endif
}