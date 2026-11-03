================================================================================
							  VGAPLAY (V1.4)
================================================================================
1. PROJECT OVERVIEW
--------------------------------------------------------------------------------

VGAPlay is a cinematic and video stream player optimized for the MS-DOS 
operating system (compiled using DJGPP). It plays video streams compressed 
with the LZ4 algorithm in standard VGA graphics mode (Mode 13h: 320x200 / 
320x256 resolution in 256 colors) with full audio synchronization.

Major Changes in Version 1.4:

  * Complete Removal of Allegro Dependency: Graphics, text, keyboard, and audio 
    drivers have been entirely rewritten using low-level native C code (BIOS 
    interrupt 0x10 calls, direct I/O port manipulation via conio.h, and DPMI 
    memory access via dosmemput).
  * Native Audio Drivers: Direct hardware support for AC'97 (Intel ICH) via 
    PCI bus mastering and Sound Blaster 16 (16-bit auto-init DMA), including a 
    fallback silent mode driven by a software clock (uclock).
  * Licensing: The entire codebase is distributed under the MIT License, with 
    the exception of explicitly credited third-party parts (such as the 
    initial indexing structure inspired by Volkertb's "ich2player", while 
    excluding the CC-BY-SA 3.0 file printhex.asm to maintain MIT purity).
    The original version of ich2player was originally developed and
    published by Jeff Leyda on Sep 02, 2002 (jeff@silent.net).

2. MAIN PROGRAM: VGAPLAY.EXE (main.c)
--------------------------------------------------------------------------------

VGAPLAY.EXE is the primary playback engine. It orchestrates video decoding, 
graphic rendering, user input handling, and audio synchronization.

A. Syntax and Usage:

   VGAPLAY.EXE <base_filename> [options]

   * <base_filename> : Base name without extension (searches for <name>.BIN 
                       for video data and <name>.WAV for audio data).
   * -idx            : Optional flag to manually rebuild the temporal index 
                       file (.IDX) if missing or corrupted.
B. Architecture and Execution Flow:

   1. Audio Hardware Initialization (audio_init):

      * Attempts to parse parameters from CONFIG.CFG (SB_BASE, SB_IRQ, 
        SB_DMA16).
      * Falls back to automatic detection of AC'97 (via PCI bus scanning) or 
        Sound Blaster 16 (via BLASTER environment variable or I/O ports).
      * If no compatible hardware is present, switches to silent mode using 
        the system uclock.

   2. Stream Loading and Indexing:

      * Parses the binary video header (decode_video_header) from the .BIN file.
      * Opens the .WAV file and validates its parameters (16-bit PCM, mono/stereo).
      * Loads or generates the temporal index table (.IDX), linking each frame 
        to a specific audio sample count to maintain tight sync.

   3. Graphics Mode Switch (Mode 13h):

     * Executes BIOS software interrupt __dpmi_int(0x10) with AX = 0x0013 to 
        set 320x200 graphics mode in 256 colors.

   4. Main Playback Loop (play_lz4_cinematic):

      * User Input: Monitors keyboard input in real time via kbhit() / getch(). 
        Pressing ESC (ASCII 27) cleanly aborts playback.
      * Audio-Video Sync: Queries exact audio hardware position via 
        audio_get_position() (DMA registers for AC'97, software clock for 
        SB16 / Silent Mode).
      * Continuous LZ4 Decoding: Decompresses frames on the fly into a 
        unified circular buffer (video_ring_buffer) to preserve frame history.
      * Vertical Retrace (Vsync): Waits for vertical retrace (inportb(0x3DA)) 
        to eliminate screen tearing.
      * VGA Rendering: Updates the 256-color DAC palette (ports 0x3C8/0x3C9) 
        and transfers pixel data to VRAM at 0xA0000 via dosmemput.

   5. Cleanup & Exit:

      * Restores standard MS-DOS text mode (AX = 0x0003).
      * Frees allocated heap memory and halts audio hardware (audio_stop).

