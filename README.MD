# VGAPlay

**VGAPlay** is a real-time synchronized video and audio playback ecosystem engineered for legacy x86 PC architectures running MS-DOS, FreeDOS, or DOSBox emulation.

Developed as an extensive modern architectural fork of [*DOSVideoPlayer* by Netdex](https://github.com/Netdex/DOSVideoPlayer) (2017), VGAPlay bridges modern video formats with authentic retro hardware. It features a lightweight, zero-external-dependency DOS C player engine, alongside a robust cross-platform conversion suite (**FramePack Suite**).

---

## 🏗️ Architecture Overview

The VGAPlay ecosystem is divided into two distinct components:

1. **Target DOS Player (`VGAPLAY.EXE`):** A native 32-bit DOS application (compiled via DJGPP) operating in VGA Mode 13h ($320 \times 200$, 256 indexed colors). It decodes LZ4 video streams dynamically and feeds hardware PCM audio output with tight real-time clock synchronization.

2. **Host Suite (`FramePack Suite`):** A cross-platform multi-target command-line toolkit (Linux, Windows 32-bit, Windows 64-bit, Windows XP) used to process modern video files, perform Floyd-Steinberg dither quantization, extract raw PCM audio, and bundle video assets into compressed `.BIN` containers.

---

## 📺 Video Demonstration

*(Overview of early build capabilities)*

---

## 📝 Software Component Descriptions

### 1. DOS Target Executables

* **`VGAPLAY.EXE` (`main.c`):** The primary DOS multimedia player engine. Built in pure C (DJGPP target) with zero heavy external library dependencies:

  * **Graphics:** Direct Mode 13h BIOS Interrupt `$0x10$` configuration, DAC palette updates via hardware ports `$0x3C8$` / `$0x3C9$`, vertical retrace sync (`$0x3DA$`), and DPMI VRAM rendering at `$0xA0000$`.

  * **Audio Subsystem:** Features dual hardware audio drivers: native PCI Bus Mastering for **AC'97** (Intel ICH chipset family) and 16-bit auto-init DMA for **Sound Blaster 16** (with automatic fallback to a high-precision `uclock` software timing system if no sound card is present).

  * **Sync Engine:** Double-buffered 128 KB LZ4 decompression ring buffer (`video_ring_buffer`) for continuous streaming with automated frame-dropping lock-step audio synchronization.

* **`VERIFY.EXE` (`verify.c`):** A native DOS diagnostic and verification tool that parses global headers, validates block-by-block LZ4 stream decompression, and outputs structural health logs directly into `VERIFY.LOG`.

### 2. Host Conversion Utilities (FramePack Suite v1.4)

* **`PNGExtract` (`PNGExtract.c`):** Ingests video containers (MP4, MKV, AVI) natively using the FFmpeg API. It resizes frames to a $320 \times 200$ canvas (with direct stretch or 160px padded aspect options), applies high-quality Floyd-Steinberg error diffusion dithering, and outputs an 8-bit indexed PNG frame sequence.

* **`mpg2wav` (`mpg2wav.c`):** Demuxes the primary audio stream via FFmpeg (`libswresample`). It resamples audio down to a clean **44.1 kHz / Stereo / 16-bit Signed PCM WAV** file without bloated RIFF metadata tags to ensure maximum hardware driver stability.

* **`FramePack` (`FramePack.c`):** The primary production pipeline encoder. It scans an ordered directory of 8-bit indexed PNG frames, performs double-pass SHA-256 verification, maps palettes to uniform 768-byte chunks, compresses pixel index buffers via dependent streaming LZ4 compression (`LZ4_compress_fast_continue`), and packs everything into a unified `.BIN` movie package.

* **`Verificator` (`verificator.c`):** A host-side diagnostic tool used to validate `.BIN` packages. It parses index tables, calculates compression ratios, tests streaming decompression, and can extract individual frames back out to standard PNG image files for visual inspection.

---

## 🚀 Pipeline Workflow & Deployment

To convert a modern video file for playback on DOS hardware, follow this workflow:

```
┌────────────────┐     ┌────────────┐     ┌───────────┐     ┌──────────────┐
│  Source Video  ├────►│ PNGExtract ├────►│ FramePack ├────►│  VIDEO.BIN   │
│  (MP4/MKV/AVI) │     └────────────┘     └───────────┘     └──────┬───────┘
│                │     ┌────────────┐                          │ Deployment
│                ├────►│  mpg2wav   ├─────────────────────────►│  VIDEO.WAV   │
└────────────────┘     └────────────┘                          └──────────────┘
```

### Conversion Steps

1. **Extract Video Frames:**
   ```bash
   PNGExtract video.mp4 ./PNG_frames 15 256 1
   ```
   *(Args: `<input_file> <output_dir> <fps> <max_colors> <padding_mode 0/1>`)*

2. **Extract Audio Track:**
   ```bash
   mpg2wav video.mp4 VIDEO
   ```
   *(Args: `<input_file> <output_prefix>` -> produces `VIDEO.WAV`)*

3. **Pack Binary Video Container:**
   ```bash
   FramePack ./PNG_frames VIDEO 15 256
   ```
   *(Args: `<directory> <output_prefix> <fps> <max_colors>` -> produces `VIDEO.BIN`)*

4. **Validate Package (Optional):**
   ```bash
   Verificator VIDEO.BIN
   ```

5. **Deploy to DOS:**
   Transfer `VIDEO.BIN`, `VIDEO.WAV`, `VGAPLAY.EXE`, and the DPMI extender `CWSDPMI.EXE` into the target DOS directory.

---

## 💾 Running the Player under MS-DOS / DOSBox

### System Requirements

* **CPU:** Intel 80386 processor or higher.
* **OS:** MS-DOS 5.0+, FreeDOS, or DOSBox / DOSBox-Staging.
* **Audio:** AC'97 (Intel ICH PCI), Sound Blaster 16 compatible sound card, or silent mode.
* **DPMI Extender:** Requires `CWSDPMI.EXE` present in the execution folder or path.

### Execution Command

Launch the player by providing the base filename (prefix) of your movie:

```dos
VGAPLAY.EXE VIDEO
```

*Note: VGAPLAY will automatically look for and stream both `VIDEO.BIN` and `VIDEO.WAV`.*

### Command Line Options

* `VGAPLAY.EXE <filename>` – Plays the cinematic sequence.
* `VGAPLAY.EXE -v` or `--version` – Displays version information (`VGAPLAY version 1.4`).
* `VGAPLAY.EXE <filename> -idx` – Rebuilds the frame-to-sample index mapping file (`<filename>.IDX`).
* `VGAPLAY.EXE <filename> -log` – Generates real-time timing diagnostics into `SYNC.LOG`.

### In-Game / Runtime Controls

* `[ESC]` – Aborts playback and immediately returns to DOS / caller routine.

---

## 📑 Custom Movie File Format (.BIN) Specification

The `.BIN` file layout structure is packed as follows:

```c
struct video_header {
    uint16_t frame_count;   // Total number of video frames (2 bytes, Little-Endian)
    uint8_t  fps;           // Target playback frame rate (1 byte)
    uint8_t  padding;       // 32-bit alignment padding byte (1 byte)
};

// Index Table (located immediately after the header)
// Array of signed 32-bit integers specifying the LZ4 compressed payload 
// byte size for each frame: int32_t index_table[frame_count];

struct frame_data_block {
    // Fixed DOS Palette Chunk (256 RGB colors * 3 bytes = 768 bytes)
    struct palette_entry {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } palette[256];

    // LZ4 Compressed Stream Payload (Size given by index_table[frame_idx])
    // Uncompresses sequentially to exactly 64,000 raw pixel indices (320x200).
    uint8_t compressed_lz4_data[];
};
```

---

## 🛠️ Build & Compilation Instructions

### 1. Host Toolset (FramePack Suite)

The host tools include a multi-platform `Makefile` supporting Linux builds and MinGW-w64 cross-compilation:

* `make all` – Compiles binaries for Linux, Win32, and Win64.
* `make linux` – Compiles Linux host executables to `bin/Linux/`.
* `make windows32` – Cross-compiles 32-bit Windows executables to `bin/Win32/`.
* `make windows64` – Cross-compiles 64-bit Windows executables to `bin/Win64/`.
* `make clean` – Cleans up the `bin/` directory and intermediate build objects.

### 2. DOS Target Player (`VGAPLAY.EXE`)

To compile the DOS target executable:

* Use **DJGPP** (GCC for 32-bit DPMI DOS).
* Compile `main.c` linking `lz4.c` into `VGAPLAY.EXE`.

---

## ⚖️ Licensing & Legal Information

The VGAPlay project aggregates components under permissive, compatible open-source legal frameworks:

* **Core Software Suite (`VGAPLAY.EXE`, `VERIFY.EXE`, FramePack Suite):** **MIT License**. Copyright (c) 2026 Kitsune64 on GitHub.
* **LZ4 & xxHash:** **BSD 2-Clause License**. Copyright (C) Yann Collet.
* **libpng:** **libpng License**. Copyright (c) 1995-2019 Cosmin Truta and contributors.
* **zlib:** **zlib License**. Copyright (c) Jean-loup Gailly and Mark Adler.
* **FFmpeg:** Dynamically linked. **GNU Lesser General Public License (LGPL) v3.0 or later**. Copyright (c) FFmpeg Team.
* **CWSDPMI (`CWSDPMI.EXE`):** **GNU General Public License v2 (GNU GPL v2)** with binary distribution clearings. Copyright (C) 1995-2010 Charles W. Sandmann.

---

### Appendix: Full License Texts

#### 1. Core Project License (MIT License)

```text
Copyright (c) 2026 Kitsune64 on GitHub
Copyright (c) 2026 Netdex on Github

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

#### 2. LZ4 Decompression Library & xxHash (BSD 2-Clause License)

```text
Copyright (C) 2011-2017, Yann Collet.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

#### 3. libpng Reference Library (libpng License)

```text
Copyright (c) 1995-2019 Cosmin Truta and libpng contributors

The PNG Reference Library is supplied "AS IS". The Contributing Authors
and Group 42, Inc. disclaim all warranties, expressed or implied,
including, without limitation, the warranties of merchantability and of
fitness for any purpose. The Contributing Authors and Group 42, Inc.
assume no liability for direct, indirect, incidental, special, exemplary,
or consequential damages, which may result from the use of the PNG
Reference Library, even if advised of the possibility of such damage.

Permission is hereby granted to use, copy, modify, and distribute this
source code, or portions hereof, for any purpose, without fee, subject
to the following restrictions:

1. The origin of this source code must not be misrepresented.
2. Altered source versions must be plainly marked as such and must not
   be misrepresented as being the original source.
3. This Copyright notice may not be removed or altered from any source
   or altered source distribution.
```

---

#### 4. zlib Compression Library (zlib License)

```text
Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such and must not be
   misrepresented as being the original source.
3. This notice may not be removed or altered from any source distribution.
```

---

#### 5. FFmpeg Multimedia Framework (LGPL v3.0 Notice)

```text
Copyright (c) FFmpeg Team

The compiled binary packages of FramePack Suite distribute and interface with
the FFmpeg multimedia framework libraries dynamically (via external .dll files).

FFmpeg is a trademark of Fabrice Bellard (originator of the FFmpeg project).
The FFmpeg binaries bundled in this distribution are licensed under the GNU 
Lesser General Public License (LGPL) version 3.0 or later.

In strict compliance with LGPL guidelines:
1. The FFmpeg components are linked DYNAMICALLY ONLY. The core FramePack Suite
   executables remain isolated, independent binaries.
2. The unmodified third-party shared .dll libraries can be freely upgraded, 
   replaced, or re-linked by the end user at any time.
3. This notice explicitly provides the required attribution and mechanism 
   of use, fully preserving the standalone MIT License status of the core 
   FramePack Suite software programs.
```

---

#### 6. CWSDPMI Extender (GNU General Public License v2)

```text
Software Component : CWSDPMI (DPMI 0.90 Server for 32-bit DOS Executables)
Author / Copyright : Copyright (C) 1995-2010 Charles W. Sandmann

CWSDPMI is distributed under the terms of the GNU General Public License 
version 2 (GNU GPL v2) as published by the Free Software Foundation, with 
special binary redistribution permissions granted directly by the author, 
Charles W. Sandmann.

In accordance with the author's official distribution requirements (COPYING.CWS):
- Unmodified binary distributions of CWSDPMI.EXE may be bundled and 
  distributed alongside commercial or open-source software applications 
  without requiring the primary software application to be placed under 
  the GNU GPL license.
- Distributors are obligated to notify users of their right to acquire 
  the complete original source code, documentation, and official updates 
  for the CWSDPMI binary component.

Complete source code package (csdpmi7s.zip):
https://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/util/system/cwsdpmi/
```
