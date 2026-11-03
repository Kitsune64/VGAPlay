# FramePack Suite

A powerful, multi-platform multimedia pipeline designed to extract video frames, process audio, and pack them into custom retro-compliant binary assets (specifically optimized for retro playback engines like *VGAPlay*).

This project is licensed under the **MIT License** and features fully dynamic, shared-library linking compliant with open-source distribution standards.

## 🚀 Prerequisites

To build the suite from source, ensure you have the following tools installed on your host system:

* **GNU Make**
* **GCC** (for native Linux compilation)
* **MinGW-w64 toolchain** (for Windows cross-compilation: `i686-w64-mingw32-gcc` and `x86_64-w64-mingw32-gcc`)
* **zlib development files**
* **FFmpeg and libpng development libraries** (for frame and audio extraction tools)

## 🛠️ Suite Utilities (v1.4)

The suite consists of 4 main utilities:

1. **FramePack** (`FramePack.c`): Packs all indexed PNG images from a folder into a single optimized `.BIN` file (incorporating LZ4 compression, SHA-256 integrity checks, and palette analysis).
2. **Verificator** (`verificator.c`): Analyzes and validates the global structure of generated `.BIN` files, verifies compressed blocks, and allows targeted extraction of specific frames to PNG.
3. **PNGExtract** (`PNGExtract.c`): Converts video input (e.g., MP4) into an 8-bit indexed PNG image sequence using Floyd-Steinberg dithering, constant frame rate (CFR) filter control, and padding/aspect ratio modes.
4. **mpg2wav** (`mpg2wav.c`): Extracts and resamples video audio streams into an uncompressed standard WAV PCM format (44100 Hz, Stereo, 16-bit signed, Sound Blaster 16 compatible).

## 📂 Repository Structure

* `lib/` - Houses embedded third-party components (LZ4, xxHash).
* `include/` - Shared C project header files.
* `deps/WinXP/` - Houses essential runtime dependencies for legacy environments (`libgcc_s_dw2-1.dll`).
* `samples/` - Contains the universal `demo.mp4` video sample for instant verification.
* `script/` - Automation and deployment scripts (`convert.sh` for Linux, `convert.bat` for Windows/XP).

## 📦 Building the Project

Ensure you have your environment-specific FFmpeg development libraries in the `./ffmpeg/` directory (or configured via `pkg-config` under Linux).

To build all targeted subdirectories at once from your Linux cross-compilation host, run:

```bash
make clean
make
```

To build a specific target architecture independently:

```bash
make linux        # Generates native Linux binaries & packages convert.sh
make windows32    # Cross-compiles modern 32-bit Windows executables
make windows64    # Cross-compiles modern 64-bit Windows executables
make windowsxp    # Generates Windows XP 32-bit binaries + packages convert.bat & runtime DLLs
```

## ⚙️ Quick Usage

1. Navigate to your compiled standalone package inside the corresponding target subdirectory under `bin/` or download an archive from the **Releases** section.
2. For automated conversion under Windows, run the interactive **`convert.bat`** wizard alongside the executables and your input media.

## 📄 License

The core source code of the **FramePack Suite** created by the author is licensed under the **MIT License**. Incorporated third-party components (such as LZ4, libpng, zlib, and FFmpeg) retain their respective permissive open-source licenses (BSD, zlib, LGPL). Refer to the `LICENSE` file for full legal details.