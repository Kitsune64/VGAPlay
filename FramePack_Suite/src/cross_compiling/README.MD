# FramePack Suite

A powerful, multi-platform multimedia pipeline designed to extract video frames, process audio, and pack them into custom retro-compliant binary assets. 

This project is licensed under the **MIT License** and features fully dynamic, shared-library linking compliant with open-source distribution standards.

## 📋 Prerequisites

To build the suite from source, ensure you have the following tools installed on your host system:
* **GNU Make**
* **GCC** (for native Linux compilation)
* **MinGW-w64 toolchain** (for Windows cross-compilation: `i686-w64-mingw32-gcc` and `x86_64-w64-mingw32-gcc`)
* **zlib development files** (headers and libraries)

## 🚀 Features

* **Multi-Platform Targets:** Native builds for Linux (x64), modern Windows (32/64-bit), and a specialized legacy deployment pipeline targeting **Windows XP (NT 0x0501)**.
* **Customizable Image Extractions:** Allows users to choose their own custom output directory for extracted PNG frames.
* **Retro-Quantized Video & Audio:** Converts videos to high-quality 8-bit indexed 320x200 canvas with Floyd-Steinberg dithering and downmixes audio to Sound Blaster standard (11025 Hz, 8-bit Mono).

## 📁 Repository Structure

* `deps/WinXP/` - Houses essential runtime dependencies for legacy environments (`libgcc_s_dw2-1.dll`).
* `samples/` - Contains the universal `demo.mp4` video sample for instant verification.
* `script/WinXP/` - Houses the interactive, ultra-compatible `convert.bat` script.

## 🛠️ Building the Project

Ensure you have your environment-specific FFmpeg (3.4.2 for Windows XP, or modern versions for newer OS) development libraries in the `./ffmpeg/` directory.

To build all targeted subdirectories at once from your Linux cross-compilation host, run:
```bash
make clean
make
```
*(Note: running `make` or `make all` automatically triggers the full cross-compilation suite).*

To build a specific architecture independently:
```bash
make windowsxp  # Generates Windows XP binaries + packages convert.bat & runtime DLLs
make windows32  # Cross-compiles modern 32-bit Windows executables
make windows64  # Cross-compiles modern 64-bit Windows executables
make linux      # Generates native Linux binaries & packages convert.sh
```

## 📦 Usage (Windows Target)

1. Navigate to your compiled standalone package inside the `bin/` directory or download it from the **Releases** tab.
2. Ensure `convert.bat`, `demo.mp4`, and all accompanying `.exe` and `.dll` files are in the same local directory (e.g., `C:\FramePack Suite`).
3. Double-click **`convert.bat`** to start the interactive wizard. The script is optimized to fit legacy 80-column console environments perfectly.
