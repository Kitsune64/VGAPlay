#define _POSIX_C_SOURCE 200809L

#include <png.h>
#include "include/lz4.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* Portability layer between Windows and Linux systems */
#ifdef _WIN32
#include <windows.h>

#define strcasecmp _stricmp
#define strdup _strdup
#define PATH_SEPARATOR '\\'
#else
#include <dirent.h>

#include <strings.h>

#define PATH_SEPARATOR '/'
#endif

#define W 320
#define H 200
#define PIXELS (W * H)
#define HASH_SIZE 131072

typedef struct {
    char * path;
    char * name;
} File;

typedef struct {
    uint8_t r, g, b;
    uint32_t count;
} Color;

typedef struct {
    uint8_t r, g, b, a;
} RGBA;

typedef struct {
    int start, end;
} Box;

static int sort_channel = 0;

/* Lookup cache cache for RGB -> Palette index */
static uint32_t color_map_keys[HASH_SIZE];
static uint8_t color_map_values[HASH_SIZE];

static int compare_files(const void * aa, const void * bb) {
    const File * a = aa, * b = bb;
    const char * x = a->name, * y = b->name;

    while (*x && *y) {
        if (isdigit((unsigned char) *x) && isdigit((unsigned char) *y)) {
            char *ex, *ey;
            unsigned long long nx = strtoull(x, &ex, 10);
            unsigned long long ny = strtoull(y, &ey, 10);

            if (nx != ny)
                return nx < ny ? -1 : 1;

            x = ex;
            y = ey;
            continue;
        }

        int cx = tolower((unsigned char) *x);
        int cy = tolower((unsigned char) *y);

        if (cx != cy) return cx - cy;

        x++;
        y++;
    }

    return (unsigned char) *x - (unsigned char) *y;
}

static int has_extension(const char * name, const char * ext) {
    size_t n = strlen(name);
    size_t e = strlen(ext);
    return n >= e && strcasecmp(name + n - e, ext) == 0;
}

static int read_png(const char * path, RGBA ** pixels, uint32_t * width, uint32_t * height) {
    png_image image;
    memset(&image, 0, sizeof image);
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&image, path)) {
        fprintf(stderr, "PNG Error %s : %s\n", path, image.message);
        return 0;
    }

    image.format = PNG_FORMAT_RGBA;
    RGBA * p = malloc(PNG_IMAGE_SIZE(image));

    if (!p) {
        perror("malloc");
        png_image_free(&image);
        return 0;
    }

    if (!png_image_finish_read(&image, NULL, p, 0, NULL)) {
        fprintf(stderr, "PNG Error %s : %s\n", path, image.message);
        free(p);
        png_image_free(&image);
        return 0;
    }

    *width = image.width;
    *height = image.height;
    *pixels = p;

    png_image_free(&image);
    return 1;
}

static RGBA opaque_pixel(RGBA p) {
    if (p.a != 255) {
        p.r = (uint8_t)((unsigned) p.r * p.a / 255u);
        p.g = (uint8_t)((unsigned) p.g * p.a / 255u);
        p.b = (uint8_t)((unsigned) p.b * p.a / 255u);
    }
    p.a = 255;
    return p;
}

static uint32_t rgb_key(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t) r << 16) | ((uint32_t) g << 8) | b;
}

static uint32_t hash_key(uint32_t k) {
    k ^= k >> 16;
    k *= 0x7feb352du;
    k ^= k >> 15;
    k *= 0x846ca68bu;
    k ^= k >> 16;
    return k;
}

static int color_channel(const Color * c, int channel) {
    if (channel == 0) return c->r;
    if (channel == 1) return c->g;
    return c->b;
}

static int compare_channel(const void * aa, const void * bb) {
    const Color * a = aa;
    const Color * b = bb;
    return color_channel(a, sort_channel) - color_channel(b, sort_channel);
}

