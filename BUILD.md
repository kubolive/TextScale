# Building TextScale from Source

This document describes how to compile the **TextScale** native OBS Studio plugin on Windows 10/11 x64.

---

## 1. Prerequisites and Toolchain

To build `text-scale.dll`, ensure the following tools are installed:

1. **Operating System:** Windows 10 or Windows 11 (64-bit).
2. **C++ Compiler:**
   - **GCC / MinGW-w64 x86_64** (UCRT runtime, POSIX threads, SEH exceptions; e.g. GCC 14.2.0 WinLibs).
   - Alternatively: **Microsoft Visual Studio 2022** (MSVC v143 toolset with C++ Desktop Workload).
3. **CMake:** Version 3.20 or newer.
4. **Ninja:** Version 1.10 or newer (or MSVC `msbuild` / `ninja`).
5. **OBS Studio SDK / Headers:**
   - Header files for `libobs` (`obs.h`, `graphics/graphics.h`, etc.) and `obs-frontend-api`.
   - The pre-configured source directory is located at `obs-src/libobs` in this project.

---

## 2. Environment Setup

Add your compiler and CMake binaries to your PowerShell or Command Prompt environment:

```powershell
# Example for MinGW-w64 GCC and CMake:
$env:Path = "C:\Users\Dell\mingw64\bin;C:\Program Files\CMake\bin;C:\Users\Dell\AppData\Local\Programs\Python\Python311\Scripts;" + $env:Path
```

Verify compiler and CMake accessibility:
```powershell
g++ --version
cmake --version
ninja --version
```

---

## 3. CMake Configuration

Navigate to the repository root and configure the build directory using Ninja:

```powershell
cd c:\Users\Dell\Desktop\text_scale_obs

cmake -B build -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_CXX_COMPILER="C:/Users/Dell/mingw64/bin/g++.exe" `
    -DCMAKE_C_COMPILER="C:/Users/Dell/mingw64/bin/gcc.exe"
```

---

## 4. Compiling the Plugin

Execute Ninja to compile the C++ source files and link `text-scale.dll`:

```powershell
cmake --build build --config Release
```

Expected build artifacts in `build/`:
- `text-scale.dll`: The compiled native OBS 64-bit video filter plugin (~630 KB).
- `test_math.exe`: The standalone unit test executable.

---

## 5. Running the Unit Tests

Execute the compiled math verification suite:

```powershell
.\build\test_math.exe
```

All 34 test cases will execute and output:
```
==================================================
ALL 34 TESTS PASSED SUCCESSFULLY!
==================================================
```

---

## 6. Packaging for Distribution

You can install or package the files into the standard OBS plugin hierarchy:

```powershell
# Create release structure
New-Item -ItemType Directory -Force -Path "dist/text-scale/bin/64bit"
New-Item -ItemType Directory -Force -Path "dist/text-scale/data/locale"

# Copy binary and runtime dependencies
Copy-Item "build/text-scale.dll" "dist/text-scale/bin/64bit/"
Copy-Item "C:\Users\Dell\mingw64\bin\libwinpthread-1.dll" "dist/text-scale/bin/64bit/"

# Copy shader and localization files
Copy-Item "data/text_scale.effect" "dist/text-scale/data/"
Copy-Item "data/locale/en-US.ini" "dist/text-scale/data/locale/"

# Create ZIP archive
Compress-Archive -Path "dist/text-scale" -DestinationPath "TextScale-OBS-Plugin-Windows-x64.zip" -Force
```

---

## 7. Troubleshooting Build Issues

- **Missing `gdiplus.lib` / GDI+ Headers:** Ensure `-lgdiplus` is present in linker flags when using MinGW (handled automatically by `CMakeLists.txt`).
- **Static Runtime Flags:** `-static-libgcc -static-libstdc++` are linked into the binary to prevent missing MSVCRT/GCC runtime DLL errors on end-user machines.
- **Header conflicts:** `obs-config.h` is provided within `obs-src/libobs/obsconfig.h` matching the OBS 31.1.2 platform target.
