#define _POSIX_C_SOURCE 200809L
#include <png.h>
#include "include/lz4.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 200
#define PIXELS (W * H)

typedef struct {
    uint8_t r, g, b;
} RGBColor;

/* Writes the 8-bit indexed buffer and its palette directly into a PNG file */
static int save_png(const char *filename, const uint8_t *indices, const RGBColor *palette, int colors) {
    png_image image;
    memset(&image, 0, sizeof image);
    image.version = PNG_IMAGE_VERSION;
    image.width = W;
    image.height = H;
    image.format = PNG_FORMAT_RGB_COLORMAP;
    image.colormap_entries = colors;

    return png_image_write_to_file(&image, filename, 0, indices, 0, palette);
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage Mode 1 (Analysis) : %s <video.bin>\n", argv[0]);
        fprintf(stderr, "Usage Mode 2 (Extract)  : %s <video.bin> <frame_index> <output.png>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Error opening file");
        return 1;
    }

    int target_extract_frame = -1;
    const char *output_png_path = NULL;

    if (argc == 4) {
        target_extract_frame = atoi(argv[2]);
        output_png_path = argv[3];
    }

    // 1. READ GLOBAL HEADER (4 bytes)
    uint16_t frame_count = 0;
    uint8_t fps = 0;
    uint8_t padding = 0;

    if (fread(&frame_count, 2, 1, f) != 1 || 
        fread(&fps, 1, 1, f) != 1 || 
        fread(&padding, 1, 1, f) != 1) {
        fprintf(stderr, "Error: Invalid or corrupted global file header.\n");
        fclose(f);
        return 1;
    }

    if (target_extract_frame > 0 && target_extract_frame > frame_count) {
        fprintf(stderr, "Error: Requested frame %d, but file only contains %u frames.\n", target_extract_frame, frame_count);
        fclose(f);
        return 1;
    }

    printf("=== FRAMEPACK BINARY VERIFICATOR ===\n");
    printf("Total Frames : %u\n", frame_count);
    printf("Target FPS   : %u (Duration: %.2f seconds)\n", fps, (double)frame_count / fps);
    printf("------------------------------------\n");

    // 2. READ THE ENTIRE INDEX TABLE
    int32_t *index_table = malloc((size_t)frame_count * sizeof(int32_t));
    if (!index_table) {
        perror("Error allocating memory for index table");
        fclose(f);
        return 1;
    }

    if (fread(index_table, sizeof(int32_t), (size_t)frame_count, f) != (size_t)frame_count) {
        fprintf(stderr, "Error: Failed to read the complete Index Table.\n");
        free(index_table);
        fclose(f);
        return 1;
    }

    uint64_t total_compressed_size = 0;
    uint64_t total_palette_size = (uint64_t)frame_count * 768; // 256 colors * 3 bytes per frame fixed
    
    RGBColor current_palette[256];
    uint8_t *extracted_indices = malloc(PIXELS);
    char *lz4_buffer = malloc((size_t)LZ4_compressBound(PIXELS));
    LZ4_streamDecode_t *lz4_stream = LZ4_createStreamDecode();

    if (!extracted_indices || !lz4_buffer || !lz4_stream) {
        fprintf(stderr, "Memory allocation error.\n");
        if (lz4_stream) LZ4_freeStreamDecode(lz4_stream);
        free(extracted_indices);
        free(lz4_buffer);
        free(index_table);
        fclose(f);
        return 1;
    }

    int extraction_success = 0;

    // 3. STREAM AND DECOMPRESS ALL FRAMES SÉQUENTIALLY
    for (uint16_t i = 1; i <= frame_count; i++) {
        int32_t lz4_size = index_table[i - 1];
        total_compressed_size += lz4_size;

        // Read fixed 768-byte palette chunk directly
        if (fread(current_palette, 1, 768, f) != 768) {
            fprintf(stderr, "Error: Invalid palette data structure at frame %u.\n", i);
            break;
        }

        // Read raw LZ4 frame data block using size fetched from the Index Table
        if (fread(lz4_buffer, 1, (size_t)lz4_size, f) != (size_t)lz4_size) {
            fprintf(stderr, "Error: Could not read full LZ4 block for frame %u.\n", i);
            break;
        }
        
        // Continuous stream dependent decompression loop
        int decomp_bytes = LZ4_decompress_safe_continue(
            lz4_stream, lz4_buffer, (char *)extracted_indices, lz4_size, PIXELS
        );

        if (decomp_bytes != PIXELS) {
            fprintf(stderr, "Error: LZ4 decompression failed at frame %u (Got %d bytes).\n", i, decomp_bytes);
            break;
        }

        // Export requested frame directly with its full fixed 256-color colormap context
        if (target_extract_frame == i) {
            extraction_success = save_png(output_png_path, extracted_indices, current_palette, 256);
        }
    }

    long current_pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);

    printf("Validation Summary:\n");
    printf("  File Size          : %ld bytes\n", file_size);
    printf("  Palette Overhead   : %" PRIu64 " bytes\n", total_palette_size);
    printf("  Compressed Video   : %" PRIu64 " bytes\n", total_compressed_size);
    
    double raw_size = (double)frame_count * PIXELS;
    printf("  Pure Video Ratio   : %.2f%%\n", ((double)total_compressed_size / raw_size) * 100.0);

    if (current_pos == file_size) {
        printf("\nResult: SUCCESS (File structure is perfectly valid).\n");
    } else {
        printf("\nResult: WARNING (%ld trailing or junk bytes detected at EOF).\n", file_size - current_pos);
    }

    if (target_extract_frame != -1) {
        if (extraction_success) {
            printf("Extraction: Frame %d successfully exported to '%s'.\n", target_extract_frame, output_png_path);
        } else {
            fprintf(stderr, "Extraction: FAILED to export frame %d.\n", target_extract_frame);
        }
    }

    LZ4_freeStreamDecode(lz4_stream);
    free(extracted_indices);
    free(lz4_buffer);
    free(index_table);
    fclose(f);
    return 0;
}
