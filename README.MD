# VGAPlay

A VGA video player (fork of DOSVideoPlayer by Netdex) and format converter for DOS systems.

### [Demonstration Video (Workflow of the new version on Linux and Dosbox-X Emulator)](https://www.youtube.com/watch?v=j21uid9JJi8)

## Description

This is a Video Player compatible with DOS systems (MS-DOS, FreeDOS, DOSBox Emulator) written in C using the DJGPP compiler with the Allegro 4.2 library.

You can find a full set of DJGPP compiler environment for DOS with Allegro 4.2 AND Sound Blaster fix here:  
https://www.mrdictionary.net/allegro/

The player reads a custom binary file (`.BIN`) generated from a series of PNG images. The frames are processed, converted into an 8-bit indexed color format, and compressed using **LZ4 dependent stream compression** to achieve high decompression speeds under DOS.

For the audio portion, the project relies on the Allegro 4.2 multimedia library, compatible with the Sound Blaster 16 sound card (emulated seamlessly in DOSBox, Nuked, etc.). 

## Audio Requirements

To ensure smooth playback without buffering issues or stuttering under MS-DOS, the external audio track (`.WAV`) must be pre-converted to:
* **Format:** PCM 8-bit unsigned mono (`pcm_u8`)
* **Sample Rate:** 11025 Hz (recommended for optimal performance)

You can easily convert any MP3, MP4, or WAV file using **FFmpeg**:
```bash
ffmpeg -i input.mp4 -ar 11025 -ac 1 -c:a pcm_u8 -fflags +bitexact -map_metadata -1 audio_dos.wav
```
## Workflow & Binary Generation

1. Prepare Frames: Export your source video as a series of PNG images (strictly 320x200 resolution) named in ascending order (e.g., frame_000001.png, frame_000002.png).

2. Compile the Generator: Compile main.c to generate the packaging utility.

3. Run the Generator: Execute the program providing your directory, output name, frame count, FPS, and max colors limit:
```bash
./main ./PNG video PNG 2853 15 256
```
Usage arguments: <DIRECTORY> <PREFIX> <EXTENSION> <FRAME_COUNT> <FPS> <MAX_COLORS>

4. Result: The tool automatically handles median-cut quantization, palette optimization, and dependent LZ4 stream compression, outputting a ready-to-use VIDEO.BIN file alongside your audio track.

## Components & Data Structure
# Movie File Format (.BIN)
```C
struct video_header {
    unsigned short frame_count;   // Total number of frames in the video (2 bytes)
    unsigned char  fps;           // Frames per second / Frame rate (1 byte)
    unsigned char  padding;       // Padding byte for 32-bit alignment (1 byte)
};

// The index table is located right after the global header.
// It contains 'frame_count' signed 4-byte integers (int32_t) representing 
// the compressed size (in bytes) of each frame's LZ4 data block.
typedef int32_t index_table_entry; // Size = frame_count * 4 bytes

struct palette_entry {
    unsigned char r; // Red component (0-255)
    unsigned char g; // Green component (0-255)
    unsigned char b; // Blue component (0-255)
};

struct frame_data_block {
    // DOS Fixed Palette Optimization: Systematically flushes a uniform 768-byte packet 
    // (256 colors * 3 bytes). Unused colors are padded with zeros (0,0,0).
    struct palette_entry palette[256]; 
    
    int size;                     // Size stored in the index table (LZ4 compressed size)
    unsigned char *data;          // Pixel index data (320x200 = 64,000 raw pixels) 
                                  // compressed via LZ4 in dependent stream mode (LZ4_compress_fast_continue)
};

struct video_binary {
    struct video_header header;
    int32_t *index_table;
    struct frame_data_block *frames;
};
```
# Verification Tool (verificator.c)

A verification utility is provided to test and debug generated .BIN packages.
* Analysis Mode: Checks structural integrity, index table mapping, and compression ratios.
```bash
./verificator video.bin
```

* Extraction Mode: Decompresses and extracts a specific frame directly to a PNG file for visual inspection.
```bash
./verificator video.bin 150 frame_150.png
```