3. VERIFICATION PROGRAM: VERIFMOV.EXE (verify.c)
--------------------------------------------------------------------------------

VERIFMOV.EXE is a diagnostic utility for quality assurance. It verifies the 
structural integrity and complete LZ4 decompressibility of a video file (.BIN) 
prior to deployment on target hardware.

A. Syntax and Usage:

   VERIFY.EXE <path_to_video.bin>

B. Architecture and Operations:

   1. Header Analysis:

      * Opens the binary video file in read-only mode.
      * Reads global parameters: frame count, frame rate (fps), and 32-bit 
        alignment padding bytes.

   2. Index Table Loading:

      * Dynamically allocates heap memory to hold the index table containing 
        compressed frame sizes.

   3. Sequential Frame Verification:

      * Initializes an independent LZ4 stream decoder (LZ4_createStreamDecode).
      * For each frame:
        - Skips the fixed 768-byte palette block.
        - Reads the compressed payload chunk based on the index table size.
        - Runs LZ4_decompress_safe_continue to verify that the output yields 
          the exact uncompressed size of 64,000 bytes (BLOCK_BYTES).
      * Logs an error if stream corruption or size mismatch occurs.

   4. Log Generation (verify.log):

      * Outputs a summary to standard stdout and writes a detailed log to 
        verify.log, reporting total errors found. Returns 0 on success.

================================================================================
4. LICENSING AND DISTRIBUTION INFORMATION
--------------------------------------------------------------------------------

This package bundles components governed by distinct legal frameworks:

A. Main Software Assets (VGAPLAY & VERIFMOV):
   This software is based on earlier work by netdex (2017) and incorporates 
   hardware initialization principles originally developed by Jeff Leyda 
   (2002) and modified by Volkert de Buisonjé in the "ich2player" project. 
   Jeff Leyda released his original code under permissive terms:
   "It's free, do with it what you will. I don't particularly care."
   To maintain pure MIT compatibility for VGAPlay, the file printhex.asm 
   (originally shared under CC BY-SA 3.0) has been completely excluded from 
   this distribution.
   The VGAPlay engine, its C conversion, and all modifications are 
   distributed under the terms of the MIT License.

B. Embedded Third-Party Compression Library (LZ4 - Yann Collet):
   Integrated directly into the codebase, governed by the BSD 2-Clause 
   License.

C. External DOS Extender Component (CWSDPMI.EXE - Charles W. Sandmann):
   Exclusive property of its author. Distributed under the GNU General 
   Public License version 2 (GNU GPL v2) with special binary 
   distribution permissions granted by the author.
--------------------------------------------------------------------------------
APPENDIX: FULL LICENSE TEXTS
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
MIT LICENSE (For VGAPLAY and VERIFMOV)
--------------------------------------------------------------------------------

Copyright (c) 2026 Kitsune64 on Github.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
URL of the VGAPlay Project page on Github:
https://github.com/Kitsune64/VGAPlay

--------------------------------------------------------------------------------
BSD 2-CLAUSE LICENSE (For the embedded LZ4 decompression code)
--------------------------------------------------------------------------------

Copyright (C) 2011-2017, Yann Collet.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

--------------------------------------------------------------------------------
DOS EXTENDER LICENSE COMPLIANCE (For CWSDPMI.EXE)
--------------------------------------------------------------------------------

Copyright (C) 1995-2010 Charles W. Sandmann

This component is licensed under the GNU General Public License v2.
In compliance with the author's specific distribution terms (detailed 
in COPYING.CWS), you are hereby notified that you have the right to 
receive the original source code, documentation, and official binary 
updates for the CWSDPMI server component. 
You can download the full original source code package (csdpmi7s.zip) 
from the official FreeDOS mirror repository at URL:
https://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/util/system/cwsdpmi/
