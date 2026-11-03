/*
 * FramePack.c
 *
 * Packe toutes les images PNG indexees d'un dossier dans un fichier BIN.
 *
 * Arguments:
 *   FramePack <DIRECTORY> <PREFIX> <FPS> <MAX_COLORS>
 *
 * Exemple:
 *   FramePack ./PNG video 15 256
 *
 * Le format BIN actuel est conserve:
 *   - uint16 little-endian : nombre d'images
 *   - uint8                : FPS
 *   - uint8                : padding
 *   - une table de tailles LZ4 int32 little-endian
 *   - pour chaque image :
 *       768 octets de palette RGB
 *       bloc LZ4 des 64000 indices
 *
 * Deux passages:
 *   1. Inventaire, tri, SHA-256, validation et analyse des palettes.
 *   2. Nouvelle enumeration/verifications, puis creation du BIN.
 *
 * Aucune quantification, conversion RGB ou remappage des indices.
 */

#define _POSIX_C_SOURCE 200809L

#include <png.h>
#include "include/lz4.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define PATH_SEPARATOR '\\'
#define strcasecmp _stricmp
#else
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#endif

#define W 320
#define H 200
#define PIXELS (W * H)
#define PALETTE_BYTES 768
#define SHA256_BYTES 32

typedef struct {
    char *path;
    char *name;
    uint8_t sha256[SHA256_BYTES];
    uint8_t palette[PALETTE_BYTES];
    int palette_count;
    int used_colors;
} FileInfo;

typedef struct {
    png_structp png;
    png_infop info;
    FILE *file;
    uint8_t *pixels;
    png_bytep *rows;
} PngState;

/* ---------------------------------------------------------
   SHA-256 portable (aucune dependance externe)
   --------------------------------------------------------- */

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t buffer[64];
    size_t buffer_used;
} SHA256_CTX;

static const uint32_t sha256_k[64] = {
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,
    0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,
    0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,
    0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,
    0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,
    0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,
    0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,
    0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,
    0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};

static uint32_t rotr32(uint32_t x, unsigned n)
{
    return (x >> n) | (x << (32U - n));
}

