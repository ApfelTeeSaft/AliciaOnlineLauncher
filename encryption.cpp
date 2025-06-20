#include "common.h"
#include "encryption.h"

extern DWORD g_crcTable[256];
extern bool g_crcTableInitialized;

// Initialize CRC32 table (matches assembly at 0x401847)
void InitializeCrcTable()
{
    if (g_crcTableInitialized) return;

    for (int i = 0; i < 256; i++) {
        DWORD crc = i;
        for (int j = 8; j > 0; j--) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            }
            else {
                crc >>= 1;
            }
        }
        g_crcTable[i] = crc;
    }
    g_crcTableInitialized = true;
}

// Calculate CRC32 (matches assembly implementation)
DWORD CalculateCrc32(const void* data, size_t length)
{
    InitializeCrcTable();

    DWORD crc = 0xFFFFFFFF;
    const BYTE* bytes = static_cast<const BYTE*>(data);

    for (size_t i = 0; i < length; i++) {
        BYTE tableIndex = (crc ^ bytes[i]) & 0xFF;
        crc = (crc >> 8) ^ g_crcTable[tableIndex];
    }

    return crc ^ 0xFFFFFFFF;
}

DWORD UpdateCrc32(DWORD crc, BYTE value)
{
    InitializeCrcTable();
    BYTE tableIndex = (crc ^ value) & 0xFF;
    return (crc >> 8) ^ g_crcTable[tableIndex];
}

// Transform encryption key (matches assembly at 0x4011E7)
DWORD TransformEncryptionKey(DWORD baseKey)
{
    DWORD workingKey = baseKey;

    // Extract and rearrange key components (matches assembly bit manipulation)
    DWORD temp = workingKey & 0xFF00;
    temp |= (workingKey << 16);
    temp <<= 8;
    temp |= (workingKey & 0xFF);

    workingKey = temp ^ ENCRYPTION_KEY_BASE;

    BYTE k1 = static_cast<BYTE>(workingKey & 0xFF);
    BYTE k2 = static_cast<BYTE>((workingKey >> 8) & 0xFF);
    BYTE k3 = static_cast<BYTE>((workingKey >> 16) & 0xFF);
    BYTE k4 = static_cast<BYTE>((workingKey >> 24) & 0xFF);

    // Complex transformation (matches assembly algorithm)
    DWORD result = k1;
    result = (result * 33) + result + k2;  // k1 * 33 + k1 + k2
    result = (result * 33) + result + k3;  // result * 33 + result + k3
    result = (result * 33) + result + k4 + TRANSFORM_CONSTANT;

    return result;
}

// Main encryption/decryption function (matches assembly at 0x4011E7)
void EncryptDecryptData(void* data, DWORD key, size_t length)
{
    if (!data || length == 0) return;

    BYTE* bytes = static_cast<BYTE*>(data);
    DWORD alignment = key & 3;
    DWORD transformedKey = TransformEncryptionKey(key);
    DWORD streamKey = STREAM_KEY_INIT;

    if (alignment > 0 && length > alignment) {
        ProcessDataChunk(bytes, streamKey, transformedKey, alignment);
        bytes += alignment;
        length -= alignment;
    }

    // Process main data in 4-byte chunks
    while (length >= 4) {
        ProcessDataChunk(bytes, streamKey, transformedKey, 4);
        bytes += 4;
        length -= 4;
    }

    if (length > 0) {
        ProcessDataChunk(bytes, streamKey, transformedKey, length);
    }
}

// Process data chunk (matches assembly encryption loop)
void ProcessDataChunk(BYTE* data, DWORD& streamKey, DWORD& transformKey, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        BYTE keyByte = static_cast<BYTE>(transformKey & 0xFF);

        streamKey += g_crcTable[keyByte];

        // Transform the key (matches assembly bit operations)
        DWORD temp = (~transformKey) << 21;
        temp += CHUNK_CONSTANT_1;
        transformKey = (transformKey >> 11) | temp;

        BYTE encryptByte = static_cast<BYTE>(streamKey + transformKey);

        data[i] ^= encryptByte;

        streamKey = (streamKey * 33) + encryptByte + 3;
    }
}

