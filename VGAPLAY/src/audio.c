/*
 *	  audio.c - Pilotes audio natifs pour DJGPP / MS-DOS
 *
 *    Distributed under the MIT License. See README.TXT for details.
 * 
 *  - AC'97 (Intel ICH) via PCI + bus mastering (liste BDL)
 *  - Sound Blaster 16 via DMA 16 bits auto-init
 *  - Mode muet : la position est fournie par l'horloge (uclock)
 */
#include <pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <dpmi.h>
#include <sys/movedata.h>
#include "audio.h"

#define TRACE(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)

/* ------------------------------------------------------------------ */
/* Etat global                                                         */
/* ------------------------------------------------------------------ */
int audio_driver = AUDIO_NONE;          /* materiel detecte */
static int active_driver = AUDIO_NONE;  /* mode de la lecture en cours */
static int audio_initialized = 0;
static int audio_loaded = 0;
static int sb_expand = 0;   /* 1 si conversion mono -> stéréo nécessaire */
#define FILESIZE         (64 * 1024)    /* buffer circulaire de 64 Ko */
#define HALF_FILESIZE    (FILESIZE / 2)

typedef struct {
    int selector;
    unsigned long linear;
    unsigned long physical;
} DosMemory;

static FILE *audio_file = NULL;
static unsigned char audio_local_buffer[FILESIZE] __attribute__((aligned(4)));

/* Format du WAV courant */
static unsigned int  wav_rate = 22050, wav_channels = 2, wav_bits = 16;
static unsigned long data_remaining = 0;   /* octets restants dans le chunk data */
static unsigned long total_frames = 0;

/* Suivi de position */
static unsigned long half_frames = 1;      /* frames audio par demi-buffer */
static unsigned long halves_done = 0;      /* demi-buffers entierement joues */
static unsigned long last_position = 0;    /* pour garantir une position monotone */
static uclock_t      clock_start = 0;      /* mode muet */
static unsigned char ac97_mono_temp[HALF_FILESIZE];
/* ------------------------------------------------------------------ */
/* Utilitaires                                                         */
/* ------------------------------------------------------------------ */
static void wait_us(unsigned long us)
{
    uclock_t start = uclock();
    uclock_t ticks = ((uclock_t)us * UCLOCKS_PER_SEC) / 1000000ULL;
    while ((uclock() - start) < ticks) { }
}


static int allocate_dos_mem(DosMemory *mem, size_t size, int is_sb)
{
    int sel = -1;
    int paragraphs;
    int seg;
    unsigned long raw, aligned;

    mem->selector = -1;
    mem->linear = mem->physical = 0;

    if (is_sb) {
        /* Pour la Sound Blaster : on alloue le double pour sécuriser 
           l'alignement sans dépasser de la mémoire allouée (frontière 128 Ko) */
        paragraphs = (int)((size * 2 + 15) >> 4);
    } else {
        /* Pour l'AC'97 : allocation standard sans contrainte ISA */
        paragraphs = (int)((size + 15) >> 4);
    }

    seg = __dpmi_allocate_dos_memory(paragraphs, &sel);
    if (seg == -1) return 0;

    raw = (unsigned long)seg << 4;
    aligned = raw;

    if (is_sb) {
        /* Protection DMA ISA 16 bits : Empêche le buffer de franchir 128 Ko */
        if ((raw / 0x20000UL) != ((raw + size - 1) / 0x20000UL)) {
            aligned = (raw + 0x1FFFFUL) & ~0x1FFFFUL;
        }
    }

    mem->selector = sel;
    mem->physical = aligned;
    mem->linear = aligned;
    return 1;
}

static void free_dos_mem(DosMemory *mem)
{
    if (mem->selector != -1) {
        __dpmi_free_dos_memory(mem->selector);
        mem->selector = -1;
    }
}


/* Modifiez fetch_half pour accepter l'expansion mono->stéréo sur la SB également */
static void fetch_half(int expand_mono)
{
    size_t want = expand_mono ? (HALF_FILESIZE / 2) : HALF_FILESIZE;
    size_t got = 0;

    if (audio_file && data_remaining > 0) {
        unsigned char *dst = expand_mono ? (audio_local_buffer + HALF_FILESIZE)
                                         : audio_local_buffer;
        if (want > data_remaining) want = (size_t)data_remaining;
        got = fread(dst, 1, want, audio_file);
        if (got < want) data_remaining = 0;
        else data_remaining -= got;
    }

    if (expand_mono) {
        unsigned short *src = (unsigned short *)(audio_local_buffer + HALF_FILESIZE);
        unsigned short *out = (unsigned short *)audio_local_buffer;
        size_t n = got / 2, i;
        for (i = 0; i < n; i++) {
            out[2 * i]     = src[i]; /* Canal gauche */
            out[2 * i + 1] = src[i]; /* Canal droit (duplication) */
        }
        got = n * 4;
    }

    if (got < HALF_FILESIZE)
        memset(audio_local_buffer + got, 0, HALF_FILESIZE - got);
}
/* ------------------------------------------------------------------ */
/* PCI / AC'97                                                         */
/* ------------------------------------------------------------------ */
#define PCI_INDEX_PORT   0x0CF8
#define PCI_DATA_PORT    0x0CFC
#define INDEX_MASK       31
#define BUP              0x40000000UL

