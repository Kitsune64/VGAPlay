#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

// WAV header structure
typedef struct {
    char chunk_id[4];        // "RIFF"
    uint32_t chunk_size;     // File size - 8
    char format[4];          // "WAVE"
    char subchunk1_id[4];    // "fmt "
    uint32_t subchunk1_size; // 16 for PCM
    uint16_t audio_format;   // 1 for PCM
    uint16_t num_channels;   // 1 for Mono
    uint32_t sample_rate;    // Sample rate: 11025 Hz
    uint32_t byte_rate;      // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;    // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample;// 8 for 8-bit unsigned
    char subchunk2_id[4];    // "data"
    uint32_t subchunk2_size; // Raw audio data size
} wav_header_t;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <video.mpg>\n", argv[0]);
        return 1;
    }

    plm_t *plm = plm_create_with_filename(argv[1]);
    if (!plm) {
        printf("Error opening MPEG file.\n");
        return 1;
    }

    // Disable video decoding to focus purely on audio extraction speed
    plm_set_video_enabled(plm, FALSE);
    plm_set_audio_enabled(plm, TRUE);

    // Récupération de la fréquence source native du MPEG (ex: 44100 ou 22050 Hz)
    int source_sample_rate = plm_get_samplerate(plm);
    int target_sample_rate = 11025; // Cible fixe pour Sound Blaster / DOS

    if (source_sample_rate <= 0) {
        source_sample_rate = 44100; // Valeur de repli par défaut si non détecté
    }

    // Calcul du pas de décimation (ex: 44100 / 11025 = 4)
    int step = source_sample_rate / target_sample_rate;
    if (step < 1) step = 1;

    FILE *f_wav = fopen("AUDIO.WAV", "wb");
    if (!f_wav) {
        printf("Error opening AUDIO.WAV for writing.\n");
        plm_destroy(plm);
        return 1;
    }

    // Write placeholder WAV header (will be updated at the end)
    wav_header_t header = {
        {'R', 'I', 'F', 'F'}, 0, {'W', 'A', 'V', 'E'},
        {'f', 'm', 't', ' '}, 16, 1, 1, target_sample_rate,
        target_sample_rate * 1, 1, 8,
        {'d', 'a', 't', 'a'}, 0
    };
    fwrite(&header, sizeof(wav_header_t), 1, f_wav);

    uint32_t data_bytes_written = 0;
    plm_samples_t *samples;
    int sample_counter = 0;

    // Audio decoding and downsampling loop
    while ((samples = plm_decode_audio(plm)) != NULL) {
        int num_frames = samples->count;

        for (int i = 0; i < num_frames; i++) {
            // On conserve uniquement l'échantillon qui correspond au pas cible (11025 Hz)
            if ((sample_counter % step) == 0) {
                // Downmix Stereo (2 channels) to Mono by averaging Left and Right
                float left = samples->interleaved[i * 2];
                float right = samples->interleaved[i * 2 + 1];
                float mono_sample = (left + right) * 0.5f;

                // Clamp bounds between -1.0 and 1.0
                if (mono_sample < -1.0f) mono_sample = -1.0f;
                if (mono_sample >  1.0f) mono_sample =  1.0f;

                // Convert Float [-1.0, 1.0] to 8-bit Unsigned [0, 255]
                uint8_t sample_8bit = (uint8_t)((mono_sample + 1.0f) * 127.5f);

                fwrite(&sample_8bit, 1, 1, f_wav);
            data_bytes_written++;
            }
            sample_counter++;
        }
    }

    // Update chunk sizes in the WAV header
    header.subchunk2_size = data_bytes_written;
    header.chunk_size = sizeof(wav_header_t) - 8 + data_bytes_written;
    
    fseek(f_wav, 0, SEEK_SET);
    fwrite(&header, sizeof(wav_header_t), 1, f_wav);

    fclose(f_wav);
    plm_destroy(plm);

    printf("Audio extraction completed successfully: AUDIO.WAV (11025 Hz, %u bytes)\n", data_bytes_written);
    return 0;
}