static void sha256_transform(SHA256_CTX *ctx, const uint8_t block[64])
{
    uint32_t w[64];
    uint32_t a,b,c,d,e,f,g,h;
    unsigned i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               (uint32_t)block[i * 4 + 3];
    }

    for (i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i - 15], 7) ^
                      rotr32(w[i - 15], 18) ^
                      (w[i - 15] >> 3);
        uint32_t s1 = rotr32(w[i - 2], 17) ^
                      rotr32(w[i - 2], 19) ^
                      (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = ctx->state[0]; b = ctx->state[1];
    c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5];
    g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + s1 + ch + sha256_k[i] + w[i];
        uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + maj;

        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b;
    ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f;
    ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx)
{
    static const uint32_t initial[8] = {
        0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
        0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U
    };

    memcpy(ctx->state, initial, sizeof(initial));
    ctx->bit_count = 0;
    ctx->buffer_used = 0;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len)
{
    while (len > 0) {
        size_t take = 64 - ctx->buffer_used;
        if (take > len)
            take = len;

        memcpy(ctx->buffer + ctx->buffer_used, data, take);
        ctx->buffer_used += take;
        data += take;
        len -= take;
        ctx->bit_count += (uint64_t)take * 8U;

        if (ctx->buffer_used == 64) {
            sha256_transform(ctx, ctx->buffer);
            ctx->buffer_used = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t digest[SHA256_BYTES])
{
    uint64_t bits = ctx->bit_count;
    unsigned i;

    ctx->buffer[ctx->buffer_used++] = 0x80;

    if (ctx->buffer_used > 56) {
        while (ctx->buffer_used < 64)
            ctx->buffer[ctx->buffer_used++] = 0;
        sha256_transform(ctx, ctx->buffer);
        ctx->buffer_used = 0;
    }

    while (ctx->buffer_used < 56)
        ctx->buffer[ctx->buffer_used++] = 0;

    for (i = 0; i < 8; i++)
        ctx->buffer[63 - i] = (uint8_t)(bits >> (i * 8));

    sha256_transform(ctx, ctx->buffer);

    for (i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)ctx->state[i];
    }
}

static int sha256_file(const char *path, uint8_t digest[SHA256_BYTES])
{
    FILE *f = fopen(path, "rb");
    uint8_t buffer[16384];
    size_t n;
    SHA256_CTX ctx;

    if (!f) {
        fprintf(stderr, "Cannot open for SHA-256: %s\n", path);
        return 0;
    }

    sha256_init(&ctx);

    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
        sha256_update(&ctx, buffer, n);

    if (ferror(f)) {
        fprintf(stderr, "Read error while hashing: %s\n", path);
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Close error while hashing: %s\n", path);
        return 0;
    }

    sha256_final(&ctx, digest);
    return 1;
}

/* ---------------------------------------------------------
   File helpers and numeric sorting
   --------------------------------------------------------- */

static char *duplicate_string(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

static int has_png_extension(const char *name)
{
    size_t n = strlen(name);
    return n >= 4 && strcasecmp(name + n - 4, ".png") == 0;
}

static int compare_files(const void *aa, const void *bb)
{
    const FileInfo *a = (const FileInfo *)aa;
    const FileInfo *b = (const FileInfo *)bb;
    const char *x = a->name;
    const char *y = b->name;

    while (*x && *y) {
        if (isdigit((unsigned char)*x) &&
            isdigit((unsigned char)*y)) {
            char *ex, *ey;
            unsigned long long nx = strtoull(x, &ex, 10);
            unsigned long long ny = strtoull(y, &ey, 10);

            if (nx != ny)
                return nx < ny ? -1 : 1;

            x = ex;
            y = ey;
            continue;
        }

        {
            int cx = tolower((unsigned char)*x);
            int cy = tolower((unsigned char)*y);
            if (cx != cy)
                return cx - cy;
        }

        x++;
        y++;
    }

    if (*x || *y)
        return (unsigned char)*x - (unsigned char)*y;

    /* Deterministic tie-break for names differing only by case. */
    return strcmp(a->name, b->name);
}

static int make_path(char *dst, size_t dst_size,
                     const char *directory, const char *name)
{
    size_t n = strlen(directory);

    if (n > 0 &&
        (directory[n - 1] == '/' || directory[n - 1] == '\\')) {
        int written = snprintf(dst, dst_size, "%s%s", directory, name);
        return written >= 0 && (size_t)written < dst_size;
    }

    {
        int written = snprintf(dst, dst_size, "%s%c%s",
                               directory, PATH_SEPARATOR, name);
        return written >= 0 && (size_t)written < dst_size;
    }
}

static int append_file(FileInfo **files, size_t *count, size_t *capacity,
                       const char *directory, const char *name)
{
    FileInfo *grown;
    size_t new_capacity;
    char path[4096];

    if (!make_path(path, sizeof(path), directory, name)) {
        fprintf(stderr, "Path too long: %s\n", name);
        return 0;
    }

    if (*count == *capacity) {
        new_capacity = (*capacity == 0) ? 64 : (*capacity * 2);

        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(FileInfo)) {
            fprintf(stderr, "Too many PNG files.\n");
            return 0;
        }

        grown = (FileInfo *)realloc(
            *files, new_capacity * sizeof(FileInfo));
        if (!grown) {
            fprintf(stderr, "Not enough memory for file list.\n");
            return 0;
        }

        *files = grown;
        *capacity = new_capacity;
    }

    memset(&(*files)[*count], 0, sizeof(FileInfo));

    (*files)[*count].name = duplicate_string(name);
    (*files)[*count].path = duplicate_string(path);

    if (!(*files)[*count].name || !(*files)[*count].path) {
        free((*files)[*count].name);
        free((*files)[*count].path);
        memset(&(*files)[*count], 0, sizeof(FileInfo));
        fprintf(stderr, "Not enough memory for file name.\n");
        return 0;
    }

    (*count)++;
    return 1;
}

static void free_file_list(FileInfo *files, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        free(files[i].name);
        free(files[i].path);
    }
    free(files);
}

