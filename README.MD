# VGAPlay

A high-performance VGA video player (fork of *DOSVideoPlayer* by Netdex) and multi-platform asset packaging suite designed for DOS systems (MS-DOS, FreeDOS, DOSBox Emulator).

[Demonstration Video (Workflow of the new version on Linux and DOSBox-X)](https://www.youtube.com/watch?v=j21uid9JJi8)

## Description

VGAPlay allows you to stream smooth, synchronized video and audio content on retro DOS hardware or emulators. The project is split into two main parts:
1. **The Host Tools (Linux / Windows):** A set of conversion scripts, a multi-platform compiler suite (`Makefile`), a powerful frame packager (`FramePack`), and verification tools to inspect generated binary packages.
2. **The DOS Player (`main.c`):** Written in C using the DJGPP compiler and the Allegro 4.2 library, running natively in VGA Mode 13h (320x200, 8-bit indexed color).

The player reads a custom binary container (`.BIN`) generated from a sequence of PNG frames. Frames are quantized using a median-cut algorithm, mapped to an 8-bit palette, and compressed using **LZ4 dependent stream compression** for fast real-time decompression under DOS constraints.

---

## Project Structure & Components

### 1. Host Conversion & Packaging Tools
* **`FramePack.c` (Core Packager):** Takes a directory of PNG frames, computes optimized 256-color palettes using Median-Cut quantization, compresses frame buffers using LZ4 dependent streams, and bundles everything into an optimized `.BIN` file.
* **`convert_dos.sh` / `convert_dos.bat`:** Helper scripts utilizing **FFmpeg** to convert raw video or audio inputs into the required DOS-compatible PCM format (`11025 Hz, 8-bit unsigned mono`).
* **`extract_frames.sh`:** A Bash pipeline script that extracts frames from an MP4 video, resizes/pads them to 320x200 pixels, applies Floyd-Steinberg dithering, and outputs indexed PNG frames ready for packaging.

### 2. Verification & Debugging Utilities
* **`verificator.c` (Host Verificator):** A multi-function utility compiled alongside `FramePack` to check `.BIN` structural integrity, analyze compression ratios, and extract specific frames back into PNG format for visual QA.
* **`verify.c` (DOS Verification Tool):** A target diagnostic tool designed to run or cross-check stream decoding stability and catch potential decompression errors block-by-block.

### 3. Target DOS Player
* **`main.c` (VGAPlay / RPG Engine Integration):** The main DOS executable that initializes the Sound Blaster audio track via Allegro, streams the `VIDEO.BIN` file in real-time, synchronizes frame delivery using hardware clocks, and seamlessly transitions into a Mode X game loop once playback finishes.

---

## Audio Requirements

To ensure smooth playback without buffering issues or audio stuttering under MS-DOS, audio tracks (`.WAV`) must adhere to strict parameters:
* **Format:** PCM 8-bit unsigned mono (`pcm_u8`)
* **Sample Rate:** 11025 Hz (recommended for optimal performance)

You can convert any standard audio or video file using the provided helper scripts or directly via **FFmpeg**:
```bash
ffmpeg -i input.mp4 -ar 11025 -ac 1 -c:a pcm_u8 -fflags +bitexact -map_metadata -1 audio_dos.wav
```
## Building the Suite (Host Side)

A comprehensive Makefile is provided to compile FramePack and the host Verificator for both Linux and Windows (using MinGW cross-compilation).

* Build everything (Linux & Windows targets):
```bash
make all
```

* Build Linux binaries only (./bin/Linux/):
```bash
make linux
```

* Cross-compile Windows binaries only (./bin/Windows/):
```bash
make windows
```

* Clean compiled artifacts:
```bash
make clean
```

## Workflow: From Video to DOS Binary
1. **Prepare and Extract Frames:**
    Use the extraction script or FFmpeg to output a sequence of 320x200 PNG images into a local folder (e.g., ./PNG/), named in ascending order (G_000000.PNG, G_000001.PNG, etc.).
2. **Generate the Video Binary (.BIN):*
    Run FramePack specifying the source directory, output prefix, extension, frame count, frames-per-second, and max color depth:
```bash
./bin/Linux/FramePack ./PNG video PNG 2853 15 256

Usage syntax: <DIRECTORY> <PREFIX> <EXTENSION> <FRAME_COUNT> <FPS> <MAX_COLORS>
```
3. **Verify the Package:**
You can inspect the integrity of the generated video.BIN file using the host verificator:
* **Analysis Mode:**
```bash
./bin/Linux/Verificator video.BIN
```
* **Extraction Mode (Export frame 150 to PNG):**
```bash
./bin/Linux/Verificator video.BIN 150 frame_150.png
```
4. **Run on DOS / DOSBox:**
    Place VIDEO.BIN and your converted AUDIO.WAV file in your target DOS environment directory and execute the player.
## Movie File Format (.BIN) Specification
```C
struct video_header {
    unsigned short frame_count;   // Total number of frames in the video (2 bytes)
    unsigned char  fps;           // Frames per second / Frame rate (1 byte)
    unsigned char  padding;       // Padding byte for 32-bit alignment (1 byte)
};

// The index table is located immediately after the global header.
// It contains 'frame_count' signed 4-byte integers (int32_t) representing 
// the compressed size (in bytes) of each frame's LZ4 data block.
typedef int32_t index_table_entry; // Size = frame_count * 4 bytes

struct palette_entry {
    unsigned char r; // Red component (0-255)
    unsigned char g; // Green component (0-255)
    unsigned char b; // Blue component (0-255)
};

struct frame_data_block {
    // DOS Fixed Palette Optimization: Systematically flushes a uniform 768-byte packet 
    // (256 colors * 3 bytes). Unused colors are padded with zeros (0,0,0).
    struct palette_entry palette[256]; 
    
    int size;                     // Size stored in the index table (LZ4 compressed size)
    unsigned char *data;          // Pixel index data (320x200 = 64,000 raw pixels) 
                                  // compressed via LZ4 in dependent stream mode (LZ4_compress_fast_continue)
};
```
## Compilation Requirements for DOS
To compile and run the player (main.c) natively or test it under MS-DOS:
* **Compiler:** DJGPP (GCC port for DOS)
* **Multimedia Library:** Allegro 4.2 library configured with Sound Blaster support.
* **Note:** You can find a complete, pre-configured DJGPP environment bundle for DOS with Allegro 4.2 and Sound Blaster fixes at [mrdictionary.net](https://mrdictionary.net/allegro/).
