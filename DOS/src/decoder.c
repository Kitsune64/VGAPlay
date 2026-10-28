#include "decoder.h"

LZ4_streamDecode_t lz4StreamDecode_body;
LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

/* Unified contiguous ring buffer for frame history preservation */
char *video_ring_buffer = NULL;
int decBufIndex = 0;
char *cmpBuf = NULL;

/* Array pointers to hold the pre-fetched Index Table in RAM */
int32_t *video_index_table = NULL;

struct video_header decode_video_header(FILE *file) {
    struct video_header hd;
    uint8_t padding = 0;

    read_word(file, &hd.frame_count);
    read_byte(file, &hd.frame_rate);
    read_byte(file, &padding);

    /* Clean old allocations if they exist from a previous playback */
    if (video_index_table != NULL) { free(video_index_table); video_index_table = NULL; }
    if (video_ring_buffer != NULL) { free(video_ring_buffer); video_ring_buffer = NULL; }
    if (cmpBuf != NULL)            { free(cmpBuf);            cmpBuf = NULL;            }

    /* Allocate Index Table mapping array */
    video_index_table = (int32_t *)malloc((size_t)hd.frame_count * sizeof(int32_t));
    
    /* Allocate a single, unified 128KB block to guarantee 
       absolute memory contiguity for continuous LZ4 stream decompression */
    video_ring_buffer = (char *)malloc(BLOCK_BYTES * 2);
    
    /* Allocate input buffer with extra safety boundaries */
    cmpBuf = (char *)malloc(131072); 

    if (!video_index_table || !video_ring_buffer || !cmpBuf) {
        raise_error("DOS_MEMORY_ALLOCATION_FAILED");
    }

    read_bin(file, video_index_table, (size_t)hd.frame_count * sizeof(int32_t));

    return hd;
}

/* FIXED: reset now ONLY resets the LZ4 decompression state, it DOES NOT free the active buffers */
void decode_video_reset() {
    LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);
    decBufIndex = 0;
}

/* NEW: Dedicated function to release heap memory once the cinematic is completely finished */
void decode_video_free() {
    if (video_index_table != NULL) { free(video_index_table); video_index_table = NULL; }
    if (video_ring_buffer != NULL) { free(video_ring_buffer); video_ring_buffer = NULL; }
    if (cmpBuf != NULL)            { free(cmpBuf);            cmpBuf = NULL;            }
}

void decode_video_frame(FILE *file, int frame_index, byte *palette, int *palette_size, byte *dest) {
    int cmpBytes = video_index_table[frame_index];

    *palette_size = 256;
    read_bin(file, palette, 768);

    if (cmpBytes <= 0 || cmpBytes > 128000) {
        raise_error(ERROR_DECODE);
    }
    read_bin(file, cmpBuf, (size_t)cmpBytes);

    char* const decPtr = video_ring_buffer + (decBufIndex * BLOCK_BYTES);

    const int decBytes = LZ4_decompress_safe_continue(
        lz4StreamDecode,
        cmpBuf,
        decPtr,
        cmpBytes,
        BLOCK_BYTES
    );

    if (decBytes != BLOCK_BYTES) {
        raise_error(ERROR_DECODE);
    }

    memcpy(dest, decPtr, BLOCK_BYTES);
    decBufIndex = (decBufIndex + 1) % 2;
}

struct audio_header decode_audio_header(FILE *file) {
    struct audio_header ah;
    read_word(file, &ah.frame_count);
    read_byte(file, &ah.frame_rate);
    return ah;
}

struct audio_frame decode_audio_frame(FILE *file) {
    struct audio_frame af;
    read_word(file, &af.frame);
    read_byte(file, &af.action);

    if (af.action) {
        read_word(file, &af.frequency);
    }

    return af;
}