DWORD GenerateStreamKey(DWORD baseKey, DWORD iteration)
{
    DWORD key = STREAM_KEY_INIT;
    DWORD transform = TransformEncryptionKey(baseKey);

    for (DWORD i = 0; i < iteration; i++) {
        BYTE keyByte = static_cast<BYTE>(transform & 0xFF);
        key += g_crcTable[keyByte];

        DWORD temp = (~transform) << 21;
        temp += CHUNK_CONSTANT_1;
        transform = (transform >> 11) | temp;
    }

    return key;
}

// Calculate module checksum (matches assembly checksum algorithm)
DWORD CalculateModuleChecksum(const void* moduleData, size_t moduleSize)
{
    if (!moduleData || moduleSize == 0) return 0;

    const BYTE* data = static_cast<const BYTE*>(moduleData);
    DWORD checksum = 0;

    for (size_t i = 0; i < moduleSize; i++) {
        checksum = UpdateCrc32(checksum, data[i]);
    }

    return checksum;
}

bool ValidateDataIntegrity(const void* data, size_t size, DWORD expectedChecksum)
{
    DWORD actualChecksum = CalculateModuleChecksum(data, size);
    return (actualChecksum == expectedChecksum);
}

void EncryptBuffer(void* buffer, size_t size, DWORD key)
{
    EncryptDecryptData(buffer, key, size);
}

void DecryptBuffer(void* buffer, size_t size, DWORD key)
{
    EncryptDecryptData(buffer, key, size);
}

// SHA1-like implementation (matches assembly hash functions)
void InitSHA1Context(DWORD* context)
{
    context[0] = 0x67452301;
    context[1] = 0xEFCDAB89;
    context[2] = 0x98BADCFE;
    context[3] = 0x10325476;
    context[4] = 0xC3D2E1F0;
}

void UpdateSHA1(DWORD* context, const void* data, size_t length)
{
    // Simplified SHA1 update (matches assembly pattern)
    const BYTE* bytes = static_cast<const BYTE*>(data);
    for (size_t i = 0; i < length; i++) {
        context[0] = (context[0] << 1) | (context[0] >> 31);
        context[0] ^= bytes[i];
    }
}

void FinalizeSHA1(DWORD* context, BYTE* hash)
{
    for (int i = 0; i < 5; i++) {
        hash[i * 4] = static_cast<BYTE>(context[i] & 0xFF);
        hash[i * 4 + 1] = static_cast<BYTE>((context[i] >> 8) & 0xFF);
        hash[i * 4 + 2] = static_cast<BYTE>((context[i] >> 16) & 0xFF);
        hash[i * 4 + 3] = static_cast<BYTE>((context[i] >> 24) & 0xFF);
    }
}

void CalculateSHA1Hash(const void* data, size_t length, BYTE* hash)
{
    DWORD context[5];
    InitSHA1Context(context);
    UpdateSHA1(context, data, length);
    FinalizeSHA1(context, hash);
}

// MD5-like implementation
void InitMD5Context(DWORD* context)
{
    context[0] = 0x67452301;
    context[1] = 0xEFCDAB89;
    context[2] = 0x98BADCFE;
    context[3] = 0x10325476;
}

void UpdateMD5(DWORD* context, const void* data, size_t length)
{
    const BYTE* bytes = static_cast<const BYTE*>(data);
    for (size_t i = 0; i < length; i++) {
        context[i % 4] ^= bytes[i];
        context[i % 4] = (context[i % 4] << 1) | (context[i % 4] >> 31);
    }
}

void FinalizeMD5(DWORD* context, BYTE* hash)
{
    for (int i = 0; i < 4; i++) {
        hash[i * 4] = static_cast<BYTE>(context[i] & 0xFF);
        hash[i * 4 + 1] = static_cast<BYTE>((context[i] >> 8) & 0xFF);
        hash[i * 4 + 2] = static_cast<BYTE>((context[i] >> 16) & 0xFF);
        hash[i * 4 + 3] = static_cast<BYTE>((context[i] >> 24) & 0xFF);
    }
}