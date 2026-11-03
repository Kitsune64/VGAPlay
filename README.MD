# VGAPlay (v1.4)

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-MS--DOS%20%7C%20FreeDOS%20%7C%20DOSBox-green.svg)]()
[![Toolkit](https://img.shields.io/badge/Host%20Suite-Linux%20%7C%20Win32%20%7C%20Win64%20%7C%20WinXP-orange.svg)]()

**VGAPlay** is a real-time synchronized video and audio playback ecosystem engineered for legacy x86 PC architectures running MS-DOS, FreeDOS, or DOSBox emulation. 

Developed as an extensive modern architectural fork of [*DOSVideoPlayer* by Netdex](https://github.com/Netdex/DOSVideoPlayer) (2017), VGAPlay bridges modern video formats with authentic retro hardware. It features a lightweight, zero-external-dependency DOS C player engine, alongside a robust cross-platform conversion suite (**FramePack Suite v1.4**).

---

## 🏗️ Architecture Overview

The VGAPlay ecosystem is divided into two distinct components:

1. **Target DOS Player (`VGAPLAY.EXE`):** A native 32-bit DOS application (compiled via DJGPP) operating in VGA Mode 13h ($320 \times 200$, 256 indexed colors). It decodes LZ4 video streams dynamically and feeds hardware PCM audio output with tight real-time clock synchronization.
2. **Host Suite (`FramePack Suite`):** A cross-platform multi-target command-line toolkit (Linux, Windows 32-bit, Windows 64-bit, Windows XP) used to process modern video files, perform Floyd-Steinberg dither quantization, extract raw PCM audio, and bundle video assets into compressed `.BIN` containers.

---

## 📺 Video Demonstration
[![VGAPlay Demo](https://img.youtube.com/vi/YW1tvyo3zkU/0.jpg)](https://www.youtube.com/watch?v=YW1tvyo3zkU)  
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

This ecosystem aggregates software components under compatible open-source licenses:

* **Core Software Suite (`VGAPLAY.EXE`, `VERIFY.EXE`, FramePack Suite):** Distributed under the terms of the **MIT License**. Copyright (c) 2026 Kitsune64 on GitHub.
* **LZ4 Decompression Engine & xxHash:** Integrated directly into source files. Governed by the **BSD 2-Clause License**. Copyright (C) Yann Collet.
* **FFmpeg Libraries (`libavformat`, `libavcodec`, `libswresample`, `libswscale`):** Dynamically linked in Windows builds. Governed by the **GNU Lesser General Public License (LGPL) v2.1 or later**.
* **libpng & zlib:** Statically linked in host utilities. Governed by the **libpng License** and **zlib License** respectively.
* **CWSDPMI DPMI Server (`CWSDPMI.EXE`):** Distributed under the **GNU General Public License v2 (GPLv2)** with explicit binary bundling permissions granted by author Charles W. Sandmann.

```
                    GNU GENERAL PUBLIC LICENSE
                       Version 2, June 1991

 Copyright (C) 1989, 1991 Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

                            Preamble

  The licenses for most software are designed to take away your
freedom to share and change it.  By contrast, the GNU General Public
License is intended to guarantee your freedom to share and change free
software--to make sure the software is free for all its users.  This
General Public License applies to most of the Free Software
Foundation's software and to any other program whose authors commit to
using it.  (Some other Free Software Foundation software is covered by
the GNU Lesser General Public License instead.)  You can apply it to
your programs, too.

  When we speak of free software, we are referring to freedom, not
price.  Our General Public Licenses are designed to make sure that you
have the freedom to distribute copies of free software (and charge for
this service if you wish), that you receive source code or can get it
if you want it, that you can change the software or use pieces of it
in new free programs; and that you know you can do these things.

  To protect your rights, we need to make restrictions that forbid
anyone to deny you these rights or to ask you to surrender the rights.
These restrictions translate to certain responsibilities for you if you
distribute copies of the software, or if you modify it.

  For example, if you distribute copies of such a program, whether
gratis or for a fee, you must give the recipients all the rights that
you have.  You must make sure that they, too, receive or can get the
source code.  And you must show them these terms so they know their
rights.

  We protect your rights with two steps: (1) copyright the software, and
(2) offer you this license which gives you legal permission to copy,
distribute and/or modify the software.

  Also, for each author's protection and ours, we want to make certain
that everyone understands that there is no warranty for this free
software.  If the software is modified by someone else and passed on, we
want its recipients to know that what they have is not the original, so
that any problems introduced by others will not reflect on the original
authors' reputations.

  Finally, any free program is threatened constantly by software
patents.  We wish to avoid the danger that redistributors of an individual
program will individually obtain patent licenses, in effect making the
program proprietary.  To prevent this, we have made it clear that any
patent must be licensed for everyone's free use or not licensed at all.

  The precise terms and conditions for copying, distribution and
modification follow.

                    GNU GENERAL PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. This License applies to any program or other work which contains
a notice placed by the copyright holder saying it may be distributed
under the terms of this General Public License.  The "Program", below,
refers to any such program or work, and a "work based on the Program"
means either the Program or any derivative work under copyright law:
that is to say, a work containing the Program or a portion of it,
either verbatim or with modifications and/or translated into another
language.  (Hereinafter, translation is included without limitation in
the term "modification".)  Each licensee is addressed as "you".

Activities other than copying, distribution and modification are not
covered by this License; they are outside its scope.  The act of
running the Program is not restricted, and the output from the Program
is covered only if its contents constitute a work based on the
Program (independent of having been made by running the Program).
Whether that is true depends on what the Program does.

  1. You may copy and distribute verbatim copies of the Program's
source code as you receive it, in any medium, provided that you
conspicuously and appropriately publish on each copy an appropriate
copyright notice and disclaimer of warranty; keep intact all the
notices that refer to this License and to the absence of any warranty;
and give any other recipients of the Program a copy of this License
along with the Program.

You may charge a fee for the physical act of transferring a copy, and
you may at your option offer warranty protection in exchange for a fee.

  2. You may modify your copy or copies of the Program or any portion
of it, thus forming a work based on the Program, and copy and
distribute such modifications or work under the terms of Section 1
above, provided that you also meet all of these conditions:

    a) You must cause the modified files to carry prominent notices
    stating that you changed the files and the date of any change.

    b) You must cause any work that you distribute or publish, that in
    whole or in part contains or is derived from the Program or any
    part thereof, to be licensed as a whole at no charge to all third
    parties under the terms of this License.

    c) If the modified program normally reads commands interactively
    when run, you must cause it, when started running for such
    interactive use in the most ordinary way, to print or display an
    announcement including an appropriate copyright notice and a
    notice that there is no warranty (or else, saying that you provide
    a warranty) and that users may redistribute the program under
    these conditions, and telling the user how to view a copy of this
    License.  (Exception: if the Program itself is interactive but
    does not normally print such an announcement, your work based on
    the Program is not required to print an announcement.)

These requirements apply to the modified work as a whole.  If
identifiable sections of that work are not derived from the Program,
and can be reasonably considered independent and separate works in
themselves, then this License, and its terms, do not apply to those
sections when you distribute them as separate works.  But when you
distribute the same sections as part of a whole which is a work based
on the Program, the distribution of the whole must be on the terms of
this License, whose permissions for other licensees extend to the
entire whole, and thus to each and every part regardless of who wrote it.

Thus, it is not the intent of this section to claim rights or contest
your rights to work written entirely by you; rather, the intent is to
exercise the right to control the distribution of derivative or
collective works based on the Program.

In addition, mere aggregation of another work not based on the Program
with the Program (or with a work based on the Program) on a volume of
a storage or distribution medium does not bring the other work under
the scope of this License.

  3. You may copy and distribute the Program (or a work based on it,
under Section 2) in object code or executable form under the terms of
Sections 1 and 2 above provided that you also do one of the following:

    a) Accompany it with the complete corresponding machine-readable
    source code, which must be distributed under the terms of Sections
    1 and 2 above on a medium customarily used for software interchange; or,

    b) Accompany it with a written offer, valid for at least three
    years, to give any third party, for a charge no more than your
    cost of physically performing source distribution, a complete
    machine-readable copy of the corresponding source code, to be
    distributed under the terms of Sections 1 and 2 above on a medium
    customarily used for software interchange; or,

    c) Accompany it with the information you received as to the offer
    to distribute corresponding source code.  (This alternative is
    allowed only for noncommercial distribution and only if you
    received the program in object code or executable form with such
    an offer, in accord with Subsection b above.)

The source code for a work means the preferred form of the work for
making modifications to it.  For an executable work, complete source
code means all the source code for all modules it contains, plus any
associated interface definition files, plus the scripts used to
control compilation and installation of the executable.  However, as a
special exception, the source code distributed need not include
anything that is normally distributed (in either source or binary
form) with the major components (compiler, kernel, and so on) of the
operating system on which the executable runs, unless that component
itself accompanies the executable.

If distribution of executable or object code is made by offering
access to copy from a designated place, then offering equivalent
access to copy the source code from the same place counts as
distribution of the source code, even though third parties are not
compelled to copy the source along with the object code.

  4. You may not copy, modify, sublicense, or distribute the Program
except as expressly provided under this License.  Any attempt
otherwise to copy, modify, sublicense or distribute the Program is
void, and will automatically terminate your rights under this License.
However, parties who have received copies, or rights, from you under
this License will not have their licenses terminated so long as such
parties remain in full compliance.

  5. You are not required to accept this License, since you have not
signed it.  However, nothing else grants you permission to modify or
distribute the Program or its derivative works.  These actions are
prohibited by law if you do not accept this License.  Therefore, by
modifying or distributing the Program (or any work based on the
Program), you indicate your acceptance of this License to do so, and
all its terms and conditions for copying, distributing or modifying
the Program or works based on it.

  6. Each time you redistribute the Program (or any work based on the
Program), the recipient automatically receives a license from the
original licensor to copy, distribute or modify the Program subject to
these terms and conditions.  You may not impose any further
restrictions on the recipients' exercise of the rights granted herein.
You are not responsible for enforcing compliance by third parties to
this License.

  7. If, as a consequence of a court judgment or allegation of patent
infringement or for any other reason (not limited to patent issues),
conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot
distribute so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you
may not distribute the Program at all.  For example, if a patent
license would not permit royalty-free redistribution of the Program by
all those who receive copies directly or indirectly through you, then
the only way you could satisfy both it and this License would be to
refrain entirely from distribution of the Program.

If any portion of this section is held invalid or unenforceable under
any particular circumstance, the balance of the section is intended to
apply and the section as a whole is intended to apply in other
circumstances.

It is not the purpose of this section to induce you to infringe any
patents or other property right claims or to contest validity of any
such claims; this section has the sole purpose of protecting the
integrity of the free software distribution system, which is
implemented by public license practices.  Many people have made
generous contributions to the wide range of software distributed
through that system in reliance on consistent application of that
system; it is up to the author/donor to decide if he or she is willing
to distribute software through any other system and a licensee cannot
impose that choice.

This section is intended to make thoroughly clear what is believed to
be a consequence of the rest of this License.

  8. If the distribution and/or use of the Program is restricted in
certain countries either by patents or by copyrighted interfaces, the
original copyright holder who places the Program under this License
may add an explicit geographical distribution limitation excluding
those countries, so that distribution is permitted only in or among
countries not thus excluded.  In such case, this License incorporates
the limitation as if written in the body of this License.

  9. The Free Software Foundation may publish revised and/or new versions
of the General Public License from time to time.  Such new versions will
be similar in spirit to the present version, but may differ in detail to
address new problems or concerns.

Each version is given a distinguishing version number.  If the Program
specifies a version number of this License which applies to it and "any
later version", you have the option of following the terms and conditions
either of that version or of any later version published by the Free
Software Foundation.  If the Program does not specify a version number of
this License, you may choose any version ever published by the Free Software
Foundation.

  10. If you wish to incorporate parts of the Program into other free
programs whose distribution conditions are different, write to the author
to ask for permission.  For software which is copyrighted by the Free
Software Foundation, write to the Free Software Foundation; we sometimes
make exceptions for this.  Our decision will be guided by the two goals
of preserving the free status of all derivatives of our free software and
of promoting the sharing and reuse of software generally.

                            NO WARRANTY

  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

                     END OF TERMS AND CONDITIONS
```
