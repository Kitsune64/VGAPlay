/*
 * mpg2wav - FFmpeg to Sound Blaster 16 WAV audio extractor
 *
 * Output:
 *   RIFF/WAVE
 *   Uncompressed PCM
 *   44100 Hz
 *   Stereo
 *   16-bit signed
 *   Little-endian
 *
 * Compatibility:
 *   - Windows XP 32-bit / FFmpeg 3.4
 *   - Modern Windows Win32
 *   - Modern Windows Win64
 *   - Linux Debian 13 / FFmpeg 7
 *
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>

#if LIBAVUTIL_VERSION_MAJOR >= 57
#include <libavutil/channel_layout.h>
#endif

#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS    2
#define TARGET_BITS        16
#define TARGET_BLOCK_ALIGN 4
#define TARGET_BYTE_RATE   (TARGET_SAMPLE_RATE * TARGET_BLOCK_ALIGN)

/* --------------------------------------------------------- */
/* En-tete WAV RIFF                                          */
/* --------------------------------------------------------- */

#pragma pack(push, 1)

typedef struct {
    char     chunk_id[4];
    uint32_t chunk_size;
    char     format[4];

    char     subchunk1_id[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;

    char     subchunk2_id[4];
    uint32_t subchunk2_size;
} wav_header_t;

#pragma pack(pop)

/* --------------------------------------------------------- */
/* Affichage des erreurs FFmpeg                              */
/* --------------------------------------------------------- */

static void print_ffmpeg_error(const char *message, int error)
{
    char error_buffer[AV_ERROR_MAX_STRING_SIZE];

    av_strerror(error, error_buffer, sizeof(error_buffer));

    fprintf(stderr, "%s : %s\n", message, error_buffer);
}

/* --------------------------------------------------------- */
/* Conversion d'une frame audio et ecriture dans le WAV      */
/* --------------------------------------------------------- */

static int convert_frame(
    AVFrame *frame,
    AVCodecContext *codec_ctx,
    SwrContext *swr_ctx,
    FILE *f_wav,
    uint64_t *data_bytes_written)
{
    int64_t max_out_samples;
    int out_samples;
    int ret;

    uint8_t *output_buffer = NULL;
    uint8_t *output_planes[1];

    uint64_t output_bytes;

    if (codec_ctx->sample_rate <= 0) {
        fprintf(stderr, "Frequence audio d'entree invalide.\n");
        return -1;
    }

    max_out_samples = av_rescale_rnd(
        swr_get_delay(swr_ctx, codec_ctx->sample_rate)
            + frame->nb_samples,
        TARGET_SAMPLE_RATE,
        codec_ctx->sample_rate,
        AV_ROUND_UP);

    if (max_out_samples <= 0 || max_out_samples > INT_MAX) {
        fprintf(stderr, "Taille du buffer audio invalide.\n");
        return -1;
    }

    ret = av_samples_alloc(
        &output_buffer,
        NULL,
        TARGET_CHANNELS,
        (int)max_out_samples,
        AV_SAMPLE_FMT_S16,
        0);

    if (ret < 0) {
        print_ffmpeg_error("Allocation du buffer audio impossible", ret);
        return ret;
    }

    output_planes[0] = output_buffer;

#if LIBSWRESAMPLE_VERSION_MAJOR >= 4

    ret = swr_convert(
        swr_ctx,
        output_planes,
        (int)max_out_samples,
        (const uint8_t * const *)frame->extended_data,
        frame->nb_samples);

#else

    ret = swr_convert(
        swr_ctx,
        output_planes,
        (int)max_out_samples,
        (const uint8_t **)frame->extended_data,
        frame->nb_samples);

#endif

    if (ret < 0) {
        print_ffmpeg_error("Erreur pendant le reechantillonnage", ret);
        av_freep(&output_buffer);
        return ret;
    }

    out_samples = ret;

    output_bytes = (uint64_t)out_samples * TARGET_BLOCK_ALIGN;

    if (output_bytes > UINT32_MAX - *data_bytes_written) {
        fprintf(stderr, "Le fichier WAV depasse la limite RIFF de 4 Go.\n");
        av_freep(&output_buffer);
        return -1;
    }

    if (output_bytes > 0) {
        if (fwrite(output_buffer, 1, (size_t)output_bytes, f_wav)
                != (size_t)output_bytes) {
            fprintf(stderr, "Erreur d'ecriture du fichier WAV.\n");
            av_freep(&output_buffer);
            return -1;
        }

        *data_bytes_written += output_bytes;
    }

    av_freep(&output_buffer);

    return 0;
}

/* --------------------------------------------------------- */
/* Recuperation des frames disponibles dans le decodeur     */
/* --------------------------------------------------------- */

static int drain_decoder(
    AVCodecContext *codec_ctx,
    AVFrame *frame,
    SwrContext *swr_ctx,
    FILE *f_wav,
    uint64_t *data_bytes_written)
{
    int ret;

    while (1) {
        ret = avcodec_receive_frame(codec_ctx, frame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;

        if (ret < 0) {
            print_ffmpeg_error("Erreur de decodage audio", ret);
            return ret;
        }

        ret = convert_frame(
            frame,
            codec_ctx,
            swr_ctx,
            f_wav,
            data_bytes_written);

        av_frame_unref(frame);

        if (ret < 0)
            return ret;
    }
}

/* --------------------------------------------------------- */
/* Vidage du buffer interne du reechantillonneur             */
/* --------------------------------------------------------- */

static int flush_resampler(
    SwrContext *swr_ctx,
    int input_sample_rate,
    FILE *f_wav,
    uint64_t *data_bytes_written)
{
    int ret;
    int max_out_samples;
    int out_samples;

    int64_t delay;
    int64_t max_samples_64;

    uint8_t *output_buffer = NULL;
    uint8_t *output_planes[1];

    uint64_t output_bytes;

    while (1) {
        delay = swr_get_delay(swr_ctx, input_sample_rate);

        if (delay <= 0)
            break;

        max_samples_64 = av_rescale_rnd(
            delay,
            TARGET_SAMPLE_RATE,
            input_sample_rate,
            AV_ROUND_UP);

        if (max_samples_64 < 32)
            max_samples_64 = 32;

        if (max_samples_64 > INT_MAX) {
            fprintf(stderr, "Buffer de vidage trop important.\n");
            return -1;
        }

        max_out_samples = (int)max_samples_64;

        ret = av_samples_alloc(
            &output_buffer,
            NULL,
            TARGET_CHANNELS,
            max_out_samples,
            AV_SAMPLE_FMT_S16,
            0);

        if (ret < 0) {
            print_ffmpeg_error("Allocation du buffer de vidage impossible", ret);
            return ret;
        }

        output_planes[0] = output_buffer;

        ret = swr_convert(
            swr_ctx,
            output_planes,
            max_out_samples,
            NULL,
            0);

        if (ret < 0) {
            print_ffmpeg_error("Erreur pendant le vidage du reechantillonneur", ret);
            av_freep(&output_buffer);
            return ret;
        }

        out_samples = ret;

        output_bytes = (uint64_t)out_samples * TARGET_BLOCK_ALIGN;

        if (output_bytes > UINT32_MAX - *data_bytes_written) {
            fprintf(stderr, "Le fichier WAV depasse la limite RIFF de 4 Go.\n");
            av_freep(&output_buffer);
            return -1;
        }

        if (output_bytes > 0) {
            if (fwrite(output_buffer, 1, (size_t)output_bytes, f_wav)
                    != (size_t)output_bytes) {
                fprintf(stderr, "Erreur d'ecriture du fichier WAV.\n");
                av_freep(&output_buffer);
                return -1;
            }

            *data_bytes_written += output_bytes;
        }

        av_freep(&output_buffer);

        if (out_samples == 0)
            break;
    }

    return 0;
}

/* --------------------------------------------------------- */
/* Programme principal                                       */
/* --------------------------------------------------------- */

int main(int argc, char *argv[])
{
    char *input_filename;
    char output_filename[1024];

    uint32_t target_sample_rate = TARGET_SAMPLE_RATE;

    AVFormatContext *format_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;

    const AVCodec *codec = NULL;

    AVPacket *packet = NULL;
    AVFrame *frame = NULL;

    SwrContext *swr_ctx = NULL;

    FILE *f_wav = NULL;

    wav_header_t header;

    int audio_stream_idx = -1;
    unsigned int i;

    int ret = 0;
    int error_code = 1;

    uint64_t data_bytes_written = 0;

    int header_written = 0;
    if (argc >= 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
		printf("mpg2wav version 1.4\n");
		return 0;
	}
    if (argc < 3) {
        printf("Usage: mpg2wav <file.mp4> <PREFIX>\n");
        printf("Example: mpg2wav MONFILM.MP4 MONFILM\n");
        return 1;
    }

    input_filename = argv[1];

    if (snprintf(
            output_filename,
            sizeof(output_filename),
            "%s.WAV",
            argv[2]) >= (int)sizeof(output_filename)) {
        fprintf(stderr, "Nom de fichier de sortie trop long.\n");
        return 1;
    }

    /* ----------------------------------------------------- */
    /* Enregistrement historique pour FFmpeg < 4.0           */
    /* ----------------------------------------------------- */

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    /* ----------------------------------------------------- */
    /* Ouverture du fichier multimedia                       */
    /* ----------------------------------------------------- */

    if (avformat_open_input(
            &format_ctx,
            input_filename,
            NULL,
            NULL) != 0) {
        fprintf(stderr, "Impossible d'ouvrir %s\n", input_filename);
        goto cleanup;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        fprintf(stderr, "Impossible de lire les informations du fichier.\n");
        goto cleanup;
    }

    /* Recherche du premier flux audio */

    for (i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type
                == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = (int)i;
            break;
        }
    }

    if (audio_stream_idx < 0) {
        fprintf(stderr, "Aucun flux audio trouve.\n");
        goto cleanup;
    }

    /* ----------------------------------------------------- */
    /* Initialisation du decodeur audio                      */
    /* ----------------------------------------------------- */

    {
        AVCodecParameters *codec_params =
            format_ctx->streams[audio_stream_idx]->codecpar;

        codec = avcodec_find_decoder(codec_params->codec_id);

        if (!codec) {
            fprintf(stderr, "Decodeur audio introuvable.\n");
            goto cleanup;
        }

        codec_ctx = avcodec_alloc_context3(codec);

        if (!codec_ctx) {
            fprintf(stderr, "Allocation du contexte decodeur impossible.\n");
            goto cleanup;
        }

        ret = avcodec_parameters_to_context(codec_ctx, codec_params);

        if (ret < 0) {
            print_ffmpeg_error("Initialisation du decodeur impossible", ret);
            goto cleanup;
        }

        ret = avcodec_open2(codec_ctx, codec, NULL);

        if (ret < 0) {
            print_ffmpeg_error("Impossible d'ouvrir le decodeur audio", ret);
            goto cleanup;
        }
    }

    /* ----------------------------------------------------- */
    /* Configuration du reechantillonneur                    */
    /* ----------------------------------------------------- */

#if LIBSWRESAMPLE_VERSION_MAJOR >= 4

    /*
     * API moderne :
     * FFmpeg recent, AVChannelLayout et swr_alloc_set_opts2().
     */

    {
        AVChannelLayout out_layout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout in_layout = { 0 };

        if (codec_ctx->ch_layout.nb_channels > 0) {
            ret = av_channel_layout_copy(
                &in_layout,
                &codec_ctx->ch_layout);

            if (ret < 0) {
                print_ffmpeg_error("Copie de la disposition audio impossible", ret);
                goto cleanup;
            }
        } else {
            fprintf(stderr, "Disposition des canaux audio inconnue.\n");
            goto cleanup;
        }

        ret = swr_alloc_set_opts2(
            &swr_ctx,
            &out_layout,
            AV_SAMPLE_FMT_S16,
            target_sample_rate,
            &in_layout,
            codec_ctx->sample_fmt,
            codec_ctx->sample_rate,
            0,
            NULL);

        av_channel_layout_uninit(&in_layout);

        if (ret < 0) {
            print_ffmpeg_error("Configuration du reechantillonneur impossible", ret);
            goto cleanup;
        }
    }

#else

    /*
     * Ancienne API :
     * FFmpeg 3.4, Windows XP et AVCodecContext.channel_layout.
     */

    {
        int64_t input_channel_layout = codec_ctx->channel_layout;

        if (!input_channel_layout) {
            input_channel_layout = av_get_default_channel_layout(
                codec_ctx->channels);
        }

        if (!input_channel_layout) {
            fprintf(stderr, "Disposition des canaux audio inconnue.\n");
            goto cleanup;
        }

        swr_ctx = swr_alloc_set_opts(
            NULL,
            AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_S16,
            target_sample_rate,
            input_channel_layout,
            codec_ctx->sample_fmt,
            codec_ctx->sample_rate,
            0,
            NULL);

        if (!swr_ctx) {
            fprintf(stderr, "Allocation du reechantillonneur impossible.\n");
            goto cleanup;
        }
    }

#endif

    if (!swr_ctx) {
        fprintf(stderr, "Contexte de reechantillonnage invalide.\n");
        goto cleanup;
    }

    ret = swr_init(swr_ctx);

    if (ret < 0) {
        print_ffmpeg_error("Initialisation du reechantillonneur impossible", ret);
        goto cleanup;
    }

    /* ----------------------------------------------------- */
    /* Creation du fichier WAV                               */
    /* ----------------------------------------------------- */

    f_wav = fopen(output_filename, "wb");

    if (!f_wav) {
        fprintf(stderr, "Impossible de creer %s\n", output_filename);
        goto cleanup;
    }

    memset(&header, 0, sizeof(header));

    memcpy(header.chunk_id, "RIFF", 4);
    memcpy(header.format, "WAVE", 4);

    memcpy(header.subchunk1_id, "fmt ", 4);

    header.subchunk1_size = 16;
    header.audio_format = 1;
    header.num_channels = TARGET_CHANNELS;
    header.sample_rate = target_sample_rate;
    header.byte_rate = TARGET_BYTE_RATE;
    header.block_align = TARGET_BLOCK_ALIGN;
    header.bits_per_sample = TARGET_BITS;

    memcpy(header.subchunk2_id, "data", 4);
    header.subchunk2_size = 0;

    header.chunk_size = sizeof(wav_header_t) - 8;

    /*
     * Les plateformes cibles (x86/x64) sont little-endian.
     */

    if (fwrite(&header, sizeof(header), 1, f_wav) != 1) {
        fprintf(stderr, "Impossible d'ecrire l'en-tete WAV.\n");
        goto cleanup;
    }

    header_written = 1;

    /* ----------------------------------------------------- */
    /* Allocation des structures de decodage                 */
    /* ----------------------------------------------------- */

    packet = av_packet_alloc();
    frame = av_frame_alloc();

    if (!packet || !frame) {
        fprintf(stderr, "Allocation des structures audio impossible.\n");
        goto cleanup;
    }

    printf("Extraction audio : %s\n", input_filename);
    printf("Sortie           : %s\n", output_filename);
    printf("Format           : 44100 Hz / Stereo / PCM 16 bits\n");

    /* ----------------------------------------------------- */
    /* Boucle de lecture et de decodage                      */
    /* ----------------------------------------------------- */

    while ((ret = av_read_frame(format_ctx, packet)) >= 0) {

        if (packet->stream_index == audio_stream_idx) {

            ret = avcodec_send_packet(codec_ctx, packet);

            /*
             * Si le decodeur demande de recuperer les frames
             * precedentes, on les traite avant de renvoyer le paquet.
             */

            if (ret == AVERROR(EAGAIN)) {
                ret = drain_decoder(
                    codec_ctx,
                    frame,
                    swr_ctx,
                    f_wav,
                    &data_bytes_written);

                if (ret < 0) {
                    av_packet_unref(packet);
                    goto cleanup;
                }

                ret = avcodec_send_packet(codec_ctx, packet);
            }

            if (ret < 0) {
                print_ffmpeg_error("Envoi du paquet au decodeur impossible", ret);
                av_packet_unref(packet);
                goto cleanup;
            }

            ret = drain_decoder(
                codec_ctx,
                frame,
                swr_ctx,
                f_wav,
                &data_bytes_written);

            if (ret < 0) {
                av_packet_unref(packet);
                goto cleanup;
            }
        }

        av_packet_unref(packet);
    }

    if (ret != AVERROR_EOF) {
        print_ffmpeg_error("Erreur de lecture du fichier", ret);
        goto cleanup;
    }

    /* ----------------------------------------------------- */
    /* Vidage final du decodeur                              */
    /* ----------------------------------------------------- */

    ret = avcodec_send_packet(codec_ctx, NULL);

    if (ret == AVERROR(EAGAIN)) {
        ret = drain_decoder(
            codec_ctx,
            frame,
            swr_ctx,
            f_wav,
            &data_bytes_written);

        if (ret < 0)
            goto cleanup;

        ret = avcodec_send_packet(codec_ctx, NULL);
    }

    if (ret < 0 && ret != AVERROR_EOF) {
        print_ffmpeg_error("Vidage du decodeur impossible", ret);
        goto cleanup;
    }

    if (ret != AVERROR_EOF) {
        ret = drain_decoder(
            codec_ctx,
            frame,
            swr_ctx,
            f_wav,
            &data_bytes_written);

        if (ret < 0)
            goto cleanup;
    }

    /* ----------------------------------------------------- */
    /* Vidage final du reechantillonneur                     */
    /* ----------------------------------------------------- */

    ret = flush_resampler(
        swr_ctx,
        codec_ctx->sample_rate,
        f_wav,
        &data_bytes_written);

    if (ret < 0)
        goto cleanup;

    error_code = 0;

cleanup:

    /* ----------------------------------------------------- */
    /* Mise a jour de l'en-tete WAV                           */
    /* ----------------------------------------------------- */

    if (f_wav) {

        if (header_written) {
            header.subchunk2_size = (uint32_t)data_bytes_written;
            header.chunk_size =
                (uint32_t)(sizeof(wav_header_t) - 8 + data_bytes_written);

            if (fseek(f_wav, 0, SEEK_SET) == 0) {
                if (fwrite(&header, sizeof(header), 1, f_wav) != 1) {
                    fprintf(stderr, "Erreur de mise a jour de l'en-tete WAV.\n");
                    error_code = 1;
                }
            } else {
                fprintf(stderr, "Impossible de repositionner le fichier WAV.\n");
                error_code = 1;
            }
        }

        fclose(f_wav);
    }

    /* ----------------------------------------------------- */
    /* Liberation des ressources                             */
    /* ----------------------------------------------------- */

    av_frame_free(&frame);
    av_packet_free(&packet);

    swr_free(&swr_ctx);

    avcodec_free_context(&codec_ctx);

    avformat_close_input(&format_ctx);

    /* ----------------------------------------------------- */
    /* Resultat                                              */
    /* ----------------------------------------------------- */

    if (error_code == 0) {
        printf("\nExtraction audio terminee.\n");
        printf("Fichier : %s\n", output_filename);
        printf("Donnees : %llu octets\n",
               (unsigned long long)data_bytes_written);
    } else {
        fprintf(stderr, "\nEchec de l'extraction audio.\n");
    }

    return error_code;
}

