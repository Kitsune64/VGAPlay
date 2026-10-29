/* Distributed under the MIT License. See README.TXT for details. */
#ifndef DECODER_H_
#define DECODER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "lz4.h"
#include "error.h"
#include "fileio.h"

#define BLOCK_BYTES 64000

#define ERROR_DECODE "Decode Error"

struct video_header {
    word frame_count;
    byte frame_rate;
};

struct audio_header {
    word frame_count;
    byte frame_rate;
};

struct audio_frame {
    word frame;
    byte action;
    word frequency;
};

struct video_header decode_video_header(FILE *file);

void decode_video_frame(FILE *file, int frame_index, byte *palette,
                        int *palette_size, byte *dest);

void decode_video_reset(void);

/* ADDED: Declaration of the cleanup function to resolve the main.c compiler error */
void decode_video_free(void);

struct audio_header decode_audio_header(FILE *file);

struct audio_frame decode_audio_frame(FILE *file);

#endif /* DECODER_H_ */