static int make_palette(const RGBA * pixels, Color * palette, int max_colors) {
    uint32_t * keys = calloc(HASH_SIZE, sizeof *keys);
    uint32_t * counts = calloc(HASH_SIZE, sizeof *counts);
    Color * hist = malloc(PIXELS * sizeof *hist);

    if (!keys || !counts || !hist) {
        fprintf(stderr, "Insufficient memory for palette allocation.\n");
        free(keys);
        free(counts);
        free(hist);
        return 0;
    }

    int unique = 0;

    for (int i = 0; i < PIXELS; i++) {
        RGBA p = opaque_pixel(pixels[i]);
        uint32_t key = rgb_key(p.r, p.g, p.b);
        size_t slot = hash_key(key) & (HASH_SIZE - 1);

        while (counts[slot] && keys[slot] != key)
            slot = (slot + 1) & (HASH_SIZE - 1);

        if (counts[slot] == 0) {
            keys[slot] = key;
            counts[slot] = 1;
            hist[unique++] = (Color) { p.r, p.g, p.b, 1 };
        } else {
            counts[slot]++;
        }
    }

    for (int i = 0; i < unique; i++) {
        uint32_t key = rgb_key(hist[i].r, hist[i].g, hist[i].b);
        size_t slot = hash_key(key) & (HASH_SIZE - 1);

        while (keys[slot] != key)
            slot = (slot + 1) & (HASH_SIZE - 1);

        hist[i].count = counts[slot];
    }

    free(keys);
    free(counts);

    if (unique <= max_colors) {
        for (int i = 0; i < unique; i++)
            palette[i] = hist[i];
        free(hist);
        return unique;
    }

    Box * boxes = malloc((size_t) max_colors * sizeof *boxes);
    if (!boxes) {
        fprintf(stderr, "Insufficient memory for Median Cut quantization.\n");
        free(hist);
        return 0;
    }

    int box_count = 1;
    boxes[0] = (Box) { 0, unique };

    while (box_count < max_colors) {
        int selected = -1;
        int best_range = -1;

        for (int b = 0; b < box_count; b++) {
            int start = boxes[b].start;
            int end = boxes[b].end;

            if (end - start < 2) continue;

            int minv[3] = { 255, 255, 255 };
            int maxv[3] = { 0, 0, 0 };

            for (int i = start; i < end; i++) {
                int v[3] = { hist[i].r, hist[i].g, hist[i].b };
                for (int c = 0; c < 3; c++) {
                    if (v[c] < minv[c]) minv[c] = v[c];
                    if (v[c] > maxv[c]) maxv[c] = v[c];
                }
            }

            int range = maxv[0] - minv[0];
            if (maxv[1] - minv[1] > range) range = maxv[1] - minv[1];
            if (maxv[2] - minv[2] > range) range = maxv[2] - minv[2];

            if (range > best_range) {
                best_range = range;
                selected = b;
            }
        }

        if (selected < 0) break;

        Box box = boxes[selected];
        int minv[3] = { 255, 255, 255 };
        int maxv[3] = { 0, 0, 0 };

        for (int i = box.start; i < box.end; i++) {
            int v[3] = { hist[i].r, hist[i].g, hist[i].b };
            for (int c = 0; c < 3; c++) {
                if (v[c] < minv[c]) minv[c] = v[c];
                if (v[c] > maxv[c]) maxv[c] = v[c];
            }
        }

        sort_channel = 0;
        if (maxv[1] - minv[1] > maxv[sort_channel] - minv[sort_channel]) sort_channel = 1;
        if (maxv[2] - minv[2] > maxv[sort_channel] - minv[sort_channel]) sort_channel = 2;

        qsort(hist + box.start, (size_t)(box.end - box.start), sizeof *hist, compare_channel);

        uint64_t total = 0;
        for (int i = box.start; i < box.end; i++) total += hist[i].count;

        uint64_t accumulated = 0;
        int split = box.start + 1;

        for (int i = box.start; i < box.end - 1; i++) {
            accumulated += hist[i].count;
            split = i + 1;
            if (accumulated * 2 >= total) break;
        }

        boxes[selected].end = split;
        boxes[box_count++] = (Box) { split, box.end };
    }

    for (int b = 0; b < box_count; b++) {
        uint64_t sr = 0, sg = 0, sb = 0, total = 0;
        for (int i = boxes[b].start; i < boxes[b].end; i++) {
            uint64_t n = hist[i].count;
            sr += (uint64_t) hist[i].r * n;
            sg += (uint64_t) hist[i].g * n;
            sb += (uint64_t) hist[i].b * n;
            total += n;
        }
        if (total == 0) total = 1;

        palette[b].r = (uint8_t)((sr + total / 2) / total);
        palette[b].g = (uint8_t)((sg + total / 2) / total);
        palette[b].b = (uint8_t)((sb + total / 2) / total);
        palette[b].count = (uint32_t) total;
    }

    free(boxes);
    free(hist);
    return box_count;
}

