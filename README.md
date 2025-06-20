# AliciaOnlineLauncher

A reverse-engineered and decompiled version of the Alicia Online game launcher, reconstructed from assembly analysis.

## Overview

This project contains a complete decompilation of the Alicia Online launcher executable, originally located at `C:\Users\username\AppData\Roaming\AliciaOnline\Launcher.exe`. The codebase has been reconstructed from assembly analysis to understand the launcher's functionality, including its custom encryption, file operations, and update mechanisms.

## Current Status

🚧 **Work in Progress** 🚧

- ✅ **Complete decompilation** of the original launcher
- ✅ **Encryption algorithms** fully reverse-engineered
- ✅ **File operations** and memory management reconstructed
- ✅ **Registry operations** implemented
- 🔄 **Update mechanism** currently triggers the original updater
- 🔧 **Updater bypass** is work in progress

### Current Behavior

The launcher currently mimics the original behavior by:
1. Checking registry for update flags
2. Triggering the `Install\Update.exe` process when updates are detected
3. Processing embedded module data with custom encryption

**Note:** The updater bypass is still being developed. Currently, the launcher will attempt to launch the update process rather than bypassing it entirely.

## Project Structure

```
AliciaOnlineLauncher/
├── main.cpp              # Entry point and initialization
├── launcher.cpp/.h       # Core launcher logic and module processing
├── encryption.cpp/.h     # Custom encryption/decryption algorithms
├── fileops.cpp/.h        # File mapping and I/O operations
├── memory.cpp/.h         # Custom memory management
├── registry.cpp/.h       # Windows registry operations
├── common.h              # Shared constants and structures
├── resource.h            # Resource definitions
└── launcher.manifest     # Application manifest
```

## Key Features Reverse-Engineered

### 🔐 Custom Encryption System
- **CRC32 table-based** encryption/decryption
- **Stream cipher** with key transformation
- **Module integrity validation** using custom checksums
- **SHA1/MD5-like** hashing implementations

### 📁 File Operations
- **Memory-mapped file** handling
- **Temporary file** creation and management
- **Embedded resource** extraction
- **Directory traversal** and creation

### 🔧 System Integration
- **Registry manipulation** for configuration
- **Environment variable** setup
- **Module checksum** validation
- **Update process** coordination

## Building

### Prerequisites
- Visual Studio 2022 or later
- Windows SDK 10.0
- C++17 support

### Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/apfelteesaft/AliciaOnlineLauncher.git
cd AliciaOnlineLauncher
```

2. Open `AliciaOnlineLauncher.sln` in Visual Studio

3. Build the solution:
   - **Debug|x86** for development
   - **Release|x86** for production (outputs as `Launcher.exe`)

## Technical Details

### Encryption Algorithm
The launcher uses a sophisticated custom encryption scheme:

```cpp
// Base encryption key transformation
DWORD transformedKey = TransformEncryptionKey(baseKey);
DWORD streamKey = STREAM_KEY_INIT;

// Data processing in chunks with CRC32 table
ProcessDataChunk(data, streamKey, transformedKey, length);
```

### Module Validation
Each module is validated using:
- **CRC32 checksums** for data integrity
- **Signature verification** with XOR operations (`0xC56210EC ^ 0xFC5ABF8A`)
- **Dynamic key generation** based on module content

### Registry Keys
The launcher reads/writes to:
- `HKCU\SOFTWARE\AliciaOnlineShortcut`
- `HKLM\SOFTWARE\AliciaOnlineShortcut`
- Update flags and configuration values

## Current Limitations

1. **Update Bypass Incomplete**: Currently triggers the original updater instead of bypassing it
2. **Module Loading**: Embedded module execution needs further development
3. **Error Handling**: Some edge cases in file operations need refinement

## Roadmap

- [ ] Complete updater bypass implementation
- [ ] Implement direct game launching without updater
- [ ] Add configuration options for different launch modes
- [ ] Enhance error handling and logging
- [ ] Add support for custom game paths
- [ ] Implement alternative update checking mechanisms

## Research Notes

### Assembly Analysis Locations
Key functions were located at these assembly addresses in the original binary:
- `0x401000` - Memory allocation
- `0x4010FF` - Module checksum calculation  
- `0x4011E7` - Encryption/decryption
- `0x401847` - CRC32 table initialization
- `0x402255` - File mapping creation
- `0x40270A` - Registry operations
- `0x402A7A` - Main launcher logic

### Magic Constants
```cpp
#define ENCRYPTION_KEY_BASE    0xB11924E1
#define CRC_POLYNOMIAL         0xEDB88320
#define STREAM_KEY_INIT        0x96438AF7
#define SIGNATURE_XOR          0xC56210EC
#define SIGNATURE_CHECK        0xFC5ABF8A
```

## Contributing

This is a reverse engineering project for educational purposes. Contributions are welcome for:
- Improving the updater bypass mechanism
- Enhancing code documentation
- Adding error handling
- Optimizing performance

## Legal Notice

This project is for educational and research purposes only. The code has been reverse-engineered from publicly available software for the purpose of understanding system internals and improving compatibility. Users should ensure they comply with all applicable laws and terms of service.

## License

This project is provided as-is for educational purposes. Please respect the original software's terms of service and applicable copyright laws.

---

**Note**: This decompilation represents the current understanding of the launcher's functionality. Some implementation details may differ from the original due to compiler optimizations and assembly-to-C++ translation.