--------------------------------------------------------------------------------
                         FRAMEPACK SUITE - Makefile README
--------------------------------------------------------------------------------

OVERVIEW
--------
This project provides the FramePack Suite, a collection of utilities including
FramePack, Verificator, PNGExtract, and mpg2wav. The provided Makefile supports
multi-platform target subdirectories, allowing compilation for Linux (native)
and Windows 32-bit/64-bit (via MinGW cross-compilation).


PREREQUISITES
-------------
To build the suite, ensure you have the following tools installed on your system:
  * GNU Make
  * GCC (for native Linux compilation)
  * MinGW-w64 toolchain (for Windows cross-compilation: i686-w64-mingw32-gcc 
    and x86_64-w64-mingw32-gcc)
  * zlib development files


BUILD TARGETS
-------------
You can compile the suite using the following commands with `make`:

  * make all (or simply make)
    Compiles everything (Linux, Windows 32-bit, and Windows 64-bit targets).

  * make linux
    Compiles native Linux binaries and places them into the `bin/Linux/` directory.
    It also copies the helper shell script (`convert.sh`) if available.

  * make windows32
    Cross-compiles 32-bit Windows executables (`.exe`) into the 
    `bin/Win32/` directory.

  * make windows64
    Cross-compiles 64-bit Windows executables (`.exe`) into the 
    `bin/Win64/` directory.

  * make clean
    Removes the `bin/` directory and all compiled binaries, cleaning up the 
    workspace.


DIRECTORY STRUCTURE
-------------------
Once built, the binaries are organized as follows:
  * bin/Linux/    -> Linux binaries & convert.sh script
  * bin/Win32/    -> Windows 32-bit executables
  * bin/Win64/    -> Windows 64-bit executables
--------------------------------------------------------------------------------
