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
WINDOWSXP_DIR = $(BASE_DIR)/WinXP

WIN32_FFMPEG_DIR = ./ffmpeg/win32
WIN64_FFMPEG_DIR = ./ffmpeg/win64
WINXP_FFMPEG_DIR = ./ffmpeg/winxp

SCRIPT_NAME = convert.sh
CFLAGS = -O3 -Iinclude -include zlib.h -DPNG_NO_CONFIG_H -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wno-discarded-qualifiers
CFLAGS_XP32 = $(CFLAGS) -D_WIN32_WINNT=0x0501 -D__MSVCRT_VERSION__=0x0600 -mno-stack-arg-probe
STATIC_GCC_FLAGS = -static-libgcc -static-libstdc++ -Wl,-Bstatic -lgcc -lgcc_eh -Wl,-Bdynamic

FFMPEG_LIBS = libavformat libavcodec libswscale libswresample libavutil libpng
LINUX_FFMPEG_CFLAGS  = $(shell pkg-config --cflags $(FFMPEG_LIBS) 2>/dev/null)
LINUX_FFMPEG_LDFLAGS = $(shell pkg-config --libs $(FFMPEG_LIBS) 2>/dev/null)
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
EXE_MPG2WAV_WIN64 = $(WINDOWS64_DIR)/mpg2wav.exe

EXE_WINXP = $(WINDOWSXP_DIR)/FramePack.exe
EXE_VERIF_WINXP = $(WINDOWSXP_DIR)/Verificator.exe
EXE_EXTRACT_WINXP = $(WINDOWSXP_DIR)/PNGExtract.exe
EXE_MPG2WAV_WINXP = $(WINDOWSXP_DIR)/mpg2wav.exe

.PHONY: all suite linux windows32 windows64 windowsxp clean
all: suite
suite: linux windows32 windows64 windowsxp

$(LINUX_DIR):
	mkdir -p $(LINUX_DIR)
$(WINDOWS32_DIR):
	mkdir -p $(WINDOWS32_DIR)
$(WINDOWS64_DIR):
	mkdir -p $(WINDOWS64_DIR)
$(WINDOWSXP_DIR):
	mkdir -p $(WINDOWSXP_DIR)

linux: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(LINUX_DIR)
	$(CC) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_LINUX) -lm
	$(CC) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_LINUX) -lm
	$(CC) $(CFLAGS) $(LINUX_FFMPEG_CFLAGS) PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_LINUX) $(LINUX_FFMPEG_LDFLAGS) -lm
	$(CC) $(CFLAGS) $(LINUX_FFMPEG_CFLAGS) mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_LINUX) $(LINUX_FFMPEG_LDFLAGS) -lm
	@if [ -f "script/Linux/$(SCRIPT_NAME)" ]; then cp "script/Linux/$(SCRIPT_NAME)" "$(LINUX_DIR)/" && chmod +x "$(LINUX_DIR)/$(SCRIPT_NAME)"; fi
	@if [ -f "doc/ReadMe.txt" ]; then cp "doc/ReadMe.txt" "$(LINUX_DIR)/"; fi
	@if [ -f "samples/demo.mp4" ]; then cp "samples/demo.mp4" "$(LINUX_DIR)/"; fi
	@if [ -f "LICENSE" ]; then cp "LICENSE" "$(LINUX_DIR)/"; fi

