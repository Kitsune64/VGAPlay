================================================================================
                                VGAPLAY (V1.4)
================================================================================
1. PROJECT OVERVIEW
--------------------------------------------------------------------------------

VGAPlay is a high-performance cinematic video stream player optimized for 16-bit
and 32-bit DOS environments (compiled using DJGPP). It streams and plays LZ4-
compressed video sequences in standard VGA graphics mode (Mode 13h: 320x200 /
256 colors) with tight, continuous hardware-based audio synchronization.

Major Features & Changes in Version 1.4:

  * Zero External Dependencies: All graphics, text mode switches, keyboard 
    handling, and audio drivers are implemented directly in native C (BIOS 
    interrupt 0x10, direct port I/O via conio.h, and DPMI memory transfer via 
    dosmemput).

  * Dual Hardware Audio Drivers: Native PCI Bus Mastering driver for AC'97 
    (Intel ICH chipset family) and 16-bit auto-init DMA driver for Sound 
    Blaster 16. Includes an automated software fallback clock (uclock) when no 
    audio hardware is detected.

  * Command-Line Version & Diagnostic Logging: Added native flags (-v/--version 
    and -log) for quick version queries and detailed real-time timing analysis.

  * Clean MIT Purity: Permissive licensing with full exclusion of copyleft or 
    restrictive third-party assembly wrappers.

2. MAIN PROGRAM: VGAPLAY.EXE (main.c)
--------------------------------------------------------------------------------

VGAPLAY.EXE is the primary video playback engine. It manages stream decoding,
VGA rendering, user input, and real-time audio synchronization.

A. Command Line Options & Usage:

   VGAPLAY.EXE <filename> [options]

   Arguments:

   <filename>   : Base name of the cinematic files without extension. The 
                  player automatically searches for <filename>.BIN (video) 
                  and <filename>.WAV (audio).

   Options:

   -v, --version: Prints program version ("VGAPLAY version 1.4") and exits.

   -idx         : Generates or rebuilds the frame-to-sample binary index file 
                  (<filename>.IDX) directly from the input files and exits.

   -log         : Enables real-time synchronization logging. Writes diagnostic 
                  timing details (time, frame, target, sample) to SYNC.LOG.

B. Program Flow & Operations:

   1. Version Check & Parameter Parsing:
      Checks for -v/--version first. Parses base filename and optional flags
      (-idx, -log).

   2. Audio Subsystem Initialization (audio_init):

      * Attempts to load custom ports/IRQ settings from CONFIG.CFG.

      * Scans PCI configuration registers for supported Intel ICH AC'97 
        controllers.

      * Scans I/O ports or reads BLASTER environment variable for SB16 hardware.

      * Falls back to silent mode (uclock software timer) if no card is found.

   3. Stream Preparation & Index Table Loading:

      * Parses the binary video header (.BIN file).

      * Reads the WAV audio header (.WAV file, 16-bit PCM format).

      * Loads the pre-generated index file (.IDX) or dynamically computes an 
        in-memory index mapping each frame to its target audio sample count.

   4. Display & Sync Loop (play_lz4_cinematic in Mode 13h):

      * Switches to VGA Mode 13h (320x200, 256 colors) via BIOS Int 10h.

      * Starts audio hardware playback (audio_play).

      * Continuously fetches current audio sample position (audio_get_position).

      * Decompresses LZ4 frames into a continuous 128 KB double-buffered 
        history ring (video_ring_buffer) to preserve decompression history.

      * Drops/skips rendering for falling-behind frames to maintain lock-step 
        audio sync.

      * Synchronizes display output with VGA vertical retrace (port 0x3DA).

      * Updates color palette via ports 0x3C8/0x3C9 and updates VRAM at 0xA0000 
        via dosmemput.

      * Monitors keyboard input via kbhit(); ESC key cancels playback cleanly.

   5. Resource Cleanup:

      * Stops audio playback (audio_stop) and frees allocated DPMI memory.

      * Closes log file (SYNC.LOG) if logging was requested with -log.

      * Restores standard MS-DOS text mode (Int 10h Mode 03h).

3. VERIFICATION UTILITY: VERIFY.EXE (verify.c)
--------------------------------------------------------------------------------

VERIFY.EXE is a standalone quality-assurance utility designed to check the 
structural integrity and decompression validity of binary video files (.BIN).

A. Usage:

   VERIFY.EXE <path_to_video.bin>

B. Functional Overview:

   * Validates global header parameters (frame count, FPS, alignment padding).

   * Loads the embedded index table into RAM.

   * Performs real-time sequential LZ4 decompression of all frames using an 
     independent stream decoder instance (LZ4_createStreamDecode).

   * Verifies that each decompressed frame strictly yields 64,000 bytes 
     (BLOCK_BYTES).

   * Prints results to stdout and logs errors into verify.log.

================================================================================
4. LICENSING AND LEGAL INFORMATION
--------------------------------------------------------------------------------

This software package combines open-source components under distinct permissive
licenses. All licensing information is grouped below.

A. Main Software Assets (VGAPLAY & VERIFY):

   VGAPLAY is developed and maintained by Kitsune64 on GitHub. It builds upon 
   earlier work by netdex (2017) and hardware initialization concepts 
   originally developed by Jeff Leyda (2002) and modified by Volkert de 
   Buisonje in the "ich2player" project. 

   To ensure full MIT compatibility, all non-permissive or restricted assembly 
   files (such as printhex.asm under CC BY-SA 3.0) have been completely removed. 
   The core application engine and utilities are licensed under the terms of 
   the MIT License.

B. Embedded Compression Engine (LZ4):

   The LZ4 decompression library (lz4.c / lz4.h) is copyright (C) 2011-2017 
   Yann Collet and is licensed under the BSD 2-Clause License.

C. DOS Extender (CWSDPMI.EXE):

   Copyright (C) 1995-2010 Charles W. Sandmann. Distributed under the GNU 
   General Public License v2 (GPLv2) with explicit binary distribution 
   permissions granted by the author.

--------------------------------------------------------------------------------
APPENDIX: FULL LICENSE TEXTS
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
MIT LICENSE (VGAPLAY & VERIFY)
--------------------------------------------------------------------------------

Copyright (c) 2026 Kitsune64 on Github.

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

GitHub Repository: https://github.com/Kitsune64/VGAPlay

--------------------------------------------------------------------------------
BSD 2-CLAUSE LICENSE (LZ4 Compression Library)
--------------------------------------------------------------------------------

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

--------------------------------------------------------------------------------
                    CWSDPMI EXTENDER LICENSE COMPLIANCE NOTICE
--------------------------------------------------------------------------------

Software Component : CWSDPMI (DPMI 0.90 Server for 32-bit DOS Executables)
Author / Copyright : Copyright (C) 1995-2010 Charles W. Sandmann
--------------------------------------------------------------------------------
1. GENERAL PUBLIC LICENSE TERMS & SPECIAL DISTRIBUTION PERMISSIONS
--------------------------------------------------------------------------------

CWSDPMI is distributed under the terms of the GNU General Public License 
version 2 (GNU GPL v2) as published by the Free Software Foundation, with 
special binary redistribution permissions granted directly by the author, 
Charles W. Sandmann.

In accordance with the author's official distribution requirements (as 
detailed in the original COPYING.CWS document provided with DJGPP binary 
toolchains):

  * Unmodified binary distributions of CWSDPMI.EXE may be bundled and 
    distributed alongside commercial or open-source software applications 
    without requiring the primary software application to be placed under 
    the GNU GPL license.

  * Distributors are obligated to notify users of their right to acquire 
    the complete original source code, documentation, and official updates 
    for the CWSDPMI binary component.

--------------------------------------------------------------------------------
2. SOURCE CODE AVAILABILITY & REPOSITORY LOCATION FOR CWSDPMI
--------------------------------------------------------------------------------

In full compliance with section 3 of the GNU General Public License (v2), the 
complete original source code package for the CWSDPMI server component 
(csdpmi7s.zip) is made publicly available for download.

You may retrieve the complete source code, compilation scripts, and original 
documentation directly from the official FreeDOS repository mirror at:

https://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/util/system/cwsdpmi/