typedef struct {
    unsigned long ptr;
    unsigned long control;
} BdlEntry;

static unsigned short nambar = 0, nabmbar = 0;
static DosMemory bdl_mem  = { -1, 0, 0 };
static DosMemory buf1_mem = { -1, 0, 0 };
static DosMemory buf2_mem = { -1, 0, 0 };
static unsigned char ac97_next_bdl=0;
/* Suivi de la position réelle du DMA AC'97 */
static unsigned char ac97_last_civ = 0;
static unsigned long ac97_completed_bdl = 0;
static int ac97_position_valid = 0;

static int ac97_vra = 0;
static int ac97_expand = 0;     /* mono -> stereo */

static unsigned long pci_make_addr(unsigned char bus, unsigned char slot,
                                   unsigned char func, unsigned char offset)
{
    return 0x80000000UL | ((unsigned long)bus << 16) | ((unsigned long)slot << 11) |
           ((unsigned long)func << 8) | (offset & 0xFC);
}

static unsigned long pci_read_config_dword(unsigned char bus, unsigned char slot,
                                           unsigned char func, unsigned char offset)
{
    outportl(PCI_INDEX_PORT, pci_make_addr(bus, slot, func, offset));
    return inportl(PCI_DATA_PORT);
}

static void pci_write_config_dword(unsigned char bus, unsigned char slot,
                                   unsigned char func, unsigned char offset,
                                   unsigned long value)
{
    outportl(PCI_INDEX_PORT, pci_make_addr(bus, slot, func, offset));
    outportl(PCI_DATA_PORT, value);
}

static int pci_present(void)
{
    unsigned long old = inportl(PCI_INDEX_PORT);
    int ok;
    outportl(PCI_INDEX_PORT, 0x80000000UL);
    ok = (inportl(PCI_INDEX_PORT) == 0x80000000UL);
    outportl(PCI_INDEX_PORT, old);
    return ok;
}

