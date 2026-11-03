/*
 * main.c from VGAPLAY 1.4 by Kitsune64 on Github.
 * URL: https://github.com/Kitsune64/VGAPlay
 * 
 * VGAPLAY is a VGA Player in VGA Mode 13H for DOS Operating System
 *
 * Distributed under the MIT License. See README.TXT for details.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <conio.h>
#include <pc.h>
#include <dpmi.h>
#include <sys/movedata.h>
#include <time.h>

#include "types.h"
#include "util.h"
#include "decoder.h"
#include "wav.h"
#include "audio.h"

#define VIDEO_BUF_SZ        1

byte skip = 0;
long long shift = 0;

byte video_data[320 * 200 * VIDEO_BUF_SZ];
byte palette_data[768 * VIDEO_BUF_SZ];
int palette_sizes[VIDEO_BUF_SZ];

/* Ouvre le fichier de log si l'option est activée */
static FILE* open_sync_log(int enable_log) {
    if (!enable_log) return NULL;
    
    FILE *log = fopen("SYNC.LOG", "w");
    if (log) {
        fprintf(log, "Debut de play_lz4_cinematic\n");
        fflush(log);
    } else {
        printf("Impossible de creer SYNC.LOG\n");
    }
    return log;
}

/* Ferme proprement le fichier de log s'il est actif */
static void close_sync_log(FILE *log) {
    if (log) {
        fprintf(log, "Fin de lecture\n");
        fclose(log);
    }
}

static void set_mode_13h(void) {
    __dpmi_regs r;
    memset(&r, 0, sizeof(r));
    r.h.ah = 0x00;
    r.h.al = 0x13;
    __dpmi_int(0x10, &r);
}

static void set_text_mode(void) {
    __dpmi_regs r;
    memset(&r, 0, sizeof(r));
    r.h.ah = 0x00;
    r.h.al = 0x03;
    __dpmi_int(0x10, &r);
}

static void set_vga_palette(const byte *raw_pal) {
    int i;
    outportb(0x3C8, 0);
    for (i = 0; i < 256 * 3; i++) {
        outportb(0x3C9, raw_pal[i] >> 2);
    }
}

