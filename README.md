# Alicia Online Launcher

A comprehensive launcher system for Alicia Online with integrated locale emulation capabilities.

## ⚠️ Project Status

**This project is no longer under active development.** The AliciaOnlineUpdater component remains incomplete and will not be worked on further. This repository is provided as-is for reference purposes.

### ⚠️ Important Disclaimer

**This is a decompilation and reconstruction of the original launcher provided by [aliciagame.com](https://aliciagame.com).** 

- **Functionality is NOT guaranteed** - This code may not work as expected
- **Reverse-engineered code** - Reconstructed from assembly/binary analysis
- **Educational purposes only** - Intended for learning and reference
- **No official support** - This is an unofficial reconstruction
- **Use at your own risk** - No warranty or guarantee of functionality

## Overview

This project consists of multiple components that were reverse-engineered from the official Alicia Online launcher to understand its architecture and locale emulation system:

### Components

#### 1. AliciaOnlineLauncher
The main launcher application that handles:
- **Game file management** with encryption/decryption capabilities
- **Registry-based configuration** storage
- **Memory management** utilities
- **File operations** for game assets
- **Update checking** and process management
- **Embedded module loading** and execution

#### 2. NTLEAS (NT Locale Emulator Advance)
A standalone locale emulation tool that allows running Windows applications with specific regional settings:
- **Process injection** for locale emulation
- **API hooking** for character encoding conversion
- **Command-line interface** for configuration
- **Multi-language support** (Japanese, Chinese, Korean, etc.)
- **Debug mode** with comprehensive error handling

#### 3. ntleai.dll
The injection DLL that provides runtime locale emulation:
- **API function hooking** for encoding conversion functions
- **Locale override** for system calls
- **Thread-safe** string conversion operations
- **Exception handling** with crash dump generation
- **COM interface integration** for codepage detection

#### 4. AliciaOnlineUpdater *(Incomplete)*
A basic updater application template that was not fully implemented.

## Features

### Launcher Capabilities
- Custom encryption algorithms for game file protection
- Registry-based configuration management
- Automatic update detection and launching
- File mapping and memory management
- Process spawning with locale emulation integration

### Locale Emulation
- **Codepage conversion**: Automatic handling of character encoding
- **Locale spoofing**: Override system locale for specific applications
- **API interception**: Hooks Windows APIs for seamless locale emulation
- **Multi-threading support**: Thread-safe operation with synchronization
- **Error resilience**: Comprehensive error handling and recovery

### Supported Locales
- Japanese (Shift-JIS, CP932)
- Simplified Chinese (GBK, CP936)
- Traditional Chinese (Big5, CP950)
- Korean (CP949)
- And other Windows codepages

## Build Requirements

- **Visual Studio 2022** (v143 toolset)
- **Windows 10 SDK**
- **C++17** standard support
- Target platforms: x86, x64

### Dependencies
- `kernel32.lib`, `user32.lib`, `advapi32.lib`
- `shell32.lib`, `shlwapi.lib`
- `version.lib`, `ole32.lib`
- `comdlg32.lib` (for NTLEAS)

### ⚠️ Build Warnings
Since this is **reverse-engineered code**:
- Some function signatures may not be completely accurate
- **Compilation is not guaranteed** to succeed without modifications
- **Runtime behavior may differ** from the original launcher
- **Debugging information** was reconstructed and may be incomplete
- **Dependencies** may not match the original exactly

## Usage

### Basic Launcher
```
Launcher.exe
```

### NTLEAS Command Line
```
ntleas.exe <target_executable> [options]
```

#### NTLEAS Options
- `A<args>` - Application arguments
- `C<codepage>` - Set codepage (e.g., C932 for Japanese)
- `L<lcid>` - Set locale ID (e.g., L1041 for Japanese)
- `D` - Enable debug mode
- `E<0|1>` - Error display toggle
- `T` - Set working directory to target executable's directory

#### Example
```bash
# Run a Japanese game with proper locale emulation
ntleas.exe game.exe C932 L1041 T
```

## Architecture

**Note: This architecture was reconstructed through reverse engineering and assembly analysis of the original binaries.**

### Encryption System
The launcher implements a custom encryption scheme with:
- **CRC32-based checksums** for integrity verification
- **Stream ciphers** with key transformation
- **Chunk-based processing** for large files
- **Memory-safe operations** with bounds checking

### API Hooking Mechanism
The locale emulation uses sophisticated API hooking:
- **Runtime patching** of system DLLs
- **Import table modification** for seamless integration
- **Thread-local storage** for per-thread state management
- **Exception safety** with proper cleanup

### Memory Management
Custom memory allocators provide:
- **Heap isolation** for crash resilience
- **Secure memory clearing** for sensitive data
- **Buffer overflow protection** with validation
- **Leak detection** in debug builds

*Implementation details were reconstructed from assembly code analysis and may not reflect the exact original implementation.*

## File Structure

```
AliciaOnlineLauncher/
├── AliciaOnlineLauncher/     # Main launcher application
│   ├── encryption.*          # Encryption and CRC utilities
│   ├── fileops.*             # File and directory operations
│   ├── launcher.*            # Core launcher logic
│   ├── memory.*              # Memory management
│   ├── registry.*            # Windows registry operations
│   └── main.cpp              # Application entry point
├── NTLEAS/                   # Locale emulator application
│   ├── ntleas.cpp            # Main emulator logic
│   ├── defs.h                # System definitions
│   └── ntleas.manifest       # Application manifest
├── ntleai/                   # Injection DLL
│   ├── ntleai.cpp            # Core DLL functionality
│   ├── hooks.cpp             # API hook implementations
│   ├── dllmain.cpp           # DLL entry point
│   └── ntleai.h              # Header definitions
└── AliciaOnlineUpdater/      # Incomplete updater (not developed)
```

## Security Considerations

- **Process integrity**: Validation of target executables before injection
- **Memory protection**: Secure handling of sensitive configuration data
- **Error isolation**: Exception handling prevents crashes from propagating
- **Access control**: Proper privilege management for system operations

## Compatibility

### Supported Windows Versions
- Windows 7 SP1+
- Windows 8/8.1
- Windows 10/11

### Architecture Support
- x86 (32-bit) - Primary target
- x64 (64-bit) - Supported

## Limitations

- **Windows-only**: No cross-platform support
- **Process injection**: May trigger antivirus software
- **System-level**: Requires appropriate privileges for API hooking
- **Legacy codebase**: Built for older game compatibility

## Legal Notice

**This is reverse-engineered software based on the original launcher from [aliciagame.com](https://aliciagame.com).**

- This code is provided **for educational and research purposes only**
- **Not affiliated with or endorsed by** the original developers
- Users are responsible for ensuring compliance with applicable software licenses and terms of service
- **Reverse engineering** was performed for educational understanding of launcher architecture and locale emulation techniques
- **No copyright infringement intended** - this is for educational analysis only
- **Original intellectual property** belongs to the respective owners at aliciagame.com

**Use of this code with any commercial software should comply with relevant terms of service and licensing agreements.**

## Contributing

This project is **no longer accepting contributions** as it is not under active development. The code is provided as-is for reference and educational purposes.

---

*Note: This launcher was designed specifically for Alicia Online and includes sophisticated locale emulation capabilities that may be useful for other legacy applications requiring specific regional settings.*