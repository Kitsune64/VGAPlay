#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <pc.h>
#include <time.h>
#include <string.h>
#include <allegro.h>

/* En-têtes du décodeur vidéo DOSVideoPlayer */
#include "types.h"
#include "util.h"
#include "decoder.h"
#include "wav.h"

#define VIDEO_BUF_SZ        1

/* Variables globales du lecteur vidéo */
byte skip = 0;
long long shift = 0;

byte video_data[320 * 200 * VIDEO_BUF_SZ];
byte palette_data[768 * VIDEO_BUF_SZ];
int palette_sizes[VIDEO_BUF_SZ];

/* =========================================================================
   LECTURE DE LA CINÉMATIQUE SYNCHRONISEE AUTOMATIQUEMENT
   Retourne 1 si l'utilisateur a appuyé sur ESC pour skip, 0 sinon.
   ========================================================================= */
int play_lz4_cinematic(const char *video_path, const char *audio_path) {
    FILE* video_file = fopen(video_path, "rb");
    if (!video_file) return 0;

    struct video_header vh = decode_video_header(video_file);
    decode_video_reset();

    // Chargement de l'audio via Allegro pour une lecture autonome
    SAMPLE *sound_track = load_wav(audio_path);
    if (!sound_track) {
        fclose(video_file);
        return 0;
    }

    // Passage en Mode 13h linéaire d'Allegro (320x200, 8-bit)
    if (set_gfx_mode(GFX_VGA, 320, 200, 0, 0) != 0) {
        destroy_sample(sound_track);
        fclose(video_file);
        return 0;
    }

    // Création du tampon mémoire intermédiaire en RAM
    BITMAP *ram_buffer = create_bitmap(320, 200);
    if (!ram_buffer) {
        destroy_sample(sound_track);
        fclose(video_file);
        return 0;
    }

    PALETTE allegro_pal;

    // Calcul du temps exact par image en fonction du framerate d'origine
    int fps = vh.frame_rate ? vh.frame_rate : 30;
    long long uclocks_per_frame = UCLOCKS_PER_SEC / fps;

    // Lancement simultané de l'audio et enregistrement du point zéro temporel
    play_sample(sound_track, 255, 128, 1000, 0);
    uclock_t start_time = uclock();

    // Boucle de lecture ancrée sur le temps réel écoulé pour éviter le décalage audio/vidéo
    int frame = 0;
    int skipped = 0;
    
    while (frame < vh.frame_count) {
        // Si on appuie sur ESC, on interrompt immédiatement la cinématique
        if (key[KEY_ESC]) {
            skipped = 1;
            break;
        }

        if (key[KEY_LSHIFT] || key[KEY_RSHIFT] || key[KEY_LCONTROL]) skip = 1; else skip = 0;

        uclock_t current_time = uclock();
        long long elapsed_uclocks = current_time - start_time;
        
        // Calculer quelle image *devrait* s'afficher à cet instant précis
        int target_frame = (int)((elapsed_uclocks * fps) / UCLOCKS_PER_SEC);

        // Si la vidéo est en retard par rapport au temps écoulé, on décode sans afficher (drop frame) pour rattraper
        if (frame < target_frame) {
            decode_video_frame(video_file, palette_data, palette_sizes, video_data);
            frame++;
            continue;
        }

        // Décodage de la trame vidéo courante
        decode_video_frame(video_file, palette_data, palette_sizes, video_data);

        byte *raw_pal = palette_data;
        int colors_to_set = *palette_sizes;

        /* Initialiser les 256 entrées */
        for (int c = 0; c < 256; c++) {
            allegro_pal[c].r = 0;
            allegro_pal[c].g = 0;
            allegro_pal[c].b = 0;
        }

        /* Charger les couleurs de la trame */
        for (int c = 0; c < colors_to_set && c < 256; c++) {
            allegro_pal[c].r = raw_pal[c * 3] >> 2;
            allegro_pal[c].g = raw_pal[c * 3 + 1] >> 2;
            allegro_pal[c].b = raw_pal[c * 3 + 2] >> 2;
        }

        set_palette(allegro_pal);

        // --- AFFICHAGE DE LA FRAME ---
        if (ram_buffer->line[1] - ram_buffer->line[0] == 320) {
            memcpy(ram_buffer->line[0], video_data, 64000);
        } else {
            for (int y = 0; y < 200; y++) {
                memcpy(ram_buffer->line[y], video_data + (y * 320), 320);
            }
        }

        // Affichage direct du buffer RAM vers l'écran
        blit(ram_buffer, screen, 0, 0, 0, 0, 320, 200);
        
        // --- SYNCHRONISATION SOUPLE ---
        uclock_t frame_target_time = start_time + ((frame + 1) * uclocks_per_frame);
        if (!skip) {
            while (uclock() < frame_target_time) {
                rest(0); // Cède la main au système sans bloquer rigidement
            }
        }

        frame++;
    }

    // Nettoyage propre
    destroy_bitmap(ram_buffer);
    stop_sample(sound_track);
    destroy_sample(sound_track);
    fclose(video_file);

    clear_to_color(screen, 0);
    return skipped;
}

/* =========================================================================
   POINT D'ENTRÉE PRINCIPAL DE VOTRE MOTEUR RPG
   ========================================================================= */
int main(void) {
    if (allegro_init() != 0) {
        printf("Erreur d'initialisation d'Allegro !\n");
        return 1;
    }

    install_keyboard();
    install_timer();

    if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
        printf("Avertissement : Le système audio (WAV/MIDI) n'est pas disponible !\n");
    }

    set_color_depth(8);

    // --- LECTURE DE LA CINÉMATIQUE ---
    if (exists("VIDEO.BIN") && exists("AUDIO.WAV")) {
        // play_lz4_cinematic va s'arrêter d'elle-même si ESC est pressé
        play_lz4_cinematic("VIDEO.BIN", "AUDIO.WAV");
    }

    // --- LANCEMENT DU MOTEUR RPG EN MODE X (320x240) ---
    // S'exécutera directement, que la vidéo soit allée jusqu'au bout ou qu'on l'ait stoppée avec ESC.
    if (set_gfx_mode(GFX_MODEX, 320, 240, 0, 0) != 0) {
        printf("Erreur : Impossible d'activer le Mode X!\n");
        return 1;
    }

    BITMAP *game_buffer = create_bitmap(320, 240);

    while (!key[KEY_ESC]) {
        clear_bitmap(game_buffer);
        textout_centre_ex(game_buffer, font, "MOTEUR EN MODE X (320x240)", 160, 100, 15, -1);
        textout_centre_ex(game_buffer, font, "CINEMATIQUE LECTURE OK", 160, 120, 14, -1);
        blit(game_buffer, screen, 0, 0, 0, 0, 320, 240);
        rest(10); 
    }

    destroy_bitmap(game_buffer);
    return 0;
}
END_OF_MAIN()
