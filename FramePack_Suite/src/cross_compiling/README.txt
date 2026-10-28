================================================================================
FramePack Suite (FramePack & Verificator)
================================================================================

This project provides two C applications designed to convert a
sequence of PNG images (320x200 resolution) into an optimized,
compact binary video stream (ideal for retro environments like
DOS), alongside a verification and extraction tool.


--------------------------------------------------------------------------------
1. PROJECT STRUCTURE & APPLICATIONS
--------------------------------------------------------------------------------
Based on the Makefile configuration, the suite compiles into:
* FramePack     : The encoding application (built from main.c).
                  It reads PNG sequences, applies Median Cut color
                  quantization (up to 256 colors), compresses pixel
                  indices using LZ4, and generates a .BIN file.
* Verificator   : The verification and extraction application
                  (built from verificator.c). It validates the
                  binary structure, checks compression metrics, and
                  extracts individual frames into PNG files.
* include/      : Local header files (LZ4 and libpng headers).
* lib/          : Auxiliary source files collected for build.


--------------------------------------------------------------------------------
2. KEY FEATURES
--------------------------------------------------------------------------------
- Custom Retro Encoding (320x200 pixels layout).
- Dynamic Color Quantization (Median Cut algorithm, 1 to 256 colors).
- High-Performance Compression (continuous LZ4 stream).
- Back-patched Index Table for direct frame access.
- Multi-platform target support (Linux and Windows targets).


--------------------------------------------------------------------------------
3. USAGE & COMPILATION
--------------------------------------------------------------------------------

A. Simplified Compilation:
   Run the following command to build everything automatically:
   make

   Other available make options:
   - make linux   : Build Linux binaries only
   - make windows : Build Windows binaries via cross-compilation
   - make clean   : Clean compiled binaries

B. Running FramePack (Encoder):
   Binaries are generated under bin/Linux/ or bin/Windows/.
   Syntax  : ./FramePack <DIR> <PREFIX> <EXT> <FRAMES> <FPS> <COLORS>
   Example : ./bin/Linux/FramePack ./PNG video PNG 2853 15 256
   (Produces an optimized video.BIN file).

C. Running Verificator:
   - Analysis Mode (File validation & compression ratio summary):
     ./bin/Linux/Verificator video.BIN
   - Extraction Mode (Extract a specific frame into a PNG file):
     ./bin/Linux/Verificator <video.bin> <frame_index> <output.png>
     Example : ./bin/Linux/Verificator video.BIN 150 frame_150.png


--------------------------------------------------------------------------------
4. BINARY FILE FORMAT (.BIN)
--------------------------------------------------------------------------------
1. Global Header (4 bytes):
   - frame_count : uint16_t (Total number of frames, up to 65535)
   - fps         : uint8_t  (Target frame rate)
   - padding     : uint8_t  (Alignment padding byte)

2. Index Table (4 bytes x frame_count):
   - Contains the exact byte size of each compressed LZ4 block per frame.

3. Frame Data Payload (Repeated per frame):
   - Fixed Palette : 768 bytes (256 RGB colors).
   - Raw LZ4 Block : Compressed index stream of the 320x200 pixel frame.


--------------------------------------------------------------------------------
5. TECHNICAL PREREQUISITES
--------------------------------------------------------------------------------
- C99 compatible compiler (GCC / MinGW-w64).
- Development libraries: libpng, zlib, and LZ4 (configured via CFLAGS).
