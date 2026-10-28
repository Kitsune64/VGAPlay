========================================================================
                       VGAPLAY & VERIFMOV for DOS
========================================================================

DESCRIPTION
===========
This package contains two essential utilities for the DOS environment:

1. VGAPLAY.EXE  - A synchronized cinematic player capable of streaming
                  video (VIDEO.BIN) and audio (AUDIO.WAV) tracks
                  in standard VGA mode (320x200, 8-bit).
                  
2. VERIFMOV.EXE - A command-line diagnostic tool used to verify the
                  structural integrity of video streams and generate
                  a detailed report (verify.log).

Both programs execute exclusively in 32-bit protected mode. They tightly
integrate the LZ4 fast decompression algorithm for real-time operation.


SYSTEM REQUIREMENTS
===================
- Compatible Intel 80386 processor (or higher).
- MS-DOS 5.0+, FreeDOS, or a compatible DOS environment.
- A DPMI server (DOS Protected Mode Interface).

For technical reasons, these programs CANNOT run without an active DPMI
host. For your convenience, the standard server "CWSDPMI.EXE" is 
included  in the root folder of this archive to allow immediate 
"out-of-the-box" execution.


PROGRAM USAGE
=============

Running VGAPLAY:
----------------
Simply execute the binary from your DOS prompt. It does not require any 
command-line arguments:
  VGAPLAY.EXE

NOTE: VGAPLAY expects to find both "VIDEO.BIN" and "AUDIO.WAV" inside 
the exact same directory as the executable. If these files are missing, 
the player will terminate immediately.

Runtime Controls:
- [ESC]         : Interrupt playback instantly and skip the cinematic.
- [LEFT SHIFT]  : Hold down to enable frame-skipping / turbo mode.
- [RIGHT SHIFT] : Hold down to enable frame-skipping / turbo mode.
- [L-CONTROL]   : Hold down to enable frame-skipping / turbo mode.

Running VERIFMOV:
-----------------
Provide the path to the binary video file you wish to analyze:
  VERIFMOV.EXE <path_to_video.bin>

This will read the global header parameters, check the RAM index table
boundaries, validate every sequential LZ4 frame block, and output a 
detailed log report directly to a file named "verify.log".


LICENSING & DISTRIBUTION MIX
============================
This package bundles components governed by distinct legal frameworks.
Please read the following compliance notice carefully.

1. Main Software Assets (VGAPLAY & VERIFMOV):
   The executable binaries VGAPLAY.EXE and VERIFMOV.EXE, alongside their
   respective source codes, are distributed under the terms of the MIT 
   License (see full text below).

2. Embedded Third-Party Compression Library (LZ4):
   The decompression engine embedded within our binaries is built upon
   the LZ4 core algorithm created by Yann Collet, which is governed by 
   the BSD 2-Clause License (included below).

3. External DOS Extender Component (CWSDPMI.EXE):
   This mandatory binary server file remains the exclusive property of 
   Charles W. Sandmann. It IS NOT covered by the MIT License. It is 
   freely bundled here under its original permissive BSD-style license.


------------------------------------------------------------------------
MIT LICENSE (For VGAPLAY and VERIFMOV)
------------------------------------------------------------------------
Copyright (c) 2026 [Your Name or Organization]

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


------------------------------------------------------------------------
BSD 2-CLAUSE LICENSE (For the embedded LZ4 decompression code)
------------------------------------------------------------------------
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


------------------------------------------------------------------------
DOS EXTENDER LICENSE & DISTRIBUTION COMPLIANCE (For CWSDPMI.EXE)
------------------------------------------------------------------------
Copyright (c) 1995-2010 Charles W. Sandmann (sandmann@clio.rice.edu)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

NOTICE TO END USERS:
In compliance with the author's terms, you are hereby notified that you 
have the right to receive the original source code, documentation, and 
official binary updates for the CWSDPMI server component. You can 
download the full source code package from the author page at URL:
http://sandmann.dotster.com/cwsdpmi/

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.
========================================================================
