/*
  PNGExtract - MP4 to Indexed 8-bit PNG Converter using FFmpeg and libpng
  License: MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FFmpeg (libav) headers */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>


/* libpng header */
#include "png.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

#define TARGET_WIDTH 320
#define TARGET_HEIGHT 200
#define VIDEO_TARGET_HEIGHT 160

AVFilterGraph *filter_graph = NULL;
AVFilterContext *buffersrc_ctx = NULL;
AVFilterContext *buffersink_ctx = NULL;

/* Mode 1: Fixed height to 160, proportional width, centered on a 320x200 canvas */
void resize_and_pad(uint8_t *src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h) {
    memset(dst, 0, dst_w * dst_h * 3);
    float scale = (float)VIDEO_TARGET_HEIGHT / (float)src_h;
    int new_h = VIDEO_TARGET_HEIGHT;
    int new_w = (int)((float)src_w * scale + 0.5f);
    if (new_w > dst_w) new_w = dst_w;

    int offset_x = (dst_w - new_w) / 2;
    int offset_y = (dst_h - new_h) / 2;

    for (int y = 0; y < new_h; y++) {
        int src_y = (y * src_h) / new_h;
        for (int x = 0; x < new_w; x++) {
            int src_x = (x * src_w) / new_w;
            int src_idx = (src_y * src_w + src_x) * 3;
            int dst_idx = ((y + offset_y) * dst_w + (x + offset_x)) * 3;
            dst[dst_idx]     = src[src_idx];
            dst[dst_idx + 1] = src[src_idx + 1];
            dst[dst_idx + 2] = src[src_idx + 2];
        }
    }
}

/* Mode 0: Direct stretching to full screen 320x200 */
void resize_image_direct(uint8_t *src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h) {
    for (int y = 0; y < dst_h; y++) {
        int src_y = (y * src_h) / dst_h;
        for (int x = 0; x < dst_w; x++) {
            int src_x = (x * src_w) / dst_w;
            int src_idx = (src_y * src_w + src_x) * 3;
            int dst_idx = (y * dst_w + x) * 3;
            dst[dst_idx]     = src[src_idx];
            dst[dst_idx + 1] = src[src_idx + 1];
            dst[dst_idx + 2] = src[src_idx + 2];
        }
    }
}

/* High-quality Floyd-Steinberg dithering and quantization (8-bit palette) */
void quantize_floyd_steinberg(uint8_t *rgb_pixels, int width, int height, int max_colors, 
                              uint8_t *indexed_pixels, png_colorp palette, int *palette_size) {
    int r_levels = 8, g_levels = 8, b_levels = 4;
    if (max_colors <= 16) { r_levels = 2; g_levels = 2; b_levels = 2; }
    else if (max_colors <= 64) { r_levels = 4; g_levels = 4; b_levels = 4; }

    int p_idx = 0;
    for (int r = 0; r < r_levels; r++) {
        for (int g = 0; g < g_levels; g++) {
            for (int b = 0; b < b_levels; b++) {
                if (p_idx >= max_colors) break;
                palette[p_idx].red   = (r * 255) / (r_levels - 1);
                palette[p_idx].green = (g * 255) / (g_levels - 1);
                palette[p_idx].blue  = (b * 255) / (b_levels - 1);
                p_idx++;
            }
        }
    }
    *palette_size = p_idx;

    int *error_buf = (int *)malloc(width * height * 3 * sizeof(int));
    for (int i = 0; i < width * height * 3; i++) error_buf[i] = rgb_pixels[i];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            int closest_color_idx = 0;
            int min_distance = 9999999;

            int r_val = error_buf[idx];     if (r_val < 0) r_val = 0; if (r_val > 255) r_val = 255;
            int g_val = error_buf[idx + 1]; if (g_val < 0) g_val = 0; if (g_val > 255) g_val = 255;
            int b_val = error_buf[idx + 2]; if (b_val < 0) b_val = 0; if (b_val > 255) b_val = 255;

            for (int i = 0; i < *palette_size; i++) {
                int dr = r_val - palette[i].red;
                int dg = g_val - palette[i].green;
                int db = b_val - palette[i].blue;
                int distance = dr*dr + dg*dg + db*db;
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_color_idx = i;
                }
            }

            indexed_pixels[y * width + x] = closest_color_idx;

            int err_r = r_val - palette[closest_color_idx].red;
            int err_g = g_val - palette[closest_color_idx].green;
            int err_b = b_val - palette[closest_color_idx].blue;

            if (x + 1 < width) {
                error_buf[idx + 3]     += (err_r * 7) / 16;
                error_buf[idx + 4]     += (err_g * 7) / 16;
                error_buf[idx + 5]     += (err_b * 7) / 16;
            }
            if (y + 1 < height) {
                if (x - 1 >= 0) {
                    int n_idx = ((y + 1) * width + (x - 1)) * 3;
                    error_buf[n_idx]     += (err_r * 3) / 16;
                    error_buf[n_idx + 1] += (err_g * 3) / 16;
                    error_buf[n_idx + 2] += (err_b * 3) / 16;
                }
                int n_idx = ((y + 1) * width + x) * 3;
                error_buf[n_idx]     += (err_r * 5) / 16;
                error_buf[n_idx + 1] += (err_g * 5) / 16;
                error_buf[n_idx + 2] += (err_b * 5) / 16;
                if (x + 1 < width) {
                    int n_idx_p = ((y + 1) * width + (x + 1)) * 3;
                    error_buf[n_idx_p]     += (err_r * 1) / 16;
                    error_buf[n_idx_p + 1] += (err_g * 1) / 16;
                    error_buf[n_idx_p + 2] += (err_b * 1) / 16;
                }
            }
        }
    }
    free(error_buf);
}

