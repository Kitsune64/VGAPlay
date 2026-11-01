/*
 * main.c from VGAPLAY by Kitsune64 on Github. 
 * URL: https://github.com/Kitsune64/VGAPlay
 * VGAPLAY is a VGA Player in VGA Mode 13H for DOS Operating System
 * 
 * Distributed under the MIT License. See README.TXT for details.
 */
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <pc.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <time.h>
#include <string.h>
#include <allegro.h>

/* DOSVideoPlayer decoder headers */
#include "types.h"
#include "util.h"
#include "decoder.h"
#include "wav.h"

#define VIDEO_BUF_SZ        1

/* Global video player variables */
byte skip = 0;
long long shift = 0;

byte video_data[320 * 200 * VIDEO_BUF_SZ];
byte palette_data[768 * VIDEO_BUF_SZ];
int palette_sizes[VIDEO_BUF_SZ];

/* Switch to Mode 13h (320x200, 256 colors) using BIOS interrupt via DPMI */
void set_mode_13h() {
    __dpmi_regs r;
    r.h.ah = 0x00; // BIOS function: Set video mode
    r.h.al = 0x13; // Mode 13h (320x200, 256 colors)
    __dpmi_int(0x10, &r);
}

/* Return to standard text mode */
void set_text_mode() {
    __dpmi_regs r;
    r.h.ah = 0x00;
    r.h.al = 0x03; // Standard text mode 80x25
    __dpmi_int(0x10, &r);
}

/* Send palette data directly to VGA DAC ports */
void set_vga_palette(const byte *raw_pal) {
    outportb(0x3C8, 0); // Start at color index 0
    for (int i = 0; i < 256 * 3; i++) {
        // Native VGA palette expects values from 0 to 63 (hence shifting right by 2)
        outportb(0x3C9, raw_pal[i] >> 2);
    }
}

/* =========================================================================
   EXPLICITLY GENERATE AND SAVE THE .IDX FILE
   ========================================================================= */
int generate_index_file(const char *base_filename) {
    char video_filename[128];
    char audio_filename[128];
    char idx_filename[128];

    sprintf(video_filename, "%s.BIN", base_filename);
    sprintf(audio_filename, "%s.WAV", base_filename);
    sprintf(idx_filename, "%s.IDX", base_filename);

    /* Open video file to read header */
    FILE *video_file = fopen(video_filename, "rb");
    if (!video_file) {
        printf("Error: Could not open video file %s\n", video_filename);
        return 0;
    }
    struct video_header vh = decode_video_header(video_file);
    fclose(video_file);

    /* Open audio file to get sample length */
    SAMPLE *sound_track = load_wav(audio_filename);
    if (!sound_track) {
        printf("Error: Could not open audio file %s\n", audio_filename);
        return 0;
    }

    /* Allocate memory for the index table */
    uint32_t *index_table = (uint32_t *)malloc(vh.frame_count * sizeof(uint32_t));
    if (!index_table) {
        destroy_sample(sound_track);
        printf("Error: Out of memory for index table!\n");
        return 0;
    }

    /* Populate index table with proportional timing */
    for (int i = 0; i < vh.frame_count; i++) {
        index_table[i] = (uint32_t)(((double)i * sound_track->len) / vh.frame_count);
    }

    /* Write index file to disk */
    FILE *f = fopen(idx_filename, "wb");
    if (!f) {
        free(index_table);
        destroy_sample(sound_track);
        printf("Error: Could not create index file %s (check write permissions)\n", idx_filename);
        return 0;
    }

    fwrite("IDX1", 1, 4, f);
    fwrite(&vh.frame_count, sizeof(int), 1, f);
    fwrite(&sound_track->len, sizeof(int), 1, f);
    fwrite(index_table, sizeof(uint32_t), vh.frame_count, f);
    fclose(f);

    /* Cleanup */
    free(index_table);
    destroy_sample(sound_track);

    printf("Success: Index file '%s' generated successfully!\n", idx_filename);
    return 1;
}

/* =========================================================================
   PLAY SYNCHRONIZED LZ4 CINEMATIC USING THE .IDX FILE IN RAM
   ========================================================================= */