static int ac97_codec_init(void)
{
    int i;
    unsigned long glob;

    /* Sortie du cold reset du lien AC-link (GLOB_CNT) */
    glob = inportl(nabmbar + 0x2C);
    outportl(nabmbar + 0x2C, glob | 0x02);

    /* Attente "primary codec ready" (GLOB_STA bit 8), 1 s max */
    for (i = 0; i < 1000; i++) {
        if (inportl(nabmbar + 0x30) & 0x100) break;
        wait_us(1000);
    }
    if (i == 1000) return 0;

    /* Reset du codec puis attente des blocs analogiques prets */
    outportw(nambar + 0x00, 0x0000);
    for (i = 0; i < 500; i++) {
        if ((inportw(nambar + 0x26) & 0x0F) == 0x0F) break;
        wait_us(1000);
    }

    outportw(nambar + 0x02, 0x0000);   /* volume master : max, non mute */
    outportw(nambar + 0x18, 0x0000);   /* volume PCM out : max, non mute */

    /* Variable Rate Audio : Activation obligatoire pour les taux != 48kHz */
    ac97_vra = 0;
    if (inportw(nambar + 0x28) & 0x0001) {
        outportw(nambar + 0x2A, inportw(nambar + 0x2A) | 0x0001);
        ac97_vra = 1;
    }

    /* FORÇAGE DIRECT DU TAUX DU WAV COURANT (ex: 22050 Hz) ICI */
    if (ac97_vra) {
        outportw(nambar + 0x2C, (unsigned short)wav_rate);
        wait_us(10000);
    }

    return 1;
}
static int ac97_detect(void)
{
    static const unsigned short dids[] = {
        0x2415, 0x2425, 0x2445, 0x2485, 0x24C5, 0x24D5, 0x25A6, 0x266E, 0x27DE, 0x7195
    };
    int bus, slot, func;
    size_t i;

    if (!pci_present()) return 0;

    for (bus = 0; bus < 8; bus++) {
        for (slot = 0; slot < 32; slot++) {
            for (func = 0; func < 8; func++) {   /* l'AC'97 ICH est en fonction 5 */
                unsigned long id = pci_read_config_dword(bus, slot, func, 0);
                unsigned short did;
                if ((id & 0xFFFF) != 0x8086) continue;
                did = (unsigned short)(id >> 16);
                for (i = 0; i < sizeof(dids) / sizeof(dids[0]); i++) {
                    if (did == dids[i]) {
                        unsigned long bar0 = pci_read_config_dword(bus, slot, func, 0x10);
                        unsigned long bar1 = pci_read_config_dword(bus, slot, func, 0x14);
                        unsigned long cmd;
                        if (!(bar0 & 1) || !(bar1 & 1)) break;   /* BAR non I/O */
                        nambar  = (unsigned short)(bar0 & 0xFFFC);
                        nabmbar = (unsigned short)(bar1 & 0xFFFC);
                        if (!nambar || !nabmbar) break;

                        TRACE("[TRACE] audio_init: AC'97 trouve (DID=%X bus=%d slot=%d func=%d)\n",
                              did, bus, slot, func);
                        cmd = pci_read_config_dword(bus, slot, func, 0x04);
                        /* I/O + bus master ; ne pas reecrire les bits de statut */
                        pci_write_config_dword(bus, slot, func, 0x04, (cmd & 0xFFFFUL) | 0x05);

                        if (ac97_codec_init()) return 1;
                        TRACE("[TRACE] audio_init: codec AC'97 non pret\n");
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

static int ac97_set_rate(unsigned int rate)
{
    unsigned short actual_rate;
    int i;

    if (rate < 8000 || rate > 48000)
        return 0;

    if (!ac97_vra)
        return rate == 48000;

    outportw(nambar + 0x2C, (unsigned short)rate);

    for (i = 0; i < 100; i++) {
        actual_rate = inportw(nambar + 0x2C);

        if (actual_rate == rate)
            return 1;

        wait_us(1000);
    }

    TRACE("[AC97] Frequence demandee : %u Hz\n", rate);
    TRACE("[AC97] Frequence lue : %u Hz\n", actual_rate);

    return 0;
}

static void ac97_free(void)
{
    free_dos_mem(&bdl_mem);
    free_dos_mem(&buf1_mem);
    free_dos_mem(&buf2_mem);
}


static void ac97_fill_buffer(DosMemory *mem)
{
    size_t wanted;
    size_t got = 0;
    size_t i;

    memset(audio_local_buffer, 0, FILESIZE);
    memset(ac97_mono_temp, 0, HALF_FILESIZE);

    if (wav_channels == 1) {

        wanted = HALF_FILESIZE;

        if (wanted > data_remaining)
            wanted = (size_t)data_remaining;

        if (audio_file && wanted > 0) {
            got = fread(ac97_mono_temp, 1, wanted, audio_file);
            data_remaining -= got;
        }

        {
            unsigned short *src =
                (unsigned short *)ac97_mono_temp;

            unsigned short *dst =
                (unsigned short *)audio_local_buffer;

            size_t samples = got / 2;

            for (i = 0; i < samples; i++) {
                dst[i * 2] = src[i];
                dst[i * 2 + 1] = src[i];
            }
        }

    } else {

        wanted = FILESIZE;

        if (wanted > data_remaining)
            wanted = (size_t)data_remaining;

        if (audio_file && wanted > 0) {
            got = fread(audio_local_buffer, 1, wanted, audio_file);
            data_remaining -= got;
        }
    }

    dosmemput(audio_local_buffer, FILESIZE, mem->linear);
}


static int ac97_load(void)
{
    BdlEntry bdl[32];
    int i, n;

    if (wav_bits != 16 || (wav_channels != 1 && wav_channels != 2))
        return 0;

    /* Reset du moteur PCM OUT avant de configurer le taux */
    outportb(nabmbar + 0x1B, 0x02);
    for (n = 0; n < 1000 && (inportb(nabmbar + 0x1B) & 0x02); n++)
        wait_us(10);

    if (!ac97_set_rate(wav_rate))
        return 0;

    wait_us(50000);

    /* Allocation de tampons de 64 Ko */
    if (!allocate_dos_mem(&bdl_mem, sizeof(bdl), 0) ||
        !allocate_dos_mem(&buf1_mem, FILESIZE, 0) ||
        !allocate_dos_mem(&buf2_mem, FILESIZE, 0)) {
        ac97_free();
        return 0;
    }

    /* Configuration de la liste BDL */
    for (i = 0; i < 32; i += 2) {
        bdl[i].ptr         = buf1_mem.physical;
        bdl[i].control     = BUP | (FILESIZE / 2);
        bdl[i + 1].ptr     = buf2_mem.physical;
        bdl[i + 1].control = BUP | (FILESIZE / 2);
    }

    dosmemput(bdl, sizeof(bdl), bdl_mem.linear);

    outportw(nabmbar + 0x16, 0x001C);
    outportl(nabmbar + 0x10, bdl_mem.physical);
    outportb(nabmbar + 0x15, 31); /* LVI initial = dernier descripteur */

    /* Préparation des tampons audio */
    ac97_expand = (wav_channels == 1);
    half_frames = FILESIZE / 4;

    ac97_fill_buffer(&buf1_mem);
    ac97_fill_buffer(&buf2_mem);

    ac97_next_bdl=0;
	return 1;
}


/* ------------------------------------------------------------------ */
/* Sound Blaster 16                                                    */
/* ------------------------------------------------------------------ */
static unsigned short sb_base = 0x220;
static unsigned char  sb_irq  = 7;
static unsigned char  sb_dma  = 5;      /* canal DMA 16 bits (5, 6 ou 7) */
static int env_irq = 0, env_dma16 = 0;
static DosMemory sb_dma_mem = { -1, 0, 0 };
static int current_buffer_half = 0;
static int sb_started = 0;

#define DMA2_MASK_REG      0xD4
#define DMA2_MODE_REG      0xD6
#define DMA2_CLEAR_REG     0xD8

#define SB_RESET_PORT      (sb_base + 0x06)
#define SB_READ_PORT       (sb_base + 0x0A)
#define SB_WRITE_PORT      (sb_base + 0x0C)
#define SB_READ_STATUS     (sb_base + 0x0E)
#define SB_INT16_ACK       (sb_base + 0x0F)
static int read_config_file(void)
{
    FILE *cfg = fopen("CONFIG.CFG", "r");
    char line[128];
    if (!cfg) return 0;

    while (fgets(line, sizeof(line), cfg)) {
        if (line[0] == '#' || line[0] == ';') continue;
        if (strstr(line, "SB_BASE=")) sb_base = (unsigned short)strtol(strstr(line, "SB_BASE=") + 8, NULL, 16);
        else if (strstr(line, "SB_IRQ="))  { sb_irq = (unsigned char)strtol(strstr(line, "SB_IRQ=") + 7, NULL, 10); env_irq = 1; }
        else if (strstr(line, "SB_DMA16=")) { sb_dma = (unsigned char)strtol(strstr(line, "SB_DMA16=") + 9, NULL, 10); env_dma16 = 1; }
    }
    fclose(cfg);
    if (sb_dma < 5 || sb_dma > 7) sb_dma = 5;
    return 1;
}

static int sb_dma_ports(unsigned char ch, unsigned short *addr,
                        unsigned short *count, unsigned short *page)
{
    switch (ch) {
        case 5: *addr = 0xC4; *count = 0xC6; *page = 0x8B; return 1;
        case 6: *addr = 0xC8; *count = 0xCA; *page = 0x89; return 1;
        case 7: *addr = 0xCC; *count = 0xCE; *page = 0x8A; return 1;
    }
    return 0;
}

static int sb_write_dsp(unsigned char cmd)
{
    unsigned long n;
    for (n = 0; n < 100000UL; n++) {
        if (!(inportb(SB_WRITE_PORT) & 0x80)) {
            outportb(SB_WRITE_PORT, cmd);
            return 1;
        }
    }
    return 0;
}

static int sb_read_dsp(void)
{
    unsigned long n;
    for (n = 0; n < 100000UL; n++) {
        if (inportb(SB_READ_STATUS) & 0x80) return inportb(SB_READ_PORT);
    }
    return -1;
}

static int sb_reset_dsp(void)
{
    int i;
    outportb(SB_RESET_PORT, 1);
    wait_us(100);
    outportb(SB_RESET_PORT, 0);
    for (i = 0; i < 100; i++) {
        if (inportb(SB_READ_STATUS) & 0x80) {
            if (inportb(SB_READ_PORT) == 0xAA) return 1;
        }
        wait_us(1000);
    }
    return 0;
}

/* Retourne 1 si la variable BLASTER existe */
static int parse_blaster_env(void)
{
    const char *p = getenv("BLASTER");
    if (!p) return 0;
    while (*p) {
        char t;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        t = (char)toupper((unsigned char)*p);
        if (t == 'A')      sb_base = (unsigned short)strtol(p + 1, NULL, 16);
        else if (t == 'I') { sb_irq = (unsigned char)strtol(p + 1, NULL, 10); env_irq = 1; }
        else if (t == 'H') { sb_dma = (unsigned char)strtol(p + 1, NULL, 10); env_dma16 = 1; }
        while (*p && *p != ' ' && *p != '\t') p++;   /* saute le champ entier */
    }
    return 1;
}

static int sb_try(unsigned short base)
{
    int major, minor;
    sb_base = base;
    if (!sb_reset_dsp()) return 0;
    if (!sb_write_dsp(0xE1)) return 0;            /* version du DSP */
    major = sb_read_dsp();
    minor = sb_read_dsp();
    if (major < 4) {
        TRACE("[TRACE] audio_init: DSP v%d.%d detecte, SB16 (v4+) requise\n", major, minor);
        return 0;
    }
    if (!env_dma16) {                              /* DMA 16 bits via le mixer */
        unsigned char v;
        outportb(sb_base + 4, 0x81);
        v = inportb(sb_base + 5);
        if (v & 0x20) sb_dma = 5; else if (v & 0x40) sb_dma = 6; else if (v & 0x80) sb_dma = 7;
    }
    if (!env_irq) {                                /* IRQ via le mixer */
        unsigned char v;
        outportb(sb_base + 4, 0x80);
        v = inportb(sb_base + 5);
        if (v & 0x01) sb_irq = 2; else if (v & 0x02) sb_irq = 5;
        else if (v & 0x04) sb_irq = 7; else if (v & 0x08) sb_irq = 10;
    }
    if (sb_dma < 5 || sb_dma > 7) sb_dma = 5;
    return 1;
}

static int sb_detect(void)
{
    static const unsigned short bases[] = { 0x220, 0x240, 0x260, 0x280, 0x210, 0x230, 0x250 };
    unsigned short first;
    size_t i;
    int have_env = parse_blaster_env();

    first = sb_base;
    if (sb_try(first)) return 1;
    if (!have_env) {
        for (i = 0; i < sizeof(bases) / sizeof(bases[0]); i++) {
            if (bases[i] == first) continue;
            if (sb_try(bases[i])) return 1;
        }
    }
    return 0;
}

/* Masquage de l'IRQ de la SB au PIC pendant la lecture (aucun handler installe) */
static unsigned short sb_pic_port = 0;
static unsigned char  sb_pic_bit = 0, sb_pic_saved = 0;
static int sb_pic_masked = 0;

static void sb_mask_irq(void)
{
    if (sb_irq > 15 || sb_irq == 2) return;
    if (sb_irq < 8) { sb_pic_port = 0x21; sb_pic_bit = (unsigned char)(1 << sb_irq); }
    else            { sb_pic_port = 0xA1; sb_pic_bit = (unsigned char)(1 << (sb_irq - 8)); }
    sb_pic_saved = inportb(sb_pic_port) & sb_pic_bit;
    outportb(sb_pic_port, inportb(sb_pic_port) | sb_pic_bit);
    sb_pic_masked = 1;
}

static void sb_restore_irq(void)
{
    if (!sb_pic_masked) return;
    outportb(sb_pic_port, (inportb(sb_pic_port) & (unsigned char)~sb_pic_bit) | sb_pic_saved);
    sb_pic_masked = 0;
}

static unsigned short sb_read_count(void)
{
    unsigned short a = 0, c = 0, p = 0;
    unsigned char lo, hi;
    if (!sb_dma_ports(sb_dma, &a, &c, &p)) return 0xFFFF;   /* canal invalide */
    outportb(DMA2_CLEAR_REG, 0x00);            /* reset du flip-flop */
    lo = inportb(c);
    hi = inportb(c);
    return (unsigned short)(lo | (hi << 8));
}

/* Position (en octets) dans le buffer DMA, avec lecture double anti-incoherence */
static __attribute__((unused)) unsigned long sb_dma_pos(void)
{
    unsigned short a = 0, b = 0;
    int tries;
    for (tries = 0; tries < 8; tries++) {
        a = sb_read_count();
        b = sb_read_count();
        if ((unsigned short)(a - b) <= 4 || (unsigned short)(b - a) <= 4) break;
    }
    if (b > (FILESIZE / 2) - 1) return 0;       /* terminal count / rechargement */
    return FILESIZE - ((unsigned long)b + 1) * 2;
}

static int sb_load(void)
{
    unsigned short a_port, c_port, p_port, word_count;
    unsigned long word_addr, phys;

    if (wav_bits != 16 || (wav_channels != 1 && wav_channels != 2)) {
        TRACE("[TRACE] audio_load_wav: format non supporte par la SB (16 bits mono/stereo requis)\n");
        return 0;
    }
    if (wav_rate < 5000 || wav_rate > 44100) {
        TRACE("[TRACE] audio_load_wav: frequence %u Hz hors limites SB16 (5000-44100)\n", wav_rate);
        return 0;
    }
    if (!sb_dma_ports(sb_dma, &a_port, &c_port, &p_port)) return 0;

    TRACE("[TRACE] audio_load_wav (SB natif): allocation buffer DMA\n");
    if (!allocate_dos_mem(&sb_dma_mem, FILESIZE, 1)) {
        TRACE("[AUDIO] Erreur d'allocation DPMI (SB)\n");
        return 0;
    }

	sb_expand = (wav_channels == 1);
    half_frames = HALF_FILESIZE / 4;           /* 16 bits stéréo par demi-buffer */
    current_buffer_half = 0;

    fetch_half(sb_expand);
    dosmemput(audio_local_buffer, HALF_FILESIZE, sb_dma_mem.linear);
    fetch_half(sb_expand);
    dosmemput(audio_local_buffer, HALF_FILESIZE, sb_dma_mem.linear + HALF_FILESIZE);


/* Programmation du canal DMA 16 bits (reste masque jusqu'a audio_play) */
    phys = sb_dma_mem.physical;
    word_addr = phys >> 1;
    word_count = (unsigned short)((FILESIZE / 2) - 1);

	outportb(DMA2_MASK_REG, 0x04 | (sb_dma & 3));
	outportb(DMA2_CLEAR_REG, 0x00);

	/* 0x58 = Mode auto-init écriture (transfert mémoire vers I/O) 
	   pour le canal DMA 16 bits, évitant le conflit de lecture */
	outportb(DMA2_MODE_REG, 0x58 | (sb_dma & 3));   

	outportb(a_port, word_addr & 0xFF);
	outportb(a_port, (word_addr >> 8) & 0xFF);
	outportb(p_port, (phys >> 16) & 0xFE);
	outportb(c_port, word_count & 0xFF);
	outportb(c_port, (word_count >> 8) & 0xFF);

	/* Autoriser à nouveau le canal DMA */
	outportb(DMA2_MASK_REG, sb_dma & 3);

    if (!sb_write_dsp(0x41) ||
        !sb_write_dsp((unsigned char)((wav_rate >> 8) & 0xFF)) ||
        !sb_write_dsp((unsigned char)(wav_rate & 0xFF))) {
        free_dos_mem(&sb_dma_mem);
        return 0;
    }
    TRACE("[TRACE] audio_load_wav (SB natif): pret\n");
    return 1;
}

/* ------------------------------------------------------------------ */
/* Lecture du WAV                                                      */
/* ------------------------------------------------------------------ */
static int parse_wav(FILE *f)
{
    char tag[4];
    uint32_t csize = 0, rate = 0, brate = 0, dsize = 0;
    uint16_t fmt_tag = 0, ch = 0, balign = 0, bits = 0;
    int have_fmt = 0;
    long pos, end;
    unsigned long frame_bytes;

    if (fread(tag, 1, 4, f) != 4 || memcmp(tag, "RIFF", 4) != 0) return 0;
    if (fseek(f, 4, SEEK_CUR) != 0) return 0;
    if (fread(tag, 1, 4, f) != 4 || memcmp(tag, "WAVE", 4) != 0) return 0;

    while (fread(tag, 1, 4, f) == 4 && fread(&csize, 4, 1, f) == 1) {
        if (memcmp(tag, "fmt ", 4) == 0) {
            if (csize < 16) return 0;
            if (fread(&fmt_tag, 2, 1, f) != 1 || fread(&ch, 2, 1, f) != 1 ||
                fread(&rate, 4, 1, f) != 1 || fread(&brate, 4, 1, f) != 1 ||
                fread(&balign, 2, 1, f) != 1 || fread(&bits, 2, 1, f) != 1) return 0;
            have_fmt = 1;
            if (csize > 16) fseek(f, (long)(csize - 16 + (csize & 1)), SEEK_CUR);
        } else if (memcmp(tag, "data", 4) == 0) {
            dsize = csize;
            break;
        } else {
            fseek(f, (long)(csize + (csize & 1)), SEEK_CUR);   /* chunks alignes sur 2 octets */
        }
    }
    if (!have_fmt || dsize == 0) return 0;
    if (fmt_tag != 1 && fmt_tag != 0xFFFE) return 0;           /* PCM / extensible */

    /* Ne jamais depasser la taille reelle du fichier */
    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);
    if (end <= pos) return 0;
    if (dsize > (uint32_t)(end - pos)) dsize = (uint32_t)(end - pos);

    frame_bytes = (unsigned long)ch * (bits / 8);
    if (frame_bytes == 0 || rate == 0) return 0;

    wav_rate = rate;
    wav_channels = ch;
    wav_bits = bits;
    data_remaining = dsize;
    total_frames = dsize / frame_bytes;
    return 1;
}

int audio_init(void)
{
    if (audio_initialized) return audio_driver;
    audio_initialized = 1;
    atexit(audio_stop);

    /* 1. Essai de lecture du fichier CONFIG.CFG */
    if (read_config_file()) {
        if (sb_try(sb_base)) {
            audio_driver = AUDIO_SB;
            return AUDIO_SB;
        }
    }

    /* 2. Secours : détection automatique classique */
    if (ac97_detect()) {
        audio_driver = AUDIO_AC97;
        return AUDIO_AC97;
    }

    if (sb_detect()) {
        audio_driver = AUDIO_SB;
        return AUDIO_SB;
    }

    audio_driver = AUDIO_NONE;
    return AUDIO_NONE;
}

int audio_load_wav(const char *filename, unsigned int *total_samples)
{
    audio_stop();   /* nettoie une lecture precedente eventuelle */

    TRACE("[TRACE] audio_load_wav: ouverture de %s\n", filename);
    audio_file = fopen(filename, "rb");
    if (!audio_file) {
        TRACE("[TRACE] audio_load_wav: echec d'ouverture du fichier audio\n");
        return 0;
    }
    if (!parse_wav(audio_file)) {
        TRACE("[TRACE] audio_load_wav: fichier WAV invalide\n");
        fclose(audio_file);
        audio_file = NULL;
        return 0;
    }

    *total_samples = (unsigned int)total_frames;
    halves_done = 0;
    last_position = 0;
    current_buffer_half = 0;
    active_driver = AUDIO_NONE;

    if (audio_driver == AUDIO_AC97) {
        if (ac97_load()) active_driver = AUDIO_AC97;
    } else if (audio_driver == AUDIO_SB) {
        if (sb_load()) active_driver = AUDIO_SB;
    }

    if (active_driver == AUDIO_NONE) {
        /* Mode muet : la video est cadencee par l'horloge */
        TRACE("[TRACE] audio_load_wav: lecture sans son (horloge de secours)\n");
        fclose(audio_file);
        audio_file = NULL;
    }
    audio_loaded = 1;
    return 1;
}

// Modifiez cette fonction dans votre fichier audio.c
void audio_play(void)
{
	
    clock_start = uclock(); // Initialisation de la référence temporelle

    if (!audio_loaded) return;

    if (active_driver == AUDIO_AC97) {

		/* Initialisation du suivi de position DMA */
		ac97_last_civ = inportb(nabmbar + 0x14) & INDEX_MASK;
		ac97_completed_bdl = 0;
		ac97_position_valid = 1;
		ac97_next_bdl = 0;

		/* Démarrage du moteur PCM OUT */
		outportb(nabmbar + 0x1B, 0x01);
	}
    else if (active_driver == AUDIO_SB) {
		unsigned short block = (HALF_FILESIZE / 2) - 1;   
		
		sb_mask_irq();
		outportb(DMA2_MASK_REG, sb_dma & 3);        
		
		sb_write_dsp(0xD1);             /* Allume explicitement le DAC de la SB16 */
		sb_write_dsp(0xB6);             /* Commande 16-bit auto-init */
		sb_write_dsp(0x30);             /* Stéréo, Signé */
		
		sb_write_dsp(block & 0xFF);     
		sb_write_dsp((block >> 8) & 0xFF);  
		
		sb_started = 1;
	}
}


void audio_update(void)
{
    if (!audio_loaded) return;

    if (active_driver == AUDIO_AC97) {
		unsigned short status;
		unsigned char current_civ;
		unsigned char stop_after;

		/* Lire l'etat du controleur et le descripteur courant */
		status = inportw(nabmbar + 0x16);
		current_civ = inportb(nabmbar + 0x14) & INDEX_MASK;

		/*
		 * Normalement, les descripteurs avant CIV sont termines.
		 * Si CELV est actif, le descripteur CIV est egalement termine.
		 */
		stop_after = current_civ;

		if (status & 0x02)
			stop_after = (current_civ + 1) & INDEX_MASK;

		/*
		 * Apres un CELV deja traite, CIV peut rester momentanement
		 * identique alors que CELV vient de disparaitre.
		 * Eviter alors de retraiter toute la liste.
		 */
		if (!((status & 0x02) == 0 &&
			  ac97_next_bdl == ((current_civ + 1) & INDEX_MASK)))
		{
			while (ac97_next_bdl != stop_after)
			{
				if (ac97_next_bdl & 1)
					ac97_fill_buffer(&buf2_mem);
				else
					ac97_fill_buffer(&buf1_mem);

				ac97_next_bdl =
					(ac97_next_bdl + 1) & INDEX_MASK;

				halves_done++;
			}
		}

		/*
		 * Maintenir le dernier descripteur valide juste devant
		 * le descripteur courant.
		 */
		outportb(nabmbar + 0x15,
				 (current_civ + 1) & INDEX_MASK);

		/* Acquitter les interruptions et erreurs */
		if (status & 0x04)
			outportw(nabmbar + 0x16, 0x0004);

		if (status & 0x10)
		{
			outportw(nabmbar + 0x16, 0x0010);
			printf("[AC97] Attention : erreur FIFO detectee\n");
		}
	}
    else if (active_driver == AUDIO_SB) {
        uclock_t elapsed = uclock() - clock_start;

        /* Le flux DMA joue toujours en stereo 16 bits */
        unsigned long bytes_played =
            (unsigned long)(((unsigned long long)elapsed *
            wav_rate * 2 * 2) / UCLOCKS_PER_SEC);

        int target_half =
            (bytes_played % FILESIZE) >= HALF_FILESIZE ? 1 : 0;

        if (target_half != current_buffer_half) {
            fetch_half(sb_expand);

            /*
             * Recharger la moitie qui vient d'etre jouee,
             * et surtout pas celle que le DMA lit actuellement.
             */
            dosmemput(audio_local_buffer,
                      HALF_FILESIZE,
                      sb_dma_mem.linear +
                      (target_half ? 0 : HALF_FILESIZE));

            current_buffer_half = target_half;

            /* Acquittement de l'interruption SB16 */
            inportb(SB_INT16_ACK);
        }
    }
}

int audio_get_position(void)
{
    unsigned long frames = 0;

    if (!audio_loaded)
        return -1;

    /*
     * AC'97 : lecture de la position réelle du moteur DMA.
     */
    if (active_driver == AUDIO_AC97 && ac97_position_valid) {

        unsigned char civ1, civ2;
        unsigned short picb;
        unsigned int consumed_samples;
        unsigned int tries;

        /*
         * Lecture cohérente de CIV et PICB.
         * Si CIV change pendant la lecture, on recommence.
         */
        for (tries = 0; tries < 8; tries++) {

            civ1 = inportb(nabmbar + 0x14) & INDEX_MASK;
            picb = inportw(nabmbar + 0x18);
            civ2 = inportb(nabmbar + 0x14) & INDEX_MASK;

            if (civ1 == civ2)
                break;
        }

        /*
         * CIV : descripteur actuellement en cours.
         * Chaque changement de CIV signifie qu'un
         * descripteur supplémentaire a été terminé.
         */
        if (civ1 != ac97_last_civ) {

            unsigned int delta;

            delta = (civ1 - ac97_last_civ) & INDEX_MASK;

            ac97_completed_bdl += delta;
            ac97_last_civ = civ1;
        }

        /*
         * PICB : nombre d'échantillons 16 bits restant
         * à transmettre dans le descripteur courant.
         *
         * Un descripteur contient FILESIZE / 2
         * échantillons 16 bits.
         */
        if (picb > (FILESIZE / 2))
            picb = FILESIZE / 2;

        consumed_samples = (FILESIZE / 2) - picb;

        /*
         * Deux échantillons 16 bits = une image stéréo.
         */
        frames = ac97_completed_bdl * half_frames;
        frames += consumed_samples / 2;
    }

    /*
     * SB16 et mode muet :
     * conservation de l'ancienne horloge logicielle.
     */
    else {

        uclock_t elapsed = uclock() - clock_start;

        frames = (unsigned long)(
            ((unsigned long long)elapsed * wav_rate)
            / UCLOCKS_PER_SEC
        );
    }

    /*
     * Garantir une position monotone.
     */
    if (frames < last_position)
        frames = last_position;

    last_position = frames;

    /*
     * Fin réelle de la piste.
     */
    if (total_frames > 0 && frames >= total_frames)
        return -1;

    return (int)frames;
}

void audio_stop(void)
{
    if (active_driver == AUDIO_AC97) {
        int n;
        outportb(nabmbar + 0x1B, 0x00);                   /* stoppe le bus master */
        outportb(nabmbar + 0x1B, 0x02);                   /* reset du moteur */
        for (n = 0; n < 1000 && (inportb(nabmbar + 0x1B) & 0x02); n++) wait_us(10);
        ac97_free();
    }
    else if (active_driver == AUDIO_SB) {
        if (sb_started) {
            sb_write_dsp(0xD5);                           /* pause DMA 16 bits immediate */
            sb_write_dsp(0xD9);                           /* sort du mode auto-init */
        }
        outportb(DMA2_MASK_REG, 0x04 | (sb_dma & 3));     /* masque le canal avant de liberer */
        sb_reset_dsp();
        inportb(SB_INT16_ACK);
        sb_restore_irq();
        sb_started = 0;
        free_dos_mem(&sb_dma_mem);
    }

    if (audio_file) {
        fclose(audio_file);
        audio_file = NULL;
    }
    active_driver = AUDIO_NONE;
    audio_loaded = 0;
}
