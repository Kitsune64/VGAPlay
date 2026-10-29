/*
  mpg2wav - MP4 Audio Extractor to Retro Sound Blaster WAV Format (11025 Hz, 8-bit, Mono)
  License: MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* FFmpeg (libav) headers */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

/* Standard WAV header structure (Microsoft RIFF) */
#pragma pack(push, 1)
typedef struct {
    char chunk_id[4];         /* "RIFF" */
    uint32_t chunk_size;      /* File size - 8 bytes */
    char format[4];           /* "WAVE" */
    char subchunk1_id[4];     /* "fmt " */
    uint32_t subchunk1_size;  /* 16 for PCM */
    uint16_t audio_format;    /* 1 for PCM uncompressed */
    uint16_t num_channels;    /* 1 for Mono */
    uint32_t sample_rate;     /* 11025 Hz target */
    uint32_t byte_rate;       /* sample_rate * channels * bits_per_sample / 8 */
    uint16_t block_align;     /* channels * bits_per_sample / 8 */
    uint16_t bits_per_sample; /* 8 for 8-bit unsigned PCM */
    char subchunk2_id[4];     /* "data" */
    uint32_t subchunk2_size;  /* Raw audio bytes size */
} wav_header_t;
#pragma pack(pop)

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: mpg2wav <file.mp4>\n");
        return 1;
    }

    char *input_filename = argv[1];
    uint32_t target_sample_rate = 11025;

    /* 1. Open input multimedia file using FFmpeg context */
    AVFormatContext *format_ctx = NULL;
    if (avformat_open_input(&format_ctx, input_filename, NULL, NULL) != 0) {
        printf("Could not open file %s\n", input_filename);
        return 1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        avformat_close_input(&format_ctx);
        return 1;
    }

    /* Find the primary audio stream index */
    int audio_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }

    if (audio_stream_idx == -1) {
        printf("No audio stream found in the input file.\n");
        avformat_close_input(&format_ctx);
        return 1;
    }

    /* Set up decoder context for the audio stream */
    AVCodecParameters *codec_params = format_ctx->streams[audio_stream_idx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_params);
    
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("Could not open audio decoder.\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return 1;
    }

    /* 2. Configure the Software Resampler (libswresample) */
    SwrContext *swr_ctx = swr_alloc();
    
    /* Set input configuration from the source video file automatically */
    av_opt_set_chlayout(swr_ctx, "in_chlayout", &codec_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);

    /* Set hardcoded target properties required by retro Sound Blaster cards */
    AVChannelLayout mono_layout;
    av_channel_layout_default(&mono_layout, 1); /* 1 channel = Mono */
    av_opt_set_chlayout(swr_ctx, "out_chlayout", &mono_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", target_sample_rate, 0);
    /* AV_SAMPLE_FMT_U8 represents 8-bit unsigned PCM data bytes */
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_U8, 0);

    if (swr_init(swr_ctx) < 0) {
        printf("Failed to initialize the audio resampler context.\n");
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return 1;
    }

    /* 3. Open output target file and write dummy WAV structure header */
    FILE *f_wav = fopen("AUDIO.WAV", "wb");
    if (!f_wav) {
        printf("Error opening AUDIO.WAV for writing.\n");
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return 1;
    }

    wav_header_t header = {
        {'R', 'I', 'F', 'F'}, 0, {'W', 'A', 'V', 'E'},
        {'f', 'm', 't', ' '}, 16, 1, 1, target_sample_rate,
        target_sample_rate * 1 * 1, 1, 8,
        {'d', 'a', 't', 'a'}, 0
    };
    fwrite(&header, sizeof(wav_header_t), 1, f_wav);

    /* 4. Resampling process processing loop */
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    uint32_t data_bytes_written = 0;

    printf("Extracting and resampling audio to 11025 Hz 8-bit Mono...\n");

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                    
                    /* Calculate upper bound allocation room for converted samples buffer */
                    int max_out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, codec_ctx->sample_rate) + frame->nb_samples, target_sample_rate, codec_ctx->sample_rate, AV_ROUND_UP);
                    
                    uint8_t *output_buffer = NULL;
                    av_samples_alloc(&output_buffer, NULL, 1, max_out_samples, AV_SAMPLE_FMT_U8, 0);

                    /* High-fidelity resample, downmix and hardware level quantization */
                    int out_samples = swr_convert(swr_ctx, &output_buffer, max_out_samples, (const uint8_t **)frame->data, frame->nb_samples);
                    
                    if (out_samples > 0) {
                        /* 1 sample = 1 byte since format layout is Mono 8-bit unsigned PCM */
                        fwrite(output_buffer, 1, out_samples, f_wav);
                        data_bytes_written += out_samples;
                    }

                    av_freep(&output_buffer);
                }
            }
        }
        av_packet_unref(packet);
    }

    /* 5. Finalize file boundaries and correct sizing data metadata fields */
    header.subchunk2_size = data_bytes_written;
    header.chunk_size = sizeof(wav_header_t) - 8 + data_bytes_written;
    
    fseek(f_wav, 0, SEEK_SET);
    fwrite(&header, sizeof(wav_header_t), 1, f_wav);
    fclose(f_wav);

    /* Context releases and allocation cleanup cycles */
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    printf("Audio extraction completed successfully: AUDIO.WAV (11025 Hz, Mono, 8-bit, %u bytes written)\n", data_bytes_written);
    return 0;
}