static int file_ok(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

int generate_index_file(const char *base_filename) {
    char video_filename[128];
    char audio_filename[128];
    char idx_filename[128];
    int32_t frame_count_i32, total_samples_i32;

    sprintf(video_filename, "%s.BIN", base_filename);
    sprintf(audio_filename, "%s.WAV", base_filename);
    sprintf(idx_filename, "%s.IDX", base_filename);

    FILE *video_file = fopen(video_filename, "rb");
    if (!video_file) {
        printf("Error: Could not open video file %s\n", video_filename);
        return 0;
    }

    struct video_header vh = decode_video_header(video_file);
    fclose(video_file);
    decode_video_free();

    if (vh.frame_count == 0) {
        printf("Error: Invalid video frame count (%d)\n", (int)vh.frame_count);
        return 0;
    }

    FILE *audio_file = fopen(audio_filename, "rb");
    if (!audio_file) {
        printf("Error: Could not open audio file %s\n", audio_filename);
        return 0;
    }

    unsigned short channels = 1, bits_per_sample = 16;
    unsigned int data_size = 0;
    char chunk[4];
    unsigned int chunk_size = 0;

    /* Analyse RIFF/WAV */
    if (fread(chunk, 1, 4, audio_file) == 4 && memcmp(chunk, "RIFF", 4) == 0) {
        fseek(audio_file, 4, SEEK_CUR);
        if (fread(chunk, 1, 4, audio_file) == 4 && memcmp(chunk, "WAVE", 4) == 0) {
            while (fread(chunk, 1, 4, audio_file) == 4) {
                if (fread(&chunk_size, 4, 1, audio_file) != 1) break;
                if (memcmp(chunk, "fmt ", 4) == 0) {
                    unsigned short audio_format;
                    fread(&audio_format, 2, 1, audio_file);
                    fread(&channels, 2, 1, audio_file);
                    fseek(audio_file, 4, SEEK_CUR); /* SampleRate */
                    fseek(audio_file, 6, SEEK_CUR); /* ByteRate & BlockAlign */
                    fread(&bits_per_sample, 2, 1, audio_file);
                    if (chunk_size > 16) fseek(audio_file, chunk_size - 16 + (chunk_size & 1), SEEK_CUR);
                } else if (memcmp(chunk, "data", 4) == 0) {
                    data_size = chunk_size;
                    break;
                } else {
                    fseek(audio_file, chunk_size + (chunk_size & 1), SEEK_CUR);
                }
            }
        }
    }

    /* Meme regle que audio.c : ne pas depasser la taille reelle du fichier */
    if (data_size != 0) {
        long pos = ftell(audio_file);
        long end;
        fseek(audio_file, 0, SEEK_END);
        end = ftell(audio_file);
        if (end > pos && data_size > (unsigned int)(end - pos)) data_size = (unsigned int)(end - pos);
    }
    fclose(audio_file);

    if (data_size == 0) {
        printf("Error: Audio data chunk not found or empty.\n");
        return 0;
    }

    unsigned int bytes_per_sample_frame = channels * (bits_per_sample / 8);
    if (bytes_per_sample_frame == 0) bytes_per_sample_frame = 2;
    unsigned int total_samples = data_size / bytes_per_sample_frame;

    uint32_t *index_table = (uint32_t *)malloc((size_t)vh.frame_count * sizeof(uint32_t));
    if (!index_table) {
        printf("Error: Out of memory for index table!\n");
        return 0;
    }

    /* Remplissage progressif et croissant de la table */
    for (int i = 0; i < (int)vh.frame_count; i++) {
        index_table[i] = (uint32_t)(((double)i * (double)total_samples) / (double)vh.frame_count);
    }

    FILE *f = fopen(idx_filename, "wb");
    if (!f) {
        free(index_table);
        printf("Error: Could not create index file %s\n", idx_filename);
        return 0;
    }

    /* Ecriture en int32 explicite (frame_count est un mot de 16 bits) */
    frame_count_i32 = (int32_t)vh.frame_count;
    total_samples_i32 = (int32_t)total_samples;
    fwrite("IDX1", 1, 4, f);
    fwrite(&frame_count_i32, 4, 1, f);
    fwrite(&total_samples_i32, 4, 1, f);
    fwrite(index_table, sizeof(uint32_t), vh.frame_count, f);
    fclose(f);

    free(index_table);
    printf("[IDX] Index successfully generated (%d frames, %u samples).\n", (int)vh.frame_count, total_samples);
    return 1;
}

int play_lz4_cinematic(const char *video_path, const char *audio_path, int enable_log) {
	int frame = 0;
	int skipped = 0;
	int target_frame = 0;
	int raw_sample;
	int current_sample;
	uclock_t debug_last = 0;
	uclock_t debug_now;
	unsigned long next_index;

    /* Utilisation de la fonction dédiée pour l'ouverture */
    FILE *log = open_sync_log(enable_log);
		
    FILE *video_file = fopen(video_path, "rb");
    if (!video_file) {
        close_sync_log(log);
        return 0;
    }

    struct video_header vh = decode_video_header(video_file);
    decode_video_reset();

    int frame_count = (int)vh.frame_count;
    if (frame_count <= 0) {
        fclose(video_file);
        decode_video_free();
        close_sync_log(log);
        return 0;
    }

    unsigned int total_samples = 0;
    if (!audio_load_wav(audio_path, &total_samples)) {
        fclose(video_file);
        decode_video_free();
        close_sync_log(log);
        return 0;
    }

    char base_name[128];
    strcpy(base_name, video_path);
    char *ext = strrchr(base_name, '.');
    if (ext) *ext = '\0';

    char idx_filename[128];
    sprintf(idx_filename, "%s.IDX", base_name);

    uint32_t *frame_index = (uint32_t *)malloc((size_t)frame_count * sizeof(uint32_t));
    if (!frame_index) {
        audio_stop();
        fclose(video_file);
        decode_video_free();
        close_sync_log(log);
        return 0;
    }

    int index_loaded = 0;
    FILE *f_idx = fopen(idx_filename, "rb");
    if (f_idx) {
        char magic[4] = {0};
        int32_t f_count = 0, f_samples = 0;

        if (fread(magic, 1, 4, f_idx) == 4 &&
            fread(&f_count, 4, 1, f_idx) == 1 &&
            fread(&f_samples, 4, 1, f_idx) == 1 &&
            memcmp(magic, "IDX1", 4) == 0 &&
            f_count == frame_count &&
            (unsigned int)f_samples == total_samples &&
            fread(frame_index, sizeof(uint32_t), (size_t)frame_count, f_idx) == (size_t)frame_count) {
            index_loaded = 1;
        }
        fclose(f_idx);
    }

    if (!index_loaded) {
        for (int i = 0; i < frame_count; i++) {
            frame_index[i] = (uint32_t)(((double)i * (double)total_samples) / (double)frame_count);
        }
    }

    printf("[DEBUG] Avant set_mode_13h\n"); fflush(stdout);
	set_mode_13h();
	printf("[DEBUG] Avant audio_play\n"); fflush(stdout);
	audio_play();
	printf("[DEBUG] Audio démarré avec succès\n"); fflush(stdout);

    while (frame < frame_count) {
        /* ESC pour quitter */
        if (kbhit() && getch() == 27) {
            skipped = 1;
            break;
        }

		audio_update();
		raw_sample = audio_get_position();
		current_sample = raw_sample;
		
		usleep(1000);
		
		debug_now = uclock();
		if (debug_last == 0)
			debug_last = debug_now;

		if (debug_now - debug_last >= UCLOCKS_PER_SEC) {
			if (log) {
				next_index = 0;
				if (frame + 1 < frame_count)
					next_index = (unsigned long)frame_index[frame + 1];

				fprintf(log,
					"time=%lu frame=%d target=%d sample=%d next_index=%lu\n",
					(unsigned long)debug_now,
					frame,
					target_frame,
					raw_sample,
					next_index);
				fflush(log);
			}
			debug_last = debug_now;
		}
		
        if (current_sample < 0) current_sample = 0x7FFFFFFF;

        while (target_frame < frame_count - 1 && frame_index[target_frame + 1] <= (uint32_t)current_sample) {
            target_frame++;
        }
        while (target_frame > 0 && frame_index[target_frame] > (uint32_t)current_sample) {
            target_frame--;
        }

        if (frame > target_frame) {
            usleep(1000);
            continue;
        }

        while (frame < target_frame && frame < frame_count) {
            decode_video_frame(video_file, frame, palette_data, palette_sizes, video_data);
            frame++;
        }

        if (frame >= frame_count) break;

        decode_video_frame(video_file, frame, palette_data, palette_sizes, video_data);

		while (inportb(0x3DA) & 0x08);
		while (!(inportb(0x3DA) & 0x08));

		set_vga_palette(palette_data);
		dosmemput(video_data, 320 * 200, 0xA0000);

        frame++;
    }

    free(frame_index);
    audio_stop();
    fclose(video_file);
    decode_video_free();

    while (kbhit()) getch();   /* vide le buffer clavier */
    set_text_mode();
    
    /* Utilisation de la fonction dédiée pour la fermeture normale */
    close_sync_log(log);
    
    return skipped;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("VGAPLAY :Usage: VGAPLAY.EXE <filename> [options]\n");
        return 1;
    }
	if (argc >= 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
		printf("VGAPLAY version 1.4\n");
		return 0;
	}
    char base_name[100];
    strncpy(base_name, argv[1], sizeof(base_name) - 1);
    base_name[sizeof(base_name) - 1] = '\0';
    char *ext = strrchr(base_name, '.');
    if (ext != NULL) *ext = '\0';

    if (argc >= 3 && strcmp(argv[2], "-idx") == 0) {
        return generate_index_file(base_name) ? 0 : 1;
    }

    /* --- INITIALISATION ET AFFICHAGE DU MATERIEL AUDIO --- */
    audio_init();
    if (audio_driver == AUDIO_AC97) {
        printf("[AUDIO] Carte son detectee : AC'97 (Natif PCI)\n");
    } else if (audio_driver == AUDIO_SB) {
        printf("[AUDIO] Carte son detectee : Sound Blaster 16\n");
    } else {
        printf("[AUDIO] Avertissement : Aucune carte son detectee (Mode muet)\n");
    }
    //usleep(1500000); /* Pause de 1.5 seconde pour lire le message */

    char video_filename[128];
    char audio_filename[128];
    sprintf(video_filename, "%s.BIN", base_name);
    sprintf(audio_filename, "%s.WAV", base_name);

	/* Vérifier si l'option -log est présente dans les arguments */
    int enable_log = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-log") == 0) {
            enable_log = 1;
            break;
        }
    }

    if (file_ok(video_filename) && file_ok(audio_filename)) {
		printf("[TRACE] Lancement de la cinematique...\n"); fflush(stdout);
        play_lz4_cinematic(video_filename, audio_filename, enable_log);
    } else {
        printf("Error: Video file (%s) or audio file (%s) not found!\n", video_filename, audio_filename);
        return 1;
    }

    set_text_mode();

    printf("CINEMATIC STREAMING PASSED OK\n");
    printf("Press any key to exit...\n");
    getch();

    return 0;
}