/* Save the file as an 8-bit indexed PNG */
int write_png_indexed(const char *path, int width, int height, uint8_t *indexed_pixels, png_colorp palette, int palette_size) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { fclose(fp); return 0; }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_write_struct(&png_ptr, NULL); fclose(fp); return 0; }
    if (setjmp(png_jmpbuf(png_ptr))) { png_destroy_write_struct(&png_ptr, &info_ptr); fclose(fp); return 0; }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_PLTE(png_ptr, info_ptr, palette, palette_size);
    png_write_info(png_ptr, info_ptr);

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) row_pointers[y] = indexed_pixels + y * width;

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
    free(row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        printf("Usage: PNGExtract <file.mp4> <output_dir> <fps> <max_colors> <padding_mode (0/1)>\n");
        return 1;
    }

    /* --- Required legacy registration for Windows XP (FFmpeg < 4.0) --- */
    #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
		av_register_all();
		avfilter_register_all();
	#endif

    char *input_filename = argv[1];
    char *output_dir = argv[2];
    double target_fps = atof(argv[3]);
    int max_colors = atoi(argv[4]);
    int padding_mode = atoi(argv[5]);

    MKDIR(output_dir);

    AVFormatContext *format_ctx = NULL;
    if (avformat_open_input(&format_ctx, input_filename, NULL, NULL) != 0) {
        printf("Could not open file %s\n", input_filename);
        return 1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) return 1;

    int video_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) return 1;

    AVCodecParameters *codec_params = format_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_params);
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) return 1;

    // ==========================================
    // INITIALISATION UNIQUE DU FILTRE EN AMONT
    // ==========================================
    filter_graph = avfilter_graph_alloc();
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    if (!filter_graph || !buffersrc || !buffersink || !outputs || !inputs) {
        fprintf(stderr, "Failed to allocate filter components\n");
        return 1;
    }

	AVRational time_base = format_ctx->streams[video_stream_idx]->time_base;
    char args[512];
    
#if defined(__linux__)
    // --- 1. Linux : FFmpeg récent, exige le range explicite ---
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:range=tv",
             codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
             time_base.num, time_base.den,
             codec_ctx->sample_aspect_ratio.num, codec_ctx->sample_aspect_ratio.den);

#elif defined(_WIN64)
    // --- 2. Windows 11 64-bit (Mise à jour : ajout de range=tv) ---
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:range=tv",
             codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
             time_base.num, time_base.den,
             codec_ctx->sample_aspect_ratio.num, codec_ctx->sample_aspect_ratio.den);

#elif defined(_WIN32)
    #if defined(__WINDOWS_XP__)
    // --- 3. Windows XP 32-bit ---
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
             time_base.num, time_base.den,
             codec_ctx->sample_aspect_ratio.num, codec_ctx->sample_aspect_ratio.den);
    #else
	// --- 4. Windows 11 32-bit (Mise à jour : ajout de range=tv) ---
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:range=tv",
             codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
             time_base.num, time_base.den,
             codec_ctx->sample_aspect_ratio.num, codec_ctx->sample_aspect_ratio.den);
    #endif
