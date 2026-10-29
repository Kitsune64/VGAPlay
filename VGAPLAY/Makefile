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
VERIFY_OBJ = $(COMMON_OBJ) $(ODIR)/verify.o

# Cible par défaut : avec la vraie liste de fichiers de doc
all: $(BDIR)/$(EXEC_FILE) $(BDIR)/$(VERIFY_FILE) $(BDIR)\CWSDPMI.EXE $(BDIR)\DOC\COPYING.CWS $(BDIR)\DOC\CSDPMI7B.VER $(BDIR)\DOC\CWSDPMI.DOC $(BDIR)\DOC\CWSPARAM.DOC

# Règle générique de compilation des .o
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compilation du jeu principal
$(BDIR)/$(EXEC_FILE): $(MAIN_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Compilation du vérificateur de vidéo
$(BDIR)/$(VERIFY_FILE): $(VERIFY_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Règle pour copier CWSDPMI.EXE
$(BDIR)\CWSDPMI.EXE: 3rdparty\BIN\CWSDPMI.EXE
	copy /Y 3rdparty\BIN\CWSDPMI.EXE $(BDIR)\CWSDPMI.EXE

# Règle corrigée : va chercher le readme.txt dans VOTRE dossier doc/ local
$(BDIR)\README.TXT: doc\README.TXT
	copy /Y doc\README.TXT $(BDIR)\README.TXT
	
# Règle DOS pour créer le dossier bin\DOC s'il n'existe pas
$(BDIR)\DOC:
	if not exist $(BDIR)\DOC mkdir $(BDIR)\DOC

# Règles de copie pour les 4 fichiers de DOC réels
$(BDIR)\DOC\COPYING.CWS: 3rdparty\DOC\CWSDPMI\COPYING.CWS | $(BDIR)\DOC
	copy /Y 3rdparty\DOC\CWSDPMI\COPYING.CWS $(BDIR)\DOC\COPYING.CWS

$(BDIR)\DOC\CSDPMI7B.VER: 3rdparty\DOC\CWSDPMI\CSDPMI7B.VER | $(BDIR)\DOC
	copy /Y 3rdparty\DOC\CWSDPMI\CSDPMI7B.VER $(BDIR)\DOC\CSDPMI7B.VER

$(BDIR)\DOC\CWSDPMI.DOC: 3rdparty\DOC\CWSDPMI\CWSDPMI.DOC | $(BDIR)\DOC
	copy /Y 3rdparty\DOC\CWSDPMI\CWSDPMI.DOC $(BDIR)\DOC\CWSDPMI.DOC

$(BDIR)\DOC\CWSPARAM.DOC: 3rdparty\DOC\CWSDPMI\CWSPARAM.DOC | $(BDIR)\DOC
	copy /Y 3rdparty\DOC\CWSDPMI\CWSPARAM.DOC $(BDIR)\DOC\CWSPARAM.DOC

.PHONY: clean all

clean:
	-del $(ODIR)\*.o
	-del $(BDIR)\*.EXE
	-del $(BDIR)\README.TXT
	@rem Suppression directe du contenu et du dossier (le tiret ignore les alertes DOS)
	-del /F /Q bin\DOC\*.*
	-rmdir /S /Q bin\DOC