static int enumerate_png_files(const char *directory,
                               FileInfo **files_out,
                               size_t *count_out)
{
    FileInfo *files = NULL;
    size_t count = 0, capacity = 0;

#ifdef _WIN32
    {
        WIN32_FIND_DATAA entry;
        char pattern[4096];
        size_t n = strlen(directory);
        HANDLE h;

        if (n > 0 &&
            (directory[n - 1] == '/' || directory[n - 1] == '\\')) {
            if (snprintf(pattern, sizeof(pattern), "%s*", directory) >=
                (int)sizeof(pattern)) {
                fprintf(stderr, "Directory path too long.\n");
                return 0;
            }
        } else {
            if (snprintf(pattern, sizeof(pattern), "%s\\*", directory) >=
                (int)sizeof(pattern)) {
                fprintf(stderr, "Directory path too long.\n");
                return 0;
            }
        }

        h = FindFirstFileA(pattern, &entry);
        if (h == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Cannot open directory: %s\n", directory);
            return 0;
        }

        do {
            if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            if (!has_png_extension(entry.cFileName))
                continue;

            if (!append_file(&files, &count, &capacity,
                             directory, entry.cFileName)) {
                FindClose(h);
                free_file_list(files, count);
                return 0;
            }
        } while (FindNextFileA(h, &entry));

        FindClose(h);
    }
#else
    {
        DIR *dir = opendir(directory);
        struct dirent *entry;

        if (!dir) {
            fprintf(stderr, "Cannot open directory %s: %s\n",
                    directory, strerror(errno));
            return 0;
        }

        while ((entry = readdir(dir)) != NULL) {
            char path[4096];
            struct stat st;

            if (!has_png_extension(entry->d_name))
                continue;
            if (entry->d_name[0] == '.')
                continue;
            if (!make_path(path, sizeof(path), directory, entry->d_name))
                continue;
            if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
                continue;

            if (!append_file(&files, &count, &capacity,
                             directory, entry->d_name)) {
                closedir(dir);
                free_file_list(files, count);
                return 0;
            }
        }

        closedir(dir);
    }
#endif

    qsort(files, count, sizeof(FileInfo), compare_files);

    *files_out = files;
    *count_out = count;
    return 1;
}

/* ---------------------------------------------------------
   PNG indexed reader
   --------------------------------------------------------- */

