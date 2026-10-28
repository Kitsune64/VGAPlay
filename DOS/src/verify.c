#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "types.h"
#include "decoder.h"
#include "fileio.h"
#include "error.h"
#include "lz4.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: VERIFY.EXE <path_to_video.bin>\n");
        return 1;
    }

    const char *video_path = argv[1];
    FILE *file = fopen(video_path, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", video_path);
        return 1;
    }

    FILE *report = fopen("verify.log", "w");
    if (!report) {
        printf("Warning: Could not create verify.log file\n");
    }

    // 1. Read Global Header with structural 32-bit alignment padding
    uint16_t frame_count = 0;
    uint8_t fps = 0;
    uint8_t padding = 0;

    if (fread(&frame_count, 2, 1, file) != 1 || 
        fread(&fps, 1, 1, file) != 1 || 
        fread(&padding, 1, 1, file) != 1) {
        printf("Error: Invalid or corrupted global binary file header.\n");
        fclose(file);
        if (report) fclose(report);
        return 1;
    }
    
    printf("=== DOS VERIFICATION REPORT: %s ===\n", video_path);
    printf("Declared Frame Count : %d\n", frame_count);
    printf("Framerate            : %d fps\n\n", fps);

    if (report) {
        fprintf(report, "=== DOS VERIFICATION REPORT: %s ===\n", video_path);
        fprintf(report, "Declared Frame Count : %d\n", frame_count);
        fprintf(report, "Framerate            : %d fps\n\n", fps);
    }

    // 2. Load the entire Index Table from disk into local RAM
    int32_t *index_table = malloc((size_t)frame_count * sizeof(int32_t));
    if (!index_table) {
        printf("Error: Index table heap allocation failed.\n");
        fclose(file);
        if (report) fclose(report);
        return 1;
    }

    if (fread(index_table, sizeof(int32_t), (size_t)frame_count, file) != (size_t)frame_count) {
        printf("Error: Failed to read the complete Index Table from disk.\n");
        free(index_table);
        fclose(file);
        if (report) fclose(report);
        return 1;
    }

    LZ4_streamDecode_t *lz4StreamDecode = LZ4_createStreamDecode();
    int errors_found = 0;

    char *temp_decBuf = malloc(BLOCK_BYTES);
    char *lz4_buffer = malloc(LZ4_COMPRESSBOUND(BLOCK_BYTES));

    if (!temp_decBuf || !lz4_buffer || !lz4StreamDecode) {
        printf("Error: Operational buffers memory allocation failed.\n");
        free(index_table); free(temp_decBuf); free(lz4_buffer);
        fclose(file); if (report) fclose(report);
        return 1;
    }

    // 3. Process stream verification using index sizing offset mapping
    for (int frame = 0; frame < frame_count; frame++) {
        int32_t cmpBytes = index_table[frame];

        // Skip the fixed 768-byte palette block safely
        if (fseek(file, 768, SEEK_CUR) != 0) {
            printf("[Frame %d] Error: Palette stream alignment corruption.\n", frame);
            errors_found++;
            break;
        }

        // Fetch raw dependent LZ4 payload block using index boundaries
        if (fread(lz4_buffer, 1, (size_t)cmpBytes, file) != (size_t)cmpBytes) {
            printf("[Frame %d] Error: Failed reading %d bytes payload block.\n", frame, (int) cmpBytes);
            errors_found++;
            break;
        }

        const int decBytes = LZ4_decompress_safe_continue(
            lz4StreamDecode, lz4_buffer, temp_decBuf, cmpBytes, BLOCK_BYTES
        );

        if (decBytes != BLOCK_BYTES) {
            printf("[Frame %d] LZ4 DECOMPRESSION ERROR (Got %d bytes instead of 64000)\n", frame, decBytes);
            if (report) fprintf(report, "[Frame %d] LZ4 DECOMPRESSION ERROR (Got %d bytes)\n", frame, decBytes);
            errors_found++;
        }
    }

    // Resources cleanup
    free(index_table);
    free(temp_decBuf);
    free(lz4_buffer);
    free(lz4StreamDecode);
    fclose(file);
    
    printf("\n=== VERIFICATION FINISHED ===\n");
    printf("Total decompression errors found: %d\n", errors_found);

    if (report) {
        fprintf(report, "\n=== VERIFICATION FINISHED ===\n");
        fprintf(report, "Total decompression errors found: %d\n", errors_found);
        fclose(report);
    }
    return 0;
}
