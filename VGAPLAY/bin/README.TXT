========================================================================
       VGAPLAY & VERIFMOV (for DOS) - USER GUIDE AND LICENSES
========================================================================

1. SYSTEM REQUIREMENTS
------------------------------------------------------------------------
- Compatible Intel 80386 processor (or higher).
- MS-DOS 5.0+, FreeDOS, or a compatible DOS environment.
- A DPMI server (DOS Protected Mode Interface). The standard server 
  "CWSDPMI.EXE" must be present in the folder.


2. PROGRAM USAGE
------------------------------------------------------------------------

--- VGAPLAY.EXE ---
Synchronized cinematic player for video (VIDEO.BIN) and audio (AUDIO.WAV) 
streams in standard VGA mode (320x200, 8-bit).

- Default usage:
  VGAPLAY.EXE
  (Automatically searches for "VIDEO.BIN" and "AUDIO.WAV" in the directory).

- Controls during playback:
  * [ESC]         : Interrupt and skip the cinematic.
  * [LEFT SHIFT]  : Enable frame-skipping / turbo mode.
  * [RIGHT SHIFT] : Enable frame-skipping / turbo mode.
  * [L-CONTROL]   : Enable frame-skipping / turbo mode.


--- VERIFMOV.EXE ---
Command-line diagnostic tool used to verify the structural integrity 
of video streams and generate a report (verify.log).

- Usage:
  VERIFMOV.EXE <path_to_video.bin>


3. LICENSING AND DISTRIBUTION INFORMATION
------------------------------------------------------------------------

This package bundles components governed by distinct legal frameworks:

A. Main Software Assets (VGAPLAY & VERIFMOV):
   Distributed under the terms of the MIT License.
   
B. Embedded Third-Party Compression Library (LZ4 - Yann Collet):
   Integrated into the code, governed by the BSD 2-Clause License.
   
C. External DOS Extender Component (CWSDPMI.EXE - Charles W. Sandmann):
   Exclusive property of its author. Not covered by the MIT license, 
   distributed under its original BSD-style license.


------------------------------------------------------------------------
APPENDIX: FULL LICENSE TEXTS
------------------------------------------------------------------------

------------------------------------------------------------------------
MIT LICENSE (For VGAPLAY and VERIFMOV)
------------------------------------------------------------------------
Copyright (c) 2026 Kitsune64 on Github

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
DOS EXTENDER LICENSE COMPLIANCE (For CWSDPMI.EXE)
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
