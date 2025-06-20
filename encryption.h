#pragma once
#include "common.h"

// CRC32 functions
void InitializeCrcTable();
DWORD CalculateCrc32(const void* data, size_t length);
DWORD UpdateCrc32(DWORD crc, BYTE value);

// Custom encryption/decryption (matches assembly algorithm)
void EncryptDecryptData(void* data, DWORD key, size_t length);
DWORD TransformEncryptionKey(DWORD baseKey);
void ProcessDataChunk(BYTE* data, DWORD& streamKey, DWORD& transformKey, size_t length);

// Advanced encryption operations
void EncryptBuffer(void* buffer, size_t size, DWORD key);
void DecryptBuffer(void* buffer, size_t size, DWORD key);
DWORD GenerateStreamKey(DWORD baseKey, DWORD iteration);

// Checksum and validation
DWORD CalculateModuleChecksum(const void* moduleData, size_t moduleSize);
bool ValidateDataIntegrity(const void* data, size_t size, DWORD expectedChecksum);

// Hash functions (SHA1-like implementation from assembly)
void InitSHA1Context(DWORD* context);
void UpdateSHA1(DWORD* context, const void* data, size_t length);
void FinalizeSHA1(DWORD* context, BYTE* hash);
void CalculateSHA1Hash(const void* data, size_t length, BYTE* hash);

// MD5-like functions (from assembly analysis)
void InitMD5Context(DWORD* context);
void UpdateMD5(DWORD* context, const void* data, size_t length);
void FinalizeMD5(DWORD* context, BYTE* hash);