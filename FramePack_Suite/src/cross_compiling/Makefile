# -------------------------------------------------------------------------
# Makefile for FramePack Suite - Multi-Platform Target Subdirectories
# -------------------------------------------------------------------------

CC = gcc
CROSS_CC_32 = i686-w64-mingw32-gcc
CROSS_CC_64 = x86_64-w64-mingw32-gcc

BASE_DIR = bin
LINUX_DIR = $(BASE_DIR)/Linux
WINDOWS32_DIR = $(BASE_DIR)/Win32
WINDOWS64_DIR = $(BASE_DIR)/Win64

# Nom de ton script shell
SCRIPT_NAME = convert.sh

CFLAGS = -O3 -Iinclude -include zlib.h -DPNG_NO_CONFIG_H -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wno-discarded-qualifiers

SRCS_LIBS = $(wildcard lib/*.c)

EXE_LINUX = $(LINUX_DIR)/FramePack
EXE_VERIF_LINUX = $(LINUX_DIR)/Verificator
EXE_EXTRACT_LINUX = $(LINUX_DIR)/PNGExtract
EXE_MPG2WAV_LINUX = $(LINUX_DIR)/mpg2wav

EXE_WIN32 = $(WINDOWS32_DIR)/FramePack.exe
EXE_VERIF_WIN32 = $(WINDOWS32_DIR)/Verificator.exe
EXE_EXTRACT_WIN32 = $(WINDOWS32_DIR)/PNGExtract.exe
EXE_MPG2WAV_WIN32 = $(WINDOWS32_DIR)/mpg2wav.exe

EXE_WIN64 = $(WINDOWS64_DIR)/FramePack.exe
EXE_VERIF_WIN64 = $(WINDOWS64_DIR)/Verificator.exe
EXE_EXTRACT_WIN64 = $(WINDOWS64_DIR)/PNGExtract.exe
EXE_MPG2WAV_WIN64 = $(WINDOWS64_DIR)/Win64/mpg2wav.exe # Corrigé ici aussi par cohérence

.PHONY: all suite linux windows32 windows64 clean

all: suite

suite: linux windows32 windows64

$(LINUX_DIR):
	mkdir -p $(LINUX_DIR)

$(WINDOWS32_DIR):
	mkdir -p $(WINDOWS32_DIR)

$(WINDOWS64_DIR):
	mkdir -p $(WINDOWS64_DIR)

linux: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(LINUX_DIR)
	@echo "Compiling Linux binaries into $(LINUX_DIR)..."
	$(CC) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_LINUX) -lm
	$(CC) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_LINUX) -lm
	$(CC) $(CFLAGS) PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_LINUX) -lm
	$(CC) $(CFLAGS) mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_LINUX) -lm
	@if [ -f "script/Linux/$(SCRIPT_NAME)" ]; then \
		cp "script/Linux/$(SCRIPT_NAME)" "$(LINUX_DIR)/"; \
		chmod +x "$(LINUX_DIR)/$(SCRIPT_NAME)"; \
		echo "Script $(SCRIPT_NAME) copied et make executable in $(LINUX_DIR) !"; \
	fi
	@echo "Linux binaries and script created successfully."

windows32: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWS32_DIR)
	@echo "Compiling Windows 32-bit versions into $(WINDOWS32_DIR)..."
	$(CROSS_CC_32) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_WIN32)
	$(CROSS_CC_32) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WIN32)
	$(CROSS_CC_32) $(CFLAGS) PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_WIN32)
	$(CROSS_CC_32) $(CFLAGS) mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_WIN32)
	@echo "Windows 32-bit versions (.exe) created successfully."

windows64: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWS64_DIR)
	@echo "Compiling Windows 64-bit versions into $(WINDOWS64_DIR)..."
	$(CROSS_CC_64) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_WIN64)
	$(CROSS_CC_64) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WIN64)
	$(CROSS_CC_64) $(CFLAGS) PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_WIN64)
	$(CROSS_CC_64) $(CFLAGS) mpg2wav.c $(SRCS_LIBS) -o $(WINDOWS64_DIR)/mpg2wav.exe
	@echo "Windows 64-bit versions (.exe) created successfully."

clean:
	@echo "Cleaning up all compiled executables..."
	rm -rf $(BASE_DIR)
	@echo "Cleanup complete."
