# VGAPlay
A VGA video player (fork of DOSVideoPlayer by Netdex) and format converter for DOS systems.
### [Demonstration Video (Workflow of the new version on Linux and Dosbox-X Emulator)](https://www.youtube.com/watch?v=j21uid9JJi8)

## Description
This is a Video Player compatible with DOS systems (MS-DOS, FreeDOS, DOSBox Emulator) written in C using DJGPP compiler with Allegro 4.2 library.

You can find a full set of DJGPP compiler environment for DOS with Allegro 4.2 AND Sound Blaster fix here:<br>
https://www.mrdictionary.net/allegro/

This Video Player is based on PNG files packed into a binary file and compressed with the LZ4 compression method. The LZ4 is a lossless data compression algorithm optimized for extremely fast compression and decompression speeds.

For the audio portion of this project, I'm using the Allegro 4.2 multimedia library, which is compatible with DOS. This video player is therefore compatible with the Sound Blaster 16 sound card, an older and very common model. This card can be easily emulated using DOSBox, Nuked, or any other emulator dedicated to Sound Blaster cards.

The sound or music (AUDIO.WAV file) must be encoded for now as a WAV file for now, idealy compressed into: 11025Hz, in 8 bit unsigned mono with the SBPLAY.EXE tool from Creative (Abandonware) for speed up decoding the audio file when playing the video.

ABPLAY25 that contain SBPLAY.EXE (player and converter) link here:<br>
https://archive.org/details/msdos_festival_ABPLAY25

## Workflow
1. Export raw video file as series of PNG files (must be 320x200) at a reasonable FPS (less than 70fps). Exported files must be in format prefix_000000.png in ascending order.

2. Modify parameters of conv/\_comp/create.bat
```
set LOCATION=[full path to folder storing PNG files]
set PREFIX=[prefix of all png files]
set EXTENSION=[extension (usually png)]
set COUNT=[number of frames]
set FPS=[frames per second]
```

3. Run create.bat, it will extract and compress the frame data

4. Copy the .lz4hc-64000 file to dos/bin and rename it accordingly

## Components
### Data Encoding
#### Movie File Format
``` c
struct video_header {
    unsigned short frame_count;   // Total number of frames in the video (2 bytes)
    unsigned char  fps;           // Frames per second / Frame rate (1 byte)
    unsigned char  padding;       // Padding byte for 32-bit alignment (1 byte)
};

// The index table is located right after the global header.
// It contains 'frame_count' signed 4-byte integers (int32_t) representing 
// the compressed size (in bytes) of each frame's LZ4 data.
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
                                  // compressed via LZ4 in dependent mode (LZ4_compress_fast_continue)
};

struct video_binary {
    struct video_header header;
    int32_t *index_table;
    struct frame_data_block *frames;
};
```

Tools for compressing video files are in conv/\_comp

FrameExtract.exe extracts frames out of a series of PNG files, and exports them to a large binary, its source is at conv/\_utilties/FramePack/FrameExtract

LZ4FramePack.exe compresses the frames with the LZ4 compression algorithm, its source is at conv/\_utilities/LZ4FramePack

#### Audio File Format
``` c
/* 
 * Audio is stored in 8-bit unsigned mono channel Microsoft header WAV data,
 * use a sample rate of < 48000Hz (11000Hz seems to run the best)
 *
 * The audio is played through PCM over the internal speaker, so the audio
 * quality is quite poor
 */

```
Audio is no longer packed with midi