int play_lz4_cinematic(const char *video_path, const char *audio_path) {
    FILE* video_file = fopen(video_path, "rb");
    if (!video_file) return 0;

    struct video_header vh = decode_video_header(video_file);
    decode_video_reset();

    SAMPLE *sound_track = load_wav(audio_path);
    if (!sound_track) {
        fclose(video_file);
        return 0;
    }

    /* Extract base filename for the .IDX file */
    char base_name[128];
    strcpy(base_name, video_path);
    char *ext = strrchr(base_name, '.');
    if (ext) *ext = '\0';

    char idx_filename[128];
    sprintf(idx_filename, "%s.IDX", base_name);

/* Allocate memory for the index table */
    uint32_t *frame_index = (uint32_t *)malloc(vh.frame_count * sizeof(uint32_t));
    if (!frame_index) {
        destroy_sample(sound_track);
        fclose(video_file);
        return 0;
    }

    /* Load the pre-calculated .IDX file from disk into RAM with integrity check */
    int index_loaded = 0;
    FILE *f_idx = fopen(idx_filename, "rb");
    if (f_idx) {
        char magic[4];
        int f_count, f_samples;
        
        fread(magic, 1, 4, f_idx);
        fread(&f_count, sizeof(int), 1, f_idx);
        fread(&f_samples, sizeof(int), 1, f_idx);

        /* Verify integrity to prevent corrupted index data */
        if (memcmp(magic, "IDX1", 4) == 0 && f_count == vh.frame_count && f_samples == sound_track->len) {
            fread(frame_index, sizeof(uint32_t), vh.frame_count, f_idx);
            index_loaded = 1;
        }
        fclose(f_idx);
    }

    /* Fallback if .IDX is missing or invalid: generate it on the fly in RAM */
    if (!index_loaded) {
        for (int i = 0; i < vh.frame_count; i++) {
            frame_index[i] = (uint32_t)(((double)i * sound_track->len) / vh.frame_count);
        }
    }

    set_mode_13h();

    int audio_voice = play_sample(sound_track, 255, 128, 1000, 0);

    int frame = 0;
    int skipped = 0;
    int target_frame = 0; // <--- Déclaré ici, en dehors de la boucle, pour garder sa valeur
    
    while (frame < vh.frame_count) {
        if (key[KEY_ESC]) {
            skipped = 1;
            break;
        }

        /* SUPPRIMER "int target_frame = frame;" D'ICI */

        /* DIRECT SYNCHRONIZATION VIA RAM INDEX TABLE LOOKUP */
        if (audio_voice >= 0) {
            int current_sample = voice_get_position(audio_voice);
            
            if (current_sample < 0) {
                break; /* Audio finished, let video finish remaining frames */
            }

            /* Find target frame efficiently using the pre-calculated index table */
            while (target_frame < vh.frame_count - 1 && frame_index[target_frame + 1] <= (uint32_t)current_sample) {
                target_frame++;
            }
            /* Recede target frame if audio position is behind */
            while (target_frame > 0 && frame_index[target_frame] > (uint32_t)current_sample) {
                target_frame--;
            }
        }

        /* 1. IF VIDEO IS AHEAD : Wait briefly during VBLANK */
        if (frame > target_frame) {
            rest(1); 
            continue;
        }

        /* 2. IF VIDEO IS BEHIND : Catch up without lagging */
        while (frame < target_frame && frame < vh.frame_count) {
            decode_video_frame(video_file, frame, palette_data, palette_sizes, video_data);
            frame++;
        }

        if (frame >= vh.frame_count) break;

        decode_video_frame(video_file, frame, palette_data, palette_sizes, video_data);

        /* 3. WAIT FOR VERTICAL RETRACE (Anti-tearing) */
        while (inportb(0x3DA) & 0x08);
        while (!(inportb(0x3DA) & 0x08));

        /* 4. UPDATE PALETTE DURING VBLANK */
        set_vga_palette(palette_data);

        /* 5. ULTRA-FAST ASSEMBLY TRANSFER (rep movsl) */
        uint32_t *src_ptr = (uint32_t *)video_data;
        uint32_t *dest_ptr = (uint32_t *)((uintptr_t)__djgpp_conventional_base + 0xA0000);
        unsigned long count = (320 * 200) / 4;

        asm volatile (
            "cld\n\t"
            "rep movsl"
            : "+S" (src_ptr), "+c" (count)
            : "D" (dest_ptr)
            : "memory"
        );

        frame++;
    }

    /* Cleanup resources */
    free(frame_index);
    stop_sample(sound_track);
    destroy_sample(sound_track);
    fclose(video_file);
    decode_video_free();

    set_text_mode();
    return skipped;
}

/* =========================================================================
   ENGINE MAIN ENTRY POINT
   ========================================================================= */
int main(int argc, char *argv[]) {

    /* Check if at least one argument is provided */
    if (argc < 2) {
        printf("Usage: VGAPLAY.EXE <filename> [options]\n");
        printf("Options:\n");
        printf("  -idx         Generate the .idx index file for the video and exit\n");
        printf("Example: VGAPLAY INTRO\n");
        printf("         VGAPLAY INTRO -idx\n");
        return 1;
    }

    /* Extract base filename */
    char base_name[100];
    strcpy(base_name, argv[1]);
    char *ext = strrchr(base_name, '.');
    if (ext != NULL) {
        *ext = '\0';
    }

    /* Check if user requested index generation via '-idx' parameter */
    if (argc >= 3 && strcmp(argv[2], "-idx") == 0) {
        return generate_index_file(base_name) ? 0 : 1;
    }

    /* --- NORMAL PLAYBACK MODE --- */
    if (allegro_init() != 0) {
        printf("Allegro initialization failed!\n");
        return 1;
    }

    install_keyboard();
    install_timer();

    if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
        printf("Warning: Sound system (WAV/MIDI) not available!\n");
    }

    set_color_depth(8);

    if (!__djgpp_nearptr_enable()) {
        printf("Erreur : Impossible d'activer le mode nearptr !\n");
        return 1;
    }

    char video_filename[128];
    char audio_filename[128];
    sprintf(video_filename, "%s.BIN", base_name);
    sprintf(audio_filename, "%s.WAV", base_name);

    /* Play cinematic */
    if (exists(video_filename) && exists(audio_filename)) {
        play_lz4_cinematic(video_filename, audio_filename);
    } else {
        __djgpp_nearptr_disable();
        set_text_mode();
        printf("Error: Video file (%s) or audio file (%s) not found!\n", video_filename, audio_filename);
        return 1; 
    }

    set_text_mode();
    __djgpp_nearptr_disable();

    printf("CINEMATIC STREAMING PASSED OK\n");
    printf("Press any key to exit...\n");
    readkey();

    return 0;
}
END_OF_MAIN()

