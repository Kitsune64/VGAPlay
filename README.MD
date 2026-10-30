# VGAPlay

The VGAPlay Project is a synchronized real-time video and audio playback ecosystem engineered for legacy DOS architectures. Developed as a modern architectural fork of the original *DOSVideoPlayer* by Netdex (2017), this project bridges modern multi-format video standards with authentic x86 retro hardware.

The ecosystem provides two primary executable segments:
* **Target Player (`VGAPLAY.EXE`):** A native real-time multimedia player running in standard VGA Mode 13h (320x200 resolution, 8-bit indexed 256-color map) built for legacy PC environments (MS-DOS, FreeDOS, and DOSBox).
* **Host Toolkit (FramePack Suite):** A comprehensive, standalone cross-platform command-line toolset used to ingest modern files, handle color dither quantization, remap audio clocks, and compile custom binary movie streams.

---

## 📺 Demonstration
[How to use VGAPlay Video](https://www.youtube.com/watch?v=YW1tvyo3zkU&t) *(Old version overview)*

---

## 📝 Software Component Descriptions

### 1. The DOS Target Executables
* **`VGAPLAY.EXE` (`main.c`):** The main multimedia deployment player. Built with DJGPP and the Allegro 4.2 framework, it opens a custom standalone movie binary stream, locks execution frequencies directly to hardware timers, streams PCM data packets to Sound Blaster clocks, and allows seamless, automated transitions into Mode X game routines immediately upon cinematic completion.
* **`VERIFMOV.EXE` (`verify.c`):** A lightweight target diagnostic and verification command-line utility built for native DOS validation. It opens movie packages locally, runs raw data decoding integrity checks block-by-block, and outputs detailed structural health logs directly into a local `VERIFY.LOG` file.

### 2. The Host Conversion & Packaging Utilities (FramePack Suite)
* **`PNGExtract` (`PNGExtract.c`):** Ingests modern video container structures (such as MP4, MKV, AVI) natively via the FFmpeg API. It extracts raw image frames at a chosen frame rate, crops top-boundary camera artifacts (4-pixel crop), resizes the viewport onto a 320x200 canvas with optional aspect padding, and applies Floyd-Steinberg error diffusion dithering to output an optimized 8-bit indexed PNG sequence.
* **`mpg2wav` (`mpg2wav.c`):** Extracts and demuxes the primary audio track from any multimedia file using FFmpeg's `libswresample` engine. It mixes down multi-channel streams into Mono, applies anti-aliasing filters to downsample frequencies to a strict 11025 Hz rate, and quantizes depth to 8-bit Unsigned PCM, producing an épuré WAV structure with zero bloated metadata tags, preventing driver crashes on retro Sound Blaster 1.0/2.0/Pro/16 DAC hardware.
* **`FramePack` (`FramePack.c`):** The main production pipeline encoder. It scans an ordered directory of 8-bit indexed PNG frames, applies a Median-Cut quantization algorithm to build a fixed 768-byte palette structure per frame, compresses pixel indices using dependent streaming LZ4 compression (`LZ4_compress_fast_continue`), and packs the outputs into a single, high-efficiency movie binary package (`.BIN`).
* **`Verificator` (`verificator.c`):** A host-side debugging tool used to validate `.BIN` packages. It parses global headers, verifies continuous index tables, tests LZ4 stream decompression stabilities, computes exact compression efficiency statistics, and allows selective single-frame export back to standard PNG files for visual quality assurance audits.

---

## 🚀 Production Pipeline Workflow

To convert a modern video asset for deployment on retro DOS hardware, follow this sequence:
1. **Extract Frames:** Run `PNGExtract` on an MP4 to create dithered PNG trames.
2. **Extract Audio:** Run `mpg2wav` on the video to generate the 8-bit WAV file.
3. **Compile Stream:** Run `FramePack` on the PNG folder to build the `.BIN` file.
4. **Visual QA:** Use `Verificator` to validate structural block health.
5. **Deployment:** Move `.BIN`, `.WAV`, and `CWSDPMI.EXE` to your DOS drive.

---

## 💾 Running the Player under MS-DOS / DOSBox

### Hardware Baseline Requirements
* **CPU:** Intel 80386 processor or higher.
* **OS:** MS-DOS 5.0+, FreeDOS, or an emulation environment (DOSBox).
* **DPMI:** A 32-bit DPMI server is strictly mandatory. You must copy the standard extender binary `CWSDPMI.EXE` into the execution directory.

### Runtime Instructions
Launch the player by passing your movie filename prefix as a command-line argument (the player automatically links and streams the matching audio track):
```dos
VGAPLAY.EXE INTRO
# OR
VGAPLAY.EXE INTRO.BIN
```
*Note: The player requires an argument and will automatically resolve, link, and stream the associated `.WAV` audio file matching your prefix name.*

### Interactive Runtime In-Game Controls
* `[ESC]` – Instantly terminates playback and skips the cinematic.
* `[LEFT SHIFT]` / `[RIGHT SHIFT]` / `[L-CONTROL]` – Forces real-time hardware frame-skipping (Turbo Mode).

---

## 📑 Custom Movie File Format (.BIN) Specification

For engine extensions or custom player developments, layouts are mapped as follows:

```c
struct video_header {
    unsigned short frame_count;   // Total number of frames in the video (2 bytes)
    unsigned char  fps;           // Playback frame rate (1 byte)
    unsigned char  padding;       // Padding byte for 32-bit alignment (1 byte)
};

// The index table is located immediately after the global header.
// Contains 'frame_count' signed 4-byte integers (int32_t) indicating 
// the exact compressed byte-size of each subsequent LZ4 video data block payload.
typedef int32_t index_table_entry; // Total Size = frame_count * 4 bytes

struct palette_entry {
    unsigned char r; // Red component (0-255)
    unsigned char g; // Green component (0-255)
    unsigned char b; // Blue component (0-255)
};

struct frame_data_block {
    // DOS Fixed Palette Optimization: Systematically flushes a uniform 768-byte packet 
    // (256 colors * 3 bytes RGB). Unused color entries are padded with zeros (0,0,0).
    struct palette_entry palette[256]; 
    
    int size;                     // Size stored in the index table (LZ4 compressed size)
    unsigned char *data;          // Pixel index data (320x200 = 64,000 raw pixels) 
                                  // compressed via LZ4 in dependent stream mode
};
```

---

## 🛠️ Host Suite Compilation

The suite includes an optimized multi-target `Makefile` supporting native Linux builds and automated MinGW-w64 cross-compilation:

* `make all` (or `make`) – Builds Linux and Windows (32-bit & 64-bit) target suites.
* `make linux` – Compiles native Linux host utilities into `bin/Linux/`.
* `make windows32` – Cross-compiles 32-bit Windows executables into `bin/Win32/`.
* `make windows64` – Cross-compiles 64-bit Windows executables into `bin/Win64/`.
* `make clean` – Wipes the `bin/` build directory and flushes intermediate workspace artifacts.

*Note: Windows cross-compilation linking utilizes embedded stubs (`.dll.a` files) located in local `./ffmpeg/` paths. Runtime shared DLL files are excluded from the source tree to prevent bloating and comply with GitHub's file limits. If a local `bin/` directory containing the official runtime DLLs is detected, the Makefile will automatically package a standalone, deployable bundle.*

---

## ⚖️ Licensing & Distribution Information

The VGAPlay project aggregates components under highly permissive, compatible open-source legal frameworks:

* **Core Software Suite (`VGAPLAY.EXE`, `VERIFMOV.EXE`, FramePack Suite):** Distributed under the terms of the **MIT License**. Copyright (c) 2026 Kitsune64 on GitHub.
* **Embedded Compression Frameworks (`LZ4` & `xxHash`):** Integrated directly into source streams. Governed under the terms of the **BSD 2-Clause License**. Copyright (C) Yann Collet.
* **External Codec Backends (`libpng` & `zlib`):** Compiled statically from source files inside the repository. Governed under the terms of the **libpng License** and the **zlib License** respectively. Copyright (c) Cosmin Truta, Jean-loup Gailly, Mark Adler, and contributors.
* **Auxiliary DOS Runtime Extender (`CWSDPMI.EXE`):** Exclusive property of its author, distributed alongside compiled binaries under the **GNU General Public License version 2 (GNU GPL v2)** via official binary distribution clearings. Copyright (C) 1995-2010 Charles W. Sandmann.