static int nearest_color(const Color * palette, int n, RGBA p) {
    unsigned best_distance = UINT_MAX;
    int best = 0;

    for (int i = 0; i < n; i++) {
        int dr = (int) p.r - palette[i].r;
        int dg = (int) p.g - palette[i].g;
        int db = (int) p.b - palette[i].b;
        unsigned d = (unsigned)(dr * dr + dg * dg + db * db);

        if (d < best_distance) {
            best_distance = d;
            best = i;
        }
    }
    return best;
}

static void write_u8(FILE * f, uint8_t v) {
    if (fwrite(&v, 1, 1, f) != 1) {
        perror("write error");
        exit(EXIT_FAILURE);
    }
}

static void write_u16le(FILE * f, uint16_t v) {
    uint8_t b[2] = { (uint8_t) v, (uint8_t)(v >> 8) };
    if (fwrite(b, 1, 2, f) != 2) {
        perror("write error");
        exit(EXIT_FAILURE);
    }
}

static void write_i32le(FILE * f, int32_t v) {
    uint32_t u = (uint32_t) v;
    uint8_t b[4] = { (uint8_t) u, (uint8_t)(u >> 8), (uint8_t)(u >> 16), (uint8_t)(u >> 24) };
    if (fwrite(b, 1, 4, f) != 4) {
        perror("write error");
        exit(EXIT_FAILURE);
    }
}

static void usage(const char * program) {
    fprintf(stderr,
        "Usage: %s <DIRECTORY> <PREFIX> <EXTENSION> <FRAME_COUNT> <FPS> <MAX_COLORS>\n"
        "Example: %s ./PNG video PNG 2853 15 256\n",
        program, program);
}