static int read_png_indexed(const char *path,
                            uint8_t **pixels,
                            uint8_t palette_rgb[PALETTE_BYTES],
                            int *palette_count,
                            int *used_colors)
{
    PngState *s;
    png_uint_32 w, h;
    png_byte color_type, bit_depth;
    png_colorp png_palette;
    int entries, i;
    png_size_t rowbytes;
    uint8_t seen_rgb[256][3];
    volatile int unique_count = 0;

    *pixels = NULL;

    s = (PngState *)calloc(1, sizeof(PngState));
    if (!s) {
        fprintf(stderr, "PNG: insufficient memory.\n");
        return 0;
    }

    s->file = fopen(path, "rb");
    if (!s->file) {
        fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
        free(s);
        return 0;
    }

    s->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!s->png) {
        fprintf(stderr, "PNG: cannot create read structure.\n");
        fclose(s->file);
        free(s);
        return 0;
    }

    s->info = png_create_info_struct(s->png);
    if (!s->info) {
        fprintf(stderr, "PNG: cannot create info structure.\n");
        png_destroy_read_struct(&s->png, NULL, NULL);
        fclose(s->file);
        free(s);
        return 0;
    }

    if (setjmp(png_jmpbuf(s->png))) {
        fprintf(stderr, "PNG decoding error: %s\n", path);
        free(s->rows);
        free(s->pixels);
        png_destroy_read_struct(&s->png, &s->info, NULL);
        fclose(s->file);
        free(s);
        return 0;
    }

    png_init_io(s->png, s->file);
    png_read_info(s->png, s->info);

    w = png_get_image_width(s->png, s->info);
    h = png_get_image_height(s->png, s->info);
    color_type = png_get_color_type(s->png, s->info);
    bit_depth = png_get_bit_depth(s->png, s->info);

    if (color_type != PNG_COLOR_TYPE_PALETTE) {
        fprintf(stderr, "%s: indexed PNG required.\n", path);
        goto png_error;
    }

    if (w != W || h != H) {
        fprintf(stderr, "%s: %ux%u; expected %dx%d.\n",
                path, (unsigned)w, (unsigned)h, W, H);
        goto png_error;
    }

    if (bit_depth != 1 && bit_depth != 2 &&
        bit_depth != 4 && bit_depth != 8) {
        fprintf(stderr, "%s: unsupported indexed bit depth: %u.\n",
                path, (unsigned)bit_depth);
        goto png_error;
    }

    if (png_get_valid(s->png, s->info, PNG_INFO_tRNS)) {
        fprintf(stderr, "%s: PNG transparency (tRNS) is not supported.\n",
                path);
        goto png_error;
    }

    if (!png_get_PLTE(s->png, s->info, &png_palette, &entries)) {
        fprintf(stderr, "%s: missing PNG palette (PLTE).\n", path);
        goto png_error;
    }

    if (entries < 1 || entries > 256) {
        fprintf(stderr, "%s: invalid palette size: %d.\n", path, entries);
        goto png_error;
    }

    if (bit_depth < 8)
        png_set_packing(s->png);

    png_set_interlace_handling(s->png);
    png_read_update_info(s->png, s->info);

    rowbytes = png_get_rowbytes(s->png, s->info);
    if (rowbytes != W) {
        fprintf(stderr, "%s: unexpected indexed row size: %lu.\n",
                path, (unsigned long)rowbytes);
        goto png_error;
    }

    s->pixels = (uint8_t *)malloc(PIXELS);
    s->rows = (png_bytep *)malloc((size_t)H * sizeof(png_bytep));

    if (!s->pixels || !s->rows) {
        fprintf(stderr, "%s: insufficient memory for image.\n", path);
        goto png_error;
    }

    for (i = 0; i < H; i++)
        s->rows[i] = s->pixels + (size_t)i * W;

    png_read_image(s->png, s->rows);
    png_read_end(s->png, NULL);

    memset(palette_rgb, 0, PALETTE_BYTES);
    for (i = 0; i < entries; i++) {
        palette_rgb[i * 3]     = png_palette[i].red;
        palette_rgb[i * 3 + 1] = png_palette[i].green;
        palette_rgb[i * 3 + 2] = png_palette[i].blue;
    }

    /*
     * Compte les couleurs RGB distinctes reellement utilisees
     * par les pixels, et non simplement le nombre d'entrees PLTE.
     */
    for (i = 0; i < PIXELS; i++) {
        unsigned index = s->pixels[i];
        int j;
        int found = 0;

        if (index >= (unsigned)entries) {
            fprintf(stderr, "%s: invalid palette index at pixel %d.\n",
                    path, i);
            goto png_error;
        }

        for (j = 0; j < unique_count; j++) {
            if (seen_rgb[j][0] == palette_rgb[index * 3] &&
                seen_rgb[j][1] == palette_rgb[index * 3 + 1] &&
                seen_rgb[j][2] == palette_rgb[index * 3 + 2]) {
                found = 1;
                break;
            }
        }

        if (!found) {
            seen_rgb[unique_count][0] = palette_rgb[index * 3];
            seen_rgb[unique_count][1] = palette_rgb[index * 3 + 1];
            seen_rgb[unique_count][2] = palette_rgb[index * 3 + 2];
            unique_count++;
        }
    }

    *palette_count = entries;
    *used_colors = unique_count;
    *pixels = s->pixels;
    s->pixels = NULL;

    free(s->rows);
    png_destroy_read_struct(&s->png, &s->info, NULL);
    fclose(s->file);
    free(s);
    return 1;

png_error:
    free(s->rows);
    free(s->pixels);
    png_destroy_read_struct(&s->png, &s->info, NULL);
    fclose(s->file);
    free(s);
    return 0;
}

/* ---------------------------------------------------------
   Little-endian output
   --------------------------------------------------------- */

static int write_u8(FILE *f, uint8_t v)
{
    return fwrite(&v, 1, 1, f) == 1;
}

static int write_u16le(FILE *f, uint16_t v)
{
    uint8_t b[2];
    b[0] = (uint8_t)v;
    b[1] = (uint8_t)(v >> 8);
    return fwrite(b, 1, 2, f) == 2;
}

