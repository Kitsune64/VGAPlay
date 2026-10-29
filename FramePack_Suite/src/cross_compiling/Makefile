# -------------------------------------------------------------------------
# Makefile for FramePack Suite - Multi-Platform Target Subdirectories
# -------------------------------------------------------------------------

# Compilers
CC = gcc
CROSS_CC_32 = i686-w64-mingw32-gcc
CROSS_CC_64 = x86_64-w64-mingw32-gcc

# Target Directories
BASE_DIR = bin
LINUX_DIR = $(BASE_DIR)/Linux
WINDOWS32_DIR = $(BASE_DIR)/Win32
WINDOWS64_DIR = $(BASE_DIR)/Win64

# Embedded Standalone FFmpeg Directories
WIN32_FFMPEG_DIR = ./ffmpeg/win32
WIN64_FFMPEG_DIR = ./ffmpeg/win64

# Name of the Shell script
SCRIPT_NAME = convert.sh

# Compilation flags
CFLAGS = -O3 -Iinclude -include zlib.h -DPNG_NO_CONFIG_H -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wno-discarded-qualifiers

# Automatic discovery of FFmpeg (including libswresample) and LibPNG flags on Linux host system
FFMPEG_LIBS = libavformat libavcodec libswscale libswresample libavutil libpng
LINUX_FFMPEG_CFLAGS  = $(shell pkg-config --cflags $(FFMPEG_LIBS) 2>/dev/null)
LINUX_FFMPEG_LDFLAGS = $(shell pkg-config --libs $(FFMPEG_LIBS) 2>/dev/null)

# Library sources
SRCS_LIBS = $(wildcard lib/*.c)

# Linux Executables
EXE_LINUX = $(LINUX_DIR)/FramePack
EXE_VERIF_LINUX = $(LINUX_DIR)/Verificator
EXE_EXTRACT_LINUX = $(LINUX_DIR)/PNGExtract
EXE_MPG2WAV_LINUX = $(LINUX_DIR)/mpg2wav

# Windows 32-bit Executables
EXE_WIN32 = $(WINDOWS32_DIR)/FramePack.exe
EXE_VERIF_WIN32 = $(WINDOWS32_DIR)/Verificator.exe
EXE_EXTRACT_WIN32 = $(WINDOWS32_DIR)/PNGExtract.exe
EXE_MPG2WAV_WIN32 = $(WINDOWS32_DIR)/mpg2wav.exe

# Windows 64-bit Executables
EXE_WIN64 = $(WINDOWS64_DIR)/FramePack.exe
EXE_VERIF_WIN64 = $(WINDOWS64_DIR)/Verificator.exe
EXE_EXTRACT_WIN64 = $(WINDOWS64_DIR)/PNGExtract.exe
EXE_MPG2WAV_WIN64 = $(WINDOWS64_DIR)/mpg2wav.exe

.PHONY: all suite linux windows32 windows64 clean

all: suite

suite: linux windows32 windows64

$(LINUX_DIR):
	mkdir -p $(LINUX_DIR)

$(WINDOWS32_DIR):
	mkdir -p $(WINDOWS32_DIR)

$(WINDOWS64_DIR):
	mkdir -p $(WINDOWS64_DIR)

# -------------------------------------------------------------------------
# Linux Compilation Target
# -------------------------------------------------------------------------
linux: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(LINUX_DIR)
	@echo "Compiling Linux binaries into $(LINUX_DIR)..."
	$(CC) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_LINUX) -lm
	$(CC) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_LINUX) -lm
	$(CC) $(CFLAGS) $(LINUX_FFMPEG_CFLAGS) PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_LINUX) $(LINUX_FFMPEG_LDFLAGS) -lm
	$(CC) $(CFLAGS) $(LINUX_FFMPEG_CFLAGS) mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_LINUX) $(LINUX_FFMPEG_LDFLAGS) -lm
	@if [ -f "script/Linux/$(SCRIPT_NAME)" ]; then \
		cp "script/Linux/$(SCRIPT_NAME)" "$(LINUX_DIR)/"; \
		chmod +x "$(LINUX_DIR)/$(SCRIPT_NAME)"; \
		echo "Script $(SCRIPT_NAME) copied and made executable in $(LINUX_DIR) !"; \
	fi
	@if [ -f "doc/ReadMe.txt" ]; then \
		cp "doc/ReadMe.txt" "$(LINUX_DIR)/"; \
		echo "ReadMe.txt copied to $(LINUX_DIR) !"; \
	fi
	@echo "Linux binaries and script created successfully."

# -------------------------------------------------------------------------
# Windows 32-bit Compilation Target
# -------------------------------------------------------------------------
windows32: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWS32_DIR)
	@echo "Compiling Windows 32-bit versions into $(WINDOWS32_DIR)..."
	$(CROSS_CC_32) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_WIN32) -lm
	$(CROSS_CC_32) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WIN32) -lm
	$(CROSS_CC_32) $(CFLAGS) -I$(WIN32_FFMPEG_DIR)/include PNGExtract.c $(SRCS_LIBS) -o $(EXE_WIN32) -L$(WIN32_FFMPEG_DIR)/lib -lavformat -lavcodec -lswscale -lavutil -lm
	$(CROSS_CC_32) $(CFLAGS) -I$(WIN32_FFMPEG_DIR)/include mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_WIN32) -L$(WIN32_FFMPEG_DIR)/lib -lavformat -lavcodec -lswresample -lavutil -lm
	@if [ -f "doc/ReadMe.txt" ]; then \
		cp "doc/ReadMe.txt" "$(WINDOWS32_DIR)/"; \
		echo "ReadMe.txt copied to $(WINDOWS32_DIR) !"; \
	fi
	@echo "Copying Win32 FFmpeg shared runtime DLLs..."
	@cp $(WIN32_FFMPEG_DIR)/bin/*.dll $(WINDOWS32_DIR)/
	@echo "Windows 32-bit standalone suite package built successfully."

# -------------------------------------------------------------------------
# Windows 64-bit Compilation Target
# -------------------------------------------------------------------------
windows64: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWS64_DIR)
	@echo "Compiling Windows 64-bit versions into $(WINDOWS64_DIR)..."
	$(CROSS_CC_64) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_WIN64) -lm
	$(CROSS_CC_64) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WIN64) -lm
	$(CROSS_CC_64) $(CFLAGS) -I$(WIN64_FFMPEG_DIR)/include PNGExtract.c $(SRCS_LIBS) -o $(EXE_WIN64) -L$(WIN64_FFMPEG_DIR)/lib -lavformat -lavcodec -lswscale -lavutil -lm
	$(CROSS_CC_64) $(CFLAGS) -I$(WIN64_FFMPEG_DIR)/include mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_WIN64) -L$(WIN64_FFMPEG_DIR)/lib -lavformat -lavcodec -lswresample -lavutil -lm
	@if [ -f "doc/ReadMe.txt" ]; then \
		cp "doc/ReadMe.txt" "$(WINDOWS64_DIR)/"; \
		echo "ReadMe.txt copied to $(WINDOWS64_DIR) !"; \
	fi
	@echo "Copying Win64 FFmpeg shared runtime DLLs..."
	@cp $(WIN64_FFMPEG_DIR)/bin/*.dll $(WINDOWS64_DIR)/
	@echo "Windows 64-bit standalone suite package built successfully."

# -------------------------------------------------------------------------
# Cleanup Target
# -------------------------------------------------------------------------
clean:
	@echo "Cleaning up all compiled executables and shared bundles..."
	rm -rf $(BASE_DIR)
	@echo "Cleanup complete."