int main(int argc, char ** argv) {
    if (argc != 7) {
        usage(argv[0]);
        return 2;
    }

    char * end;
    long frame_count = strtol(argv[4], &end, 10);
    if (*end || frame_count < 1 || frame_count > 65535) {
        fprintf(stderr, "Invalid frame count (1-65535).\n");
        return 2;
    }

    long fps = strtol(argv[5], &end, 10);
    if (*end || fps < 1 || fps > 255) {
        fprintf(stderr, "Invalid FPS rate (1-255).\n");
        return 2;
    }

    long max_colors = strtol(argv[6], &end, 10);

    if (*end || max_colors < 1 || max_colors > 256) {
        fprintf(stderr, "Invalid max colors limit (1-256).\n");
        return 2;
    }
    File * files = calloc((size_t) frame_count, sizeof *files);
    if (!files) {
        perror("calloc");
        return 1;
    }
    size_t nfiles = 0;
    #ifdef _WIN32
    WIN32_FIND_DATAA entry;
    char search_path[2048];
    size_t path_len = strlen(argv[1]);
    if (path_len > 0 && (argv[1][path_len - 1] == '/' || argv[1][path_len - 1] == '\\')) {
        snprintf(search_path, sizeof(search_path), "%s*", argv[1]);
    } else {
        snprintf(search_path, sizeof(search_path), "%s\\*", argv[1]);
    }
    HANDLE dir = FindFirstFileA(search_path, &entry);
    if (dir == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening directory: %s\n", argv[1]);
        free(files);
        return 1;
    }
    do {
        if (entry.cFileName[0] == '.' || !has_extension(entry.cFileName, argv[3]))
            continue;
        if (nfiles >= (size_t) frame_count) {
            fprintf(stderr, "More files found than requested; surplus ignored.\n");
            continue;
        }
        size_t len = strlen(argv[1]) + strlen(entry.cFileName) + 2;
        files[nfiles].path = malloc(len);
        files[nfiles].name = strdup(entry.cFileName);
        if (!files[nfiles].path || !files[nfiles].name) {
            perror("file allocation error");
            FindClose(dir);
            return 1;
        }
        snprintf(files[nfiles].path, len, "%s%c%s", argv[1], PATH_SEPARATOR, entry.cFileName);
        nfiles++;
    } while (FindNextFileA(dir, &entry));
    FindClose(dir);
    #else
    DIR * dir = opendir(argv[1]);
    if (!dir) {
        perror(argv[1]);
        free(files);
        return 1;
    }
    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' || !has_extension(entry->d_name, argv[3]))
            continue;
        if (nfiles >= (size_t) frame_count) {
            fprintf(stderr, "More files found than requested; surplus ignored.\n");
            continue;
        }
        size_t len = strlen(argv[1]) + strlen(entry->d_name) + 2;
        files[nfiles].path = malloc(len);
        files[nfiles].name = strdup(entry->d_name);
        if (!files[nfiles].path || !files[nfiles].name) {
            perror("file allocation error");
            closedir(dir);
            return 1;
        }
        snprintf(files[nfiles].path, len, "%s%c%s", argv[1], PATH_SEPARATOR, entry->d_name);
        nfiles++;
    }
    closedir(dir);
    #endif
    qsort(files, nfiles, sizeof *files, compare_files);
    if (nfiles < (size_t) frame_count) {
        fprintf(stderr, "Requested frames: %ld ; Found: %zu.\n", frame_count, nfiles);
        for (size_t i = 0; i < nfiles; i++) {
            free(files[i].path);
            free(files[i].name);
        }
        free(files);
        return 1;
    }
    char output_name[1024];
    if (snprintf(output_name, sizeof output_name, "%s.BIN", argv[2]) >= (int) sizeof output_name) {
        fprintf(stderr, "Output prefix name is too long.\n");
        for (size_t i = 0; i < nfiles; i++) {
            free(files[i].path);
            free(files[i].name);
        }
        free(files);
        return 2;
    }
    FILE * out = fopen(output_name, "wb");
    if (!out) {
        perror(output_name);
        for (size_t i = 0; i < nfiles; i++) {
            free(files[i].path);
            free(files[i].name);
        }
        free(files);
        return 1;
    }
    /* 1. GLOBAL HEADER GENERATION */
    write_u16le(out, (uint16_t) frame_count);
    write_u8(out, (uint8_t) fps);
    write_u8(out, 0); // 32-bit Alignment Padding byte
    /* 2. RESERVE INDEX TABLE SPACE (Size = frame_count * 4 bytes) */
    int32_t * index_table = calloc((size_t) frame_count, sizeof(int32_t));
    if (!index_table) {
        perror("index_table calloc error");
        fclose(out);
        return 1;
    }
    long index_table_offset = ftell(out);
    fwrite(index_table, sizeof(int32_t), (size_t) frame_count, out);
    /* --- LZ4 CONFIGURATION & STREAM SETUP --- */
    LZ4_stream_t * stream = LZ4_createStream();
    uint8_t * indices[2] = {
        malloc(PIXELS),
        malloc(PIXELS)
    };
    char * compressed = malloc((size_t) LZ4_compressBound(PIXELS));
    if (!stream || !indices[0] || !indices[1] || !compressed) {
        fprintf(stderr, "Memory allocation workflow error.\n");
        fclose(out);
        free(index_table);
        if (stream) LZ4_freeStream(stream);
        free(indices[0]);
        free(indices[1]);
        free(compressed);
        return 1;
    }
    /* 3. ENCODING MAIN LOOP */
    for (long frame = 0; frame < frame_count; frame++) {
        RGBA * pixels = NULL;
        uint32_t width, height;
        if (!read_png(files[frame].path, &pixels, &width, &height)) {
            break;
        }
        if (width != W || height != H) {
            fprintf(stderr, "%s : %ux%u ; expected 320x200.\n", files[frame].name, width, height);
        free(pixels);
            break;
        }
        Color palette[256];
        int palette_count = make_palette(pixels, palette, (int) max_colors);
        if (palette_count < 1 || palette_count > max_colors) {
            fprintf(stderr, "Invalid palette layout context: %s\n", files[frame].name);
            free(pixels);
            break;
        }
        uint8_t * idx = indices[frame & 1];
        memset(color_map_keys, 0, sizeof color_map_keys);
        for (int i = 0; i < PIXELS; i++) {
            RGBA p = opaque_pixel(pixels[i]);
            uint32_t key = rgb_key(p.r, p.g, p.b);
            uint32_t stored_key = key + 1;
            size_t slot = hash_key(key) & (HASH_SIZE - 1);
            while (color_map_keys[slot] != 0 && color_map_keys[slot] != stored_key) {
                slot = (slot + 1) & (HASH_SIZE - 1);
            }
            uint8_t palette_index;
            if (color_map_keys[slot] == stored_key) {
                palette_index = color_map_values[slot];
            } else {
                palette_index = (uint8_t) nearest_color(palette, palette_count, p);
                color_map_keys[slot] = stored_key;
                color_map_values[slot] = palette_index;
            }
            idx[i] = palette_index;
        }
        free(pixels);
        /* DOS FIXED PALETTE OPTIMIZATION: Always flush a uniform 768 bytes packet */
        for (int i = 0; i < 256; i++) {
            if (i < palette_count) {
                write_u8(out, palette[i].r);
                write_u8(out, palette[i].g);
                write_u8(out, palette[i].b);
            } else {
                write_u8(out, 0);
                write_u8(out, 0);
                write_u8(out, 0);
            }
        }
        /* DEPENDENT LZ4 COMPRESSION LAYER */
        int compressed_size = LZ4_compress_fast_continue(
            stream, (const char *) idx, compressed, PIXELS, LZ4_compressBound(PIXELS), 1);
        if (compressed_size <= 0) {
            fprintf(stderr, "LZ4 pipeline error at frame %ld.\n", frame + 1);
            break;
        }
        // Cache block size locally for retroactive injection mapping
        index_table[frame] = compressed_size;
        // Flush standalone pure payload data
        if (fwrite(compressed, 1, (size_t) compressed_size, out) != (size_t) compressed_size) {
            perror("LZ4 raw block fwrite error");
            break;
        }
        if ((frame + 1) % 100 == 0 || frame + 1 == frame_count)
            fprintf(stderr, "\rProcessed frames: %ld/%ld", frame + 1, frame_count);
    }
    fprintf(stderr, "\n");
    /* 4. RETROACTIVE INDEX TABLE INJECTION MAP */
    long end_of_file_offset = ftell(out);
    if (fseek(out, index_table_offset, SEEK_SET) == 0) {
        for (long i = 0; i < frame_count; i++) {
            write_i32le(out, index_table[i]);
        }
        fseek(out, end_of_file_offset, SEEK_SET);
    } else {
        perror("Index table retro-fseek failure notification");
    }
    /* Resources Cleanup */
    fclose(out);
    free(index_table);
    LZ4_freeStream(stream);
    free(indices[0]);
    free(indices[1]);
    free(compressed);
    for (size_t i = 0; i < nfiles; i++) {
        free(files[i].path);
        free(files[i].name);
    }
    free(files);
    printf("DOS Optimized binary package successfully created: %s\n", output_name);
    return 0;
}
