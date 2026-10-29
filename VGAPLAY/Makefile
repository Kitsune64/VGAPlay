EXEC_FILE=VGAPLAY.EXE
VERIFY_FILE=VERIFMOV.EXE

SDIR=src
ODIR=obj
LDIR=lib
BDIR=bin
IDIR=include

CC=gcc
CFLAGS=-std=gnu99 -I$(IDIR) -O3 -fomit-frame-pointer -march=i486 -Werror -Wall
LIBS=-lm -lalleg

DEPS = $(wildcard $(IDIR)/*.h)

# Cleaned common files
COMMON_SOURCES = decoder.c fileio.c util.c wav.c lz4.c error.c
COMMON_OBJ = $(addprefix $(ODIR)/, $(COMMON_SOURCES:.c=.o))

# Specific objects for each executable
MAIN_OBJ = $(COMMON_OBJ) $(ODIR)/main.o
VERIFY_OBJ = $(COMMON_OBJ) $(ODIR)/verify.o

# Default target: README.TXT and documentation files are properly included here
all: $(BDIR)/$(EXEC_FILE) $(BDIR)/$(VERIFY_FILE) $(BDIR)/CWSDPMI.EXE $(BDIR)/README.TXT $(BDIR)/DOC/COPYING.CWS $(BDIR)/DOC/CSDPMI7B.VER $(BDIR)/DOC/CWSDPMI.DOC $(BDIR)/DOC/CWSPARAM.DOC

# Generic rule for compiling .o files
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compilation of the main game
$(BDIR)/$(EXEC_FILE): $(MAIN_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Compilation of the video checker
$(BDIR)/$(VERIFY_FILE): $(VERIFY_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Rule to copy CWSDPMI.EXE
$(BDIR)/CWSDPMI.EXE: 3rdparty/BIN/CWSDPMI.EXE
	copy /Y 3rdparty\\BIN\\CWSDPMI.EXE bin\\CWSDPMI.EXE

# Rule for README.TXT
$(BDIR)/README.TXT: doc/README.TXT
	copy /Y doc\\README.TXT bin\\README.TXT
	
# DOS rule to create the bin/DOC directory if it does not exist
$(BDIR)/DOC:
	if not exist bin\\DOC mkdir bin\\DOC

# Copy rules for DOC files
$(BDIR)/DOC/COPYING.CWS: 3rdparty/DOC/CWSDPMI/COPYING.CWS | $(BDIR)/DOC
	copy /Y 3rdparty\\DOC\\CWSDPMI\\COPYING.CWS bin\\DOC\\COPYING.CWS

$(BDIR)/DOC/CSDPMI7B.VER: 3rdparty/DOC/CWSDPMI/CSDPMI7B.VER | $(BDIR)/DOC
	copy /Y 3rdparty\\DOC\\CWSDPMI\\CSDPMI7B.VER bin\\DOC\\CSDPMI7B.VER

$(BDIR)/DOC/CWSDPMI.DOC: 3rdparty/DOC/CWSDPMI/CWSDPMI.DOC | $(BDIR)/DOC
	copy /Y 3rdparty\\DOC\\CWSDPMI\\CWSDPMI.DOC bin\\DOC\\CWSDPMI.DOC

$(BDIR)/DOC/CWSPARAM.DOC: 3rdparty/DOC/CWSDPMI/CWSPARAM.DOC | $(BDIR)/DOC
	copy /Y 3rdparty\\DOC\\CWSDPMI\\CWSPARAM.DOC bin\\DOC\\CWSPARAM.DOC

.PHONY: clean all

clean:
	-del obj\\*.o
	-del bin\\*.EXE
	-del bin\\README.TXT
	-del /F /Q bin\\DOC\\*.*
	-rmdir /S /Q bin\\DOC
