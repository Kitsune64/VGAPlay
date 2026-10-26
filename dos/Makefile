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

# Fichiers communs nettoyés (sans font.c, keyboard.c, ni vga.c)
COMMON_SOURCES = decoder.c fileio.c util.c wav.c lz4.c error.c
COMMON_OBJ = $(addprefix $(ODIR)/, $(COMMON_SOURCES:.c=.o))

# Objets spécifiques pour chaque exécutable
MAIN_OBJ = $(COMMON_OBJ) $(ODIR)/main.o
VERIFY_OBJ = $(COMMON_OBJ) $(ODIR)/verify_v.o

# Cible par défaut : compile les deux programmes à la fois
all: $(BDIR)/$(EXEC_FILE) $(BDIR)/$(VERIFY_FILE)

# Règle générique de compilation des .o
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compilation du jeu principal
$(BDIR)/$(EXEC_FILE): $(MAIN_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Compilation du vérificateur de vidéo
$(BDIR)/$(VERIFY_FILE): $(VERIFY_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	del $(ODIR)\*.o
	del $(BDIR)\*.exe