windows32: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWS32_DIR)
	$(CROSS_CC_32) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_WIN32) $(STATIC_GCC_FLAGS) -lm
	$(CROSS_CC_32) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WIN32) $(STATIC_GCC_FLAGS) -lm
	$(CROSS_CC_32) $(CFLAGS) $(STATIC_GCC_FLAGS) -I$(WIN32_FFMPEG_DIR)/include PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_WIN32) -L$(WIN32_FFMPEG_DIR)/lib -lavformat -lavcodec -lswscale -lavutil -lm
	$(CROSS_CC_32) $(CFLAGS) $(STATIC_GCC_FLAGS) -I$(WIN32_FFMPEG_DIR)/include mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_WIN32) -L$(WIN32_FFMPEG_DIR)/lib -lavformat -lavcodec -lswresample -lavutil -lm
	@if [ -f "doc/ReadMe.txt" ]; then cp "doc/ReadMe.txt" "$(WINDOWS32_DIR)/"; fi
	@if [ -d "$(WIN32_FFMPEG_DIR)/bin" ]; then cp $(WIN32_FFMPEG_DIR)/bin/*.dll $(WINDOWS32_DIR)/; fi
	@if [ -f "script/WinXP/convert.bat" ]; then cp "script/WinXP/convert.bat" "$(WINDOWS32_DIR)/"; fi
	@if [ -f "samples/demo.mp4" ]; then cp "samples/demo.mp4" "$(WINDOWS32_DIR)/"; fi
	@if [ -f "LICENSE" ]; then cp "LICENSE" "$(WINDOWS32_DIR)/"; fi
	@if [ -f "$(WIN32_FFMPEG_DIR)/LICENSE.md" ] || [ -f "$(WIN32_FFMPEG_DIR)/LICENSE.txt" ] || [ -f "$(WIN32_FFMPEG_DIR)/LICENSE" ]; then mkdir -p "$(WINDOWS32_DIR)/doc"; fi
	@if [ -f "$(WIN32_FFMPEG_DIR)/LICENSE.md" ]; then cp "$(WIN32_FFMPEG_DIR)/LICENSE.md" "$(WINDOWS32_DIR)/doc/FFMPEG_LICENSE.md"; \
	elif [ -f "$(WIN32_FFMPEG_DIR)/LICENSE.txt" ]; then cp "$(WIN32_FFMPEG_DIR)/LICENSE.txt" "$(WINDOWS32_DIR)/doc/FFMPEG_LICENSE.txt"; \
	elif [ -f "$(WIN32_FFMPEG_DIR)/LICENSE" ]; then cp "$(WIN32_FFMPEG_DIR)/LICENSE" "$(WINDOWS32_DIR)/doc/FFMPEG_LICENSE"; fi

windows64: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWS64_DIR)
	$(CROSS_CC_64) $(CFLAGS) FramePack.c $(SRCS_LIBS) -o $(EXE_WIN64) $(STATIC_GCC_FLAGS) -lm
	$(CROSS_CC_64) $(CFLAGS) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WIN64) $(STATIC_GCC_FLAGS) -lm
	$(CROSS_CC_64) $(CFLAGS) $(STATIC_GCC_FLAGS) -I$(WIN64_FFMPEG_DIR)/include PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_WIN64) -L$(WIN64_FFMPEG_DIR)/lib -lavformat -lavcodec -lswscale -lavutil -lm
	$(CROSS_CC_64) $(CFLAGS) $(STATIC_GCC_FLAGS) -I$(WIN64_FFMPEG_DIR)/include mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_WIN64) -L$(WIN64_FFMPEG_DIR)/lib -lavformat -lavcodec -lswresample -lavutil -lm
	@if [ -f "doc/ReadMe.txt" ]; then cp "doc/ReadMe.txt" "$(WINDOWS64_DIR)/"; fi
	@if [ -d "$(WIN64_FFMPEG_DIR)/bin" ]; then cp $(WIN64_FFMPEG_DIR)/bin/*.dll $(WINDOWS64_DIR)/; fi
	@if [ -f "script/WinXP/convert.bat" ]; then cp "script/WinXP/convert.bat" "$(WINDOWS64_DIR)/"; fi
	@if [ -f "samples/demo.mp4" ]; then cp "samples/demo.mp4" "$(WINDOWS64_DIR)/"; fi
	@if [ -f "LICENSE" ]; then cp "LICENSE" "$(WINDOWS64_DIR)/"; fi
	@if [ -f "$(WIN64_FFMPEG_DIR)/LICENSE.md" ] || [ -f "$(WIN64_FFMPEG_DIR)/LICENSE.txt" ] || [ -f "$(WIN64_FFMPEG_DIR)/LICENSE" ]; then mkdir -p "$(WINDOWS64_DIR)/doc"; fi
	@if [ -f "$(WIN64_FFMPEG_DIR)/LICENSE.md" ]; then cp "$(WIN64_FFMPEG_DIR)/LICENSE.md" "$(WINDOWS64_DIR)/doc/FFMPEG_LICENSE.md"; \
	elif [ -f "$(WIN64_FFMPEG_DIR)/LICENSE.txt" ]; then cp "$(WIN64_FFMPEG_DIR)/LICENSE.txt" "$(WINDOWS64_DIR)/doc/FFMPEG_LICENSE.txt"; \
	elif [ -f "$(WIN64_FFMPEG_DIR)/LICENSE" ]; then cp "$(WIN64_FFMPEG_DIR)/LICENSE" "$(WINDOWS64_DIR)/doc/FFMPEG_LICENSE"; fi

windowsxp: FramePack.c verificator.c PNGExtract.c mpg2wav.c $(SRCS_LIBS) | $(WINDOWSXP_DIR)
	@echo "Compiling Windows XP 32-bit shared versions into $(WINDOWSXP_DIR)..."
	$(CROSS_CC_32) $(CFLAGS_XP32) FramePack.c $(SRCS_LIBS) -o $(EXE_WINXP) $(STATIC_GCC_FLAGS) -lm
	$(CROSS_CC_32) $(CFLAGS_XP32) verificator.c $(SRCS_LIBS) -o $(EXE_VERIF_WINXP) $(STATIC_GCC_FLAGS) -lm
	$(CROSS_CC_32) $(CFLAGS_XP32) $(STATIC_GCC_FLAGS) -mthreads -I$(WINXP_FFMPEG_DIR)/include PNGExtract.c $(SRCS_LIBS) -o $(EXE_EXTRACT_WINXP) -L$(WINXP_FFMPEG_DIR)/lib -lavformat -lavcodec -lswscale -lavutil -lm
	$(CROSS_CC_32) $(CFLAGS_XP32) $(STATIC_GCC_FLAGS) -mthreads -I$(WINXP_FFMPEG_DIR)/include mpg2wav.c $(SRCS_LIBS) -o $(EXE_MPG2WAV_WINXP) -L$(WINXP_FFMPEG_DIR)/lib -lavformat -lavcodec -lswresample -lavutil -lm
	@if [ -f "doc/ReadMe.txt" ]; then cp "doc/ReadMe.txt" "$(WINDOWSXP_DIR)/"; fi
	@if [ -d "$(WINXP_FFMPEG_DIR)/bin" ]; then cp $(WINXP_FFMPEG_DIR)/bin/*.dll $(WINDOWSXP_DIR)/; fi
	@if [ -f "deps/WinXP/libgcc_s_dw2-1.dll" ]; then cp "deps/WinXP/libgcc_s_dw2-1.dll" "$(WINDOWSXP_DIR)/"; fi
	@if [ -f "script/WinXP/convert.bat" ]; then cp "script/WinXP/convert.bat" "$(WINDOWSXP_DIR)/"; fi
	@if [ -f "samples/demo.mp4" ]; then cp "samples/demo.mp4" "$(WINDOWSXP_DIR)/"; fi
	@if [ -f "LICENSE" ]; then cp "LICENSE" "$(WINDOWSXP_DIR)/"; fi
	@if [ -f "$(WINXP_FFMPEG_DIR)/LICENSE.md" ] || [ -f "$(WINXP_FFMPEG_DIR)/LICENSE.txt" ] || [ -f "$(WINXP_FFMPEG_DIR)/LICENSE" ]; then mkdir -p "$(WINDOWSXP_DIR)/doc"; fi
	@if [ -f "$(WINXP_FFMPEG_DIR)/LICENSE.md" ]; then cp "$(WINXP_FFMPEG_DIR)/LICENSE.md" "$(WINDOWSXP_DIR)/doc/FFMPEG_LICENSE.md"; \
	elif [ -f "$(WINXP_FFMPEG_DIR)/LICENSE.txt" ]; then cp "$(WINXP_FFMPEG_DIR)/LICENSE.txt" "$(WINDOWSXP_DIR)/doc/FFMPEG_LICENSE.txt"; \
	elif [ -f "$(WINXP_FFMPEG_DIR)/LICENSE" ]; then cp "$(WINXP_FFMPEG_DIR)/LICENSE" "$(WINDOWSXP_DIR)/doc/FFMPEG_LICENSE"; fi

clean:
	@echo "Cleaning up all compiled executables..."
	rm -rf $(BASE_DIR)
