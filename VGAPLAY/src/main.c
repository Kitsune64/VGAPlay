#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <pc.h>
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

/* =========================================================================
   PLAY SYNCHRONIZED LZ4 CINEMATIC
   Returns 1 if the user pressed ESC to skip, 0 otherwise.
   ========================================================================= */
int play_lz4_cinematic(const char *video_path, const char *audio_path) {
    FILE* video_file = fopen(video_path, "rb");
    if (!video_file) return 0;

    // The updated decoder now reads the global 4-byte header AND pre-fetches the Index Table
    struct video_header vh = decode_video_header(video_file);
    decode_video_reset();

    // Load audio soundtrack via Allegro
    SAMPLE *sound_track = load_wav(audio_path);
    if (!sound_track) {
        fclose(video_file);
        return 0;
    }

    // Switch to Allegro linear Mode 13h (320x200, 8-bit)
    if (set_gfx_mode(GFX_VGA, 320, 200, 0, 0) != 0) {
        destroy_sample(sound_track);
        fclose(video_file);
        return 0;
    }

    // Create intermediate VRAM buffer in RAM
    BITMAP *ram_buffer = create_bitmap(320, 200);
    if (!ram_buffer) {
        destroy_sample(sound_track);
        fclose(video_file);
        return 0;
    }

    PALETTE allegro_pal;

    // Calculate exact clock timings per frame based on header FPS
    int fps = vh.frame_rate ? vh.frame_rate : 30;
    long long uclocks_per_frame = UCLOCKS_PER_SEC / fps;

    // Simultaneous audio start and time-zero anchoring
    play_sample(sound_track, 255, 128, 1000, 0);
    uclock_t start_time = uclock();

    int frame = 0;
    int skipped = 0;
    
    while (frame < vh.frame_count) {
        // Interrupt immediately if ESC is pressed
        if (key[KEY_ESC]) {
            skipped = 1;
            break;
        }

        if (key[KEY_LSHIFT] || key[KEY_RSHIFT] || key[KEY_LCONTROL]) skip = 1; else skip = 0;

        uclock_t current_time = uclock();
        long long elapsed_uclocks = current_time - start_time;
        
        // Calculate which frame *should* be visible right now
        int target_frame = (int)((elapsed_uclocks * fps) / UCLOCKS_PER_SEC);

        // Frame drop mechanism: if decoder lags behind real-time, skip rendering but keep decoding to feed LZ4 stream history
        if (frame < target_frame) {
            // UPDATED: Passing current "frame" index to fetch target block size from RAM
            decode_video_frame(video_file, frame, palette_data, palette_sizes, video_data);
            frame++;
            continue;
        }

        // Decode current video trame using the pre-fetched Index Table mapping
        decode_video_frame(video_file, frame, palette_data, palette_sizes, video_data);

        byte *raw_pal = palette_data;

        /* FAST VGA DAC OPTIMIZATION: Palettes are always exactly 256 colors now (768 bytes).
           We directly dump the shifted RGB triplets straight into Allegro's hardware registers structure */
        for (int c = 0; c < 256; c++) {
            allegro_pal[c].r = raw_pal[c * 3] >> 2;
            allegro_pal[c].g = raw_pal[c * 3 + 1] >> 2;
            allegro_pal[c].b = raw_pal[c * 3 + 2] >> 2;
        }

        set_palette(allegro_pal);

		// --- RENDER BUFFER TO SCREEN ---
		for (int y = 0; y < 200; y++) {
			unsigned long dest_addr = bmp_write_line(ram_buffer, y);
			memcpy((void *)dest_addr, video_data + (y * 320), 320);
		}


        // Blit intermediate buffer directly to VGA screen address
        blit(ram_buffer, screen, 0, 0, 0, 0, 320, 200);
        
        // --- SOFT TIMING SYNCHRONIZATION ---
        uclock_t frame_target_time = start_time + ((frame + 1) * uclocks_per_frame);
        if (!skip) {
            while (uclock() < frame_target_time) {
                rest(0); // Yield to system without raw locking the CPU threads
            }
        }

        frame++;
    }

    // Clean resources up
    destroy_bitmap(ram_buffer);
    stop_sample(sound_track);
    destroy_sample(sound_track);
    fclose(video_file);

    clear_to_color(screen, 0);
    decode_video_free();
    return skipped;
}

/* =========================================================================
   RPG ENGINE MAIN ENTRY POINT
   ========================================================================= */
int main(void) {
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

    // --- PLAY CINEMATIC INTRO ---
    if (exists("VIDEO.BIN") && exists("AUDIO.WAV")) {
        play_lz4_cinematic("VIDEO.BIN", "AUDIO.WAV");
    }

    // --- INITIALIZE RPG ENGINE IN MODE X (320x240) ---
    if (set_gfx_mode(GFX_MODEX, 320, 240, 0, 0) != 0) {
        printf("Error: Could not switch VGA to Mode X!\n");
        return 1;
    }

    BITMAP *game_buffer = create_bitmap(320, 240);

    while (!key[KEY_ESC]) {
        clear_bitmap(game_buffer);
        textout_centre_ex(game_buffer, font, "ENGINE ACTIVE IN MODE X (320x240)", 160, 100, 15, -1);
        textout_centre_ex(game_buffer, font, "CINEMATIC STREAMING PASSED OK", 160, 120, 14, -1);
        blit(game_buffer, screen, 0, 0, 0, 0, 320, 240);
        rest(10); 
    }

    destroy_bitmap(game_buffer);
    return 0;
}
END_OF_MAIN()
