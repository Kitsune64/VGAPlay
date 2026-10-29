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

/* Mode 1: Fixed height to 160, proportional width, centered on a 320x200 canvas */
void resize_and_pad(uint8_t *src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h) {
    /* Fill the canvas with black (background color) */
    memset(dst, 0, dst_w * dst_h * 3);
    
    /* Calculate scale based on the target height (160px) to keep aspect ratio */
    float scale = (float)VIDEO_TARGET_HEIGHT / (float)src_h;
    int new_h = VIDEO_TARGET_HEIGHT;
    int new_w = (int)((float)src_w * scale + 0.5f);
    if (new_w > dst_w) new_w = dst_w;

    /* Calculate centering offsets */
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

/* Main orchestration via FFmpeg API */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: PNGExtract <file.mp4> [fps] [max_colors] [padding_mode (0/1)]\n");
        printf("Example: PNGExtract video.mp4 15 256 1\n");
        return 1;
    }

    char *input_filename = argv[1];
    double target_fps = (argc >= 3) ? atof(argv[2]) : 15.0;
    int max_colors = (argc >= 4) ? atoi(argv[3]) : 256;
    int padding_mode = (argc >= 5) ? atoi(argv[4]) : 1;

        /* --- Force clean old PNG directory content --- */
		#if defined(_WIN32) || defined(_WIN64)
			/* Windows command to silently delete the directory and all its contents */
			system("if exist PNG rmdir /s /q PNG");
		#else
			/* Linux / macOS command to forcefully delete the directory */
			system("rm -rf PNG");
		#endif

			/* Create a fresh, empty PNG directory */
			MKDIR("PNG");

    /* 1. Open the video file using FFmpeg */
    AVFormatContext *format_ctx = NULL;
    if (avformat_open_input(&format_ctx, input_filename, NULL, NULL) != 0) {
        printf("Could not open file %s\n", input_filename);
        return 1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) return 1;

    /* Find the video stream */
    int video_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) return 1;

    /* Set up the decoder context */
    AVCodecParameters *codec_params = format_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_params);
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) return 1;

    /* 2. Prepare structures for image processing and buffer allocations */
    AVFrame *frame = av_frame_alloc();
    AVFrame *frame_rgb = av_frame_alloc();
    
    int w = codec_ctx->width;
    int h = codec_ctx->height;
    
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    uint8_t *raw_pixels = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    av_image_fill_arrays(frame_rgb->data, frame_rgb->linesize, raw_pixels, AV_PIX_FMT_RGB24, w, h, 1);

    /* Initialize the software scaling engine to convert native format to RGB24 */
    struct SwsContext *sws_ctx = sws_getContext(w, h, codec_ctx->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_LANCZOS, NULL, NULL, NULL);

    uint8_t *canvas_pixels = (uint8_t *)malloc(TARGET_WIDTH * TARGET_HEIGHT * 3);
    uint8_t *indexed_pixels = (uint8_t *)malloc(TARGET_WIDTH * TARGET_HEIGHT * 1);
    png_color palette[256];
    int palette_size = 0;

    AVPacket *packet = av_packet_alloc();
    int file_index = 0;
    
    /* Time management logic to enforce the requested FPS rate */
    double time_base = av_q2d(format_ctx->streams[video_stream_idx]->time_base);
    double next_sample_time = 0.0;

    printf("FFmpeg extraction processing (FPS: %.1f)...\n", target_fps);

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
if (avcodec_send_packet(codec_ctx, packet) >= 0) {
while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
double frame_time = frame->pts * time_base;
/* Skip frames to maintain target frame-rate */
if (file_index > 0 && frame_time < next_sample_time) {
continue;
}
next_sample_time = frame_time + (1.0 / target_fps);
/* Convert native pixel format to raw RGB24 */
sws_scale(sws_ctx, (uint8_t const * const *)frame->data, frame->linesize, 0, h, frame_rgb->data, frame_rgb->linesize);
/* Crop logic: Remove 4 pixels from the top boundary */
int crop_top = 4;
int new_w = w;
int new_h = h - crop_top;
uint8_t *cropped_pixels = (uint8_t *)malloc(new_w * new_h * 3);
for (int y = 0; y < new_h; y++) {
memcpy(cropped_pixels + y * new_w * 3, raw_pixels + (y + crop_top) * frame_rgb->linesize[0], new_w * 3);
}
/* Apply resizing rules based on chosen padding mode */
if (padding_mode == 1) {
resize_and_pad(cropped_pixels, new_w, new_h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
} else {
resize_image_direct(cropped_pixels, new_w, new_h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
}
/* Run palette quantization and save final 8-bit PNG file */
quantize_floyd_steinberg(canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT, max_colors, indexed_pixels, palette, &palette_size);
char png_name[64];
sprintf(png_name, "PNG/G_%06d.PNG", file_index);
write_png_indexed(png_name, TARGET_WIDTH, TARGET_HEIGHT, indexed_pixels, palette, palette_size);
printf("\r[Extraction] Frame: %d | Time: %.2fs", file_index, frame_time);
fflush(stdout);
free(cropped_pixels);
file_index++;
}
}
}
av_packet_unref(packet);
}
printf("\nDone! %d image frames generated.\n", file_index);
/* Full FFmpeg memory context and buffer cleanup */
free(indexed_pixels); free(canvas_pixels);
av_free(raw_pixels);
av_frame_free(&frame_rgb); av_frame_free(&frame);
av_packet_free(&packet);
sws_freeContext(sws_ctx);
avcodec_free_context(&codec_ctx);
avformat_close_input(&format_ctx);
return 0;
}