static int write_i32le(FILE *f, int32_t v)
{
    uint32_t u = (uint32_t)v;
    uint8_t b[4];

    b[0] = (uint8_t)u;
    b[1] = (uint8_t)(u >> 8);
    b[2] = (uint8_t)(u >> 16);
    b[3] = (uint8_t)(u >> 24);

    return fwrite(b, 1, 4, f) == 4;
}


/* ---------------------------------------------------------
   Usage
   --------------------------------------------------------- */

static void usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s <DIRECTORY> <PREFIX> <FPS> <MAX_COLORS>\n"
        "Example: %s ./PNG video 15 256\n",
        program, program);
}

/* ---------------------------------------------------------
   Main
   --------------------------------------------------------- */

int main(int argc, char **argv)
{
    long fps, max_colors;
    char *end;

    FileInfo *files = NULL;
    size_t file_count = 0;

    FileInfo *files_check = NULL;
    size_t check_count = 0;

    FILE *out = NULL;
    char output_name[1024] = "";

    int32_t *index_table = NULL;
    long index_table_offset = -1;

    LZ4_stream_t *stream = NULL;
    uint8_t *indices[2] = { NULL, NULL };
    char *compressed = NULL;

    size_t i;
    size_t palette_changes = 0;
    size_t unchanged_palettes = 0;
    size_t max_used_colors = 0;

    long frame;
    int success = 0;

    if (argc != 5) {
        usage(argv[0]);
        return 2;
    }

    fps = strtol(argv[3], &end, 10);
    if (*end || fps < 1 || fps > 255) {
        fprintf(stderr, "Invalid FPS rate (1-255).\n");
        return 2;
    }

    max_colors = strtol(argv[4], &end, 10);
    if (*end || max_colors < 1 || max_colors > 256) {
        fprintf(stderr, "Invalid MAX_COLORS limit (1-256).\n");
        return 2;
    }

    /* -----------------------------------------------------
       PASSAGE 1: inventory and analysis
       ----------------------------------------------------- */

    fprintf(stderr, "Pass 1: scanning PNG directory...\n");

    if (!enumerate_png_files(argv[1], &files, &file_count))
        goto cleanup;

    if (file_count == 0) {
        fprintf(stderr, "No PNG files found in %s.\n", argv[1]);
        goto cleanup;
    }

    if (file_count > 65535U) {
        fprintf(stderr,
            "Too many PNG files: %zu. BIN format limit is 65535.\n",
            file_count);
        goto cleanup;
    }

    fprintf(stderr, "PNG files found: %zu\n", file_count);
    fprintf(stderr, "Analyzing images and calculating SHA-256...\n");

    for (i = 0; i < file_count; i++) {
        uint8_t *pixels = NULL;
        uint8_t palette[PALETTE_BYTES];
        int palette_count = 0;
        int used_colors = 0;

        if (!sha256_file(files[i].path, files[i].sha256))
            goto cleanup;

        if (!read_png_indexed(files[i].path, &pixels, palette,
                              &palette_count, &used_colors)) {
            fprintf(stderr, "Cannot analyze PNG: %s\n", files[i].name);
            goto cleanup;
        }

        free(pixels);

        files[i].palette_count = palette_count;
        files[i].used_colors = used_colors;
        memcpy(files[i].palette, palette, PALETTE_BYTES);

        if ((long)used_colors > max_colors) {
            fprintf(stderr,
                "%s: %d distinct RGB colors are used; MAX_COLORS is %ld.\n",
                files[i].name, used_colors, max_colors);
            goto cleanup;
        }

        if ((size_t)used_colors > max_used_colors)
            max_used_colors = (size_t)used_colors;

        if (i == 0 || memcmp(files[i - 1].palette,
                             files[i].palette, PALETTE_BYTES) != 0) {
            palette_changes++;
        } else {
            unchanged_palettes++;
        }

        if ((i + 1) % 100 == 0 || i + 1 == file_count) {
            fprintf(stderr, "\rAnalyzed images: %zu/%zu", i + 1, file_count);
        }
    }

    fprintf(stderr, "\n\nPalette analysis:\n");
    fprintf(stderr, "  Images:                         %zu\n", file_count);
    fprintf(stderr, "  Palette changes (first = load): %zu\n",
            palette_changes);
    fprintf(stderr, "  Unchanged consecutive palettes: %zu\n",
            unchanged_palettes);
    fprintf(stderr, "  Maximum used RGB colors/image:  %zu\n",
            max_used_colors);

    /*
     * Estimation for a future one-byte FLAGS design:
     * one flag per image; a full 768-byte palette only when changed.
     * This is informational only. Current BIN remains unchanged.
     */
    {
        uint64_t potential_saving =
            (uint64_t)unchanged_palettes * PALETTE_BYTES -
            (uint64_t)file_count;

        if (potential_saving > 0) {
            fprintf(stderr,
                "  Estimated saving with FLAGS:   %llu bytes "
                "(before format overhead)\n",
                (unsigned long long)potential_saving);
        } else {
            fprintf(stderr,
                "  Estimated saving with FLAGS:   0 bytes "
                "(the flag overhead cancels the gain)\n");
        }
    }

    /* -----------------------------------------------------
       PASSAGE 2: re-enumeration and integrity checks
       ----------------------------------------------------- */

    fprintf(stderr, "\nPass 2: verifying source directory...\n");

    if (!enumerate_png_files(argv[1], &files_check, &check_count))
        goto cleanup;

    if (check_count != file_count) {
        fprintf(stderr,
            "Directory changed: first pass found %zu PNG files, "
            "second pass found %zu.\n",
            file_count, check_count);
        goto cleanup;
    }

    for (i = 0; i < file_count; i++) {
        uint8_t second_hash[SHA256_BYTES];

        if (strcmp(files[i].name, files_check[i].name) != 0) {
            fprintf(stderr,
                "Directory changed: file %zu was '%s', now '%s'.\n",
                i + 1, files[i].name, files_check[i].name);
            goto cleanup;
        }

        if (!sha256_file(files_check[i].path, second_hash))
            goto cleanup;

        if (memcmp(files[i].sha256, second_hash, SHA256_BYTES) != 0) {
            fprintf(stderr,
                "File changed between passes: %s\n",
                files[i].name);
            goto cleanup;
        }
    }

    fprintf(stderr, "Directory and PNG contents verified.\n");

    /* -----------------------------------------------------
       Output filename
       ----------------------------------------------------- */

    if (snprintf(output_name, sizeof(output_name), "%s.BIN", argv[2]) >=
        (int)sizeof(output_name)) {
        fprintf(stderr, "Output prefix name is too long.\n");
        goto cleanup;
    }

    out = fopen(output_name, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create %s: %s\n",
                output_name, strerror(errno));
        goto cleanup;
    }

    /* -----------------------------------------------------
       BIN header (unchanged)
       ----------------------------------------------------- */

    if (!write_u16le(out, (uint16_t)file_count) ||
        !write_u8(out, (uint8_t)fps) ||
        !write_u8(out, 0)) {
        fprintf(stderr, "Cannot write BIN header.\n");
        goto cleanup;
    }

    /* -----------------------------------------------------
       Reserve frame index table (unchanged)
       ----------------------------------------------------- */

    index_table = (int32_t *)calloc(file_count, sizeof(int32_t));
    if (!index_table) {
        fprintf(stderr, "Cannot allocate LZ4 index table.\n");
        goto cleanup;
    }

    index_table_offset = ftell(out);
    if (index_table_offset < 0) {
        fprintf(stderr, "Cannot determine index table offset.\n");
        goto cleanup;
    }

    for (i = 0; i < file_count; i++) {
        if (!write_i32le(out, 0)) {
            fprintf(stderr, "Cannot reserve index table.\n");
            goto cleanup;
        }
    }

    /* -----------------------------------------------------
       LZ4 stream configuration (unchanged)
       ----------------------------------------------------- */

    stream = LZ4_createStream();
    indices[0] = (uint8_t *)malloc(PIXELS);
    indices[1] = (uint8_t *)malloc(PIXELS);
    compressed = (char *)malloc((size_t)LZ4_compressBound(PIXELS));

    if (!stream || !indices[0] || !indices[1] || !compressed) {
        fprintf(stderr, "Memory allocation error for LZ4.\n");
        goto cleanup;
    }

    /* -----------------------------------------------------
       Encoding loop
       ----------------------------------------------------- */

    fprintf(stderr, "\nCreating BIN...\n");

    for (frame = 0; frame < (long)file_count; frame++) {
        uint8_t *pixels = NULL;
        uint8_t palette[PALETTE_BYTES];
        uint8_t current_hash[SHA256_BYTES];
        int palette_count = 0;
        int used_colors = 0;
        int compressed_size;
        uint8_t *idx = indices[frame & 1];

        /*
         * Hash after reading the source file in this pass.
         * It must still match the SHA-256 captured in pass 1.
         */
        if (!read_png_indexed(files[frame].path, &pixels, palette,
                              &palette_count, &used_colors)) {
            fprintf(stderr, "Cannot reread PNG: %s\n", files[frame].name);
            goto cleanup;
        }

        if (!sha256_file(files[frame].path, current_hash)) {
            free(pixels);
            goto cleanup;
        }

        if (memcmp(files[frame].sha256, current_hash, SHA256_BYTES) != 0) {
            fprintf(stderr,
                "File changed while BIN was being created: %s\n",
                files[frame].name);
            free(pixels);
            goto cleanup;
        }

        if (memcmp(files[frame].palette, palette, PALETTE_BYTES) != 0 ||
            files[frame].palette_count != palette_count ||
            files[frame].used_colors != used_colors) {
            fprintf(stderr,
                "PNG analysis mismatch between passes: %s\n",
                files[frame].name);
            free(pixels);
            goto cleanup;
        }

        /*
         * Preserve original indexed pixels and original RGB palette.
         * No remapping, quantization or dithering.
         */
        memcpy(idx, pixels, PIXELS);
        free(pixels);

        if (fwrite(palette, 1, PALETTE_BYTES, out) != PALETTE_BYTES) {
            fprintf(stderr, "Palette write error at frame %ld.\n",
                    frame + 1);
            goto cleanup;
        }

        /*
         * Keep the existing dependent LZ4 compression call.
         */
        compressed_size = LZ4_compress_fast_continue(
            stream,
            (const char *)idx,
            compressed,
            PIXELS,
            LZ4_compressBound(PIXELS),
            1
        );

        if (compressed_size <= 0) {
            fprintf(stderr, "LZ4 compression error at frame %ld.\n",
                    frame + 1);
            goto cleanup;
        }

        index_table[frame] = compressed_size;

        if (fwrite(compressed, 1, (size_t)compressed_size, out) !=
            (size_t)compressed_size) {
            fprintf(stderr, "LZ4 write error at frame %ld.\n", frame + 1);
            goto cleanup;
        }

        if ((frame + 1) % 100 == 0 || frame + 1 == (long)file_count) {
            fprintf(stderr, "\rPacked images: %ld/%zu",
                    frame + 1, file_count);
        }
    }

    fprintf(stderr, "\n");

    /* -----------------------------------------------------
       Write final index table
       ----------------------------------------------------- */

    if (fseek(out, index_table_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Index table seek error.\n");
        goto cleanup;
    }

    for (i = 0; i < file_count; i++) {
        if (!write_i32le(out, index_table[i])) {
            fprintf(stderr, "Index table write error.\n");
            goto cleanup;
        }
    }

    if (fflush(out) != 0) {
        fprintf(stderr, "Output flush error.\n");
        goto cleanup;
    }

    success = 1;

cleanup:

    if (out) {
        if (fclose(out) != 0 && success) {
            fprintf(stderr, "Output close error.\n");
            success = 0;
        }
    }

    if (stream)
        LZ4_freeStream(stream);

    free(indices[0]);
    free(indices[1]);
    free(compressed);
    free(index_table);

    free_file_list(files, file_count);
    free_file_list(files_check, check_count);

    if (!success) {
        if (output_name[0] != '\0')
            remove(output_name);

        fprintf(stderr,
            "FramePack failed. No valid BIN was produced.\n");
        return 1;
    }

    printf("BIN successfully created: %s\n", output_name);
    printf("Images packed: %zu\n", file_count);
    return 0;
}