#endif

    if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph) < 0 ||
        avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph) < 0) {
        fprintf(stderr, "Failed to create filter instances\n");
        return 1;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    char filter_spec[128];
    snprintf(filter_spec, sizeof(filter_spec), "scale=%d:%d,fps=%.2f", codec_ctx->width, codec_ctx->height, target_fps);

    if (avfilter_graph_parse_ptr(filter_graph, filter_spec, &inputs, &outputs, NULL) < 0 ||
        avfilter_graph_config(filter_graph, NULL) < 0) {
        fprintf(stderr, "Failed to parse or configure filter graph\n");
        return 1;
    }
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    // ==========================================

    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    AVFrame *frame_rgb = av_frame_alloc();
    
    int w = codec_ctx->width;
    int h = codec_ctx->height;
    
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    uint8_t *raw_pixels = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    av_image_fill_arrays(frame_rgb->data, frame_rgb->linesize, raw_pixels, AV_PIX_FMT_RGB24, w, h, 1);

    struct SwsContext *sws_ctx = sws_getContext(w, h, codec_ctx->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_LANCZOS, NULL, NULL, NULL);

    uint8_t *canvas_pixels = (uint8_t *)malloc(TARGET_WIDTH * TARGET_HEIGHT * 3);
    uint8_t *indexed_pixels = (uint8_t *)malloc(TARGET_WIDTH * TARGET_HEIGHT * 1);
    
    png_color palette[256];
    int palette_size = 0;

  AVPacket *packet = av_packet_alloc();
    int file_index = 0;

    printf("FFmpeg extraction processing with CFR filter (FPS: %.1f)...\n", target_fps);

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
    
                    // Forcer le range TV pour correspondre au filtre sous Linux / FFmpeg récent
                    #if defined(__linux__) || defined(_WIN64) || !defined(__WINDOWS_XP__)
                    if (frame->color_range == AVCOL_RANGE_UNSPECIFIED) {
                        frame->color_range = AVCOL_RANGE_MPEG;
                    }
                    #endif

                    // Neutraliser le chroma_loc pour tout harmoniser
                    frame->chroma_location = AVCHROMA_LOC_UNSPECIFIED;

					// 1. Pousser la frame décodée brute dans le filtre source
                    if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        fprintf(stderr, "Error while feeding the filtergraph\n");
                        break;
                    }

                    // 2. Extraire du filtre les frames rigoureusement cadencées (CFR)
                    while (1) {
                        int ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                        if (ret < 0) break;

                        // Traitement de la frame filtrée (CFR strict)
                        sws_scale(sws_ctx, (uint8_t const * const *)filt_frame->data, filt_frame->linesize, 0, h, frame_rgb->data, frame_rgb->linesize);
                        
                        if (padding_mode == 1) {
                            resize_and_pad(raw_pixels, w, h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
                        } else {
                            resize_image_direct(raw_pixels, w, h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
                        }
                        
                        quantize_floyd_steinberg(canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT, max_colors, indexed_pixels, palette, &palette_size);
                        
                        char png_name[256];
                        sprintf(png_name, "%s/G_%06d.PNG", output_dir, file_index);
                        write_png_indexed(png_name, TARGET_WIDTH, TARGET_HEIGHT, indexed_pixels, palette, palette_size);
                        
                        // --- AFFICHAGE DE LA PROGRESSION ---
                        double current_time = (double)(file_index) / target_fps;
                        printf("\r[Extraction] Frame: %d | Time: %.2fs", file_index, current_time);
                        fflush(stdout);
                        
                        file_index++;
                        av_frame_unref(filt_frame);
                    }
                }
            }
        }
        av_packet_unref(packet);
    }
    
    // --- FLUSH DU FILTRE (Drainage des dernières frames en cache) ---
    if (buffersrc_ctx) {
        if (av_buffersrc_add_frame_flags(buffersrc_ctx, NULL, 0) < 0) {
            fprintf(stderr, "Avertissement : échec de la purge du filtre\n");
        }
        while (1) {
            int ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;

            sws_scale(sws_ctx, (uint8_t const * const *)filt_frame->data, filt_frame->linesize, 0, h, frame_rgb->data, frame_rgb->linesize);
            
            if (padding_mode == 1) {
                resize_and_pad(raw_pixels, w, h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
            } else {
                resize_image_direct(raw_pixels, w, h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
            }
            
            quantize_floyd_steinberg(canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT, max_colors, indexed_pixels, palette, &palette_size);
            
            char png_name[256];
            sprintf(png_name, "%s/G_%06d.PNG", output_dir, file_index);
            write_png_indexed(png_name, TARGET_WIDTH, TARGET_HEIGHT, indexed_pixels, palette, palette_size);
            
            double current_time = (double)(file_index) / target_fps;
            printf("\r[Extraction - Flush] Frame: %d | Time: %.2fs", file_index, current_time);
            fflush(stdout);
            
            file_index++;
            av_frame_unref(filt_frame);
        }
    }

    printf("\nDone! %d image frames generated.\n", file_index);
    
    // Nettoyage complet
    avfilter_graph_free(&filter_graph);
    free(indexed_pixels); 
    free(canvas_pixels);
    av_free(raw_pixels);
    av_frame_free(&frame_rgb); 
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    av_packet_free(&packet);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return 0;
}

