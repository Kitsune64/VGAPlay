# VGAPlay
A VGA video player and format converter for DOS systems.
### [Demonstration Video](https://www.youtube.com/watch?v=WeJYnQct9wI)

## Description
This is a Video Player compatible with DOS systems (MS-DOS, FreeDOS, DOSBox Emulator) written in C using DJGPP compiler with Allegro 4.2 library.

This Video Player is based on PNG files packed into a binary file and compressed with the LZ4 compression method. The LZ4 is a lossless data compression algorithm optimized for extremely fast compression and decompression speeds.

For the audio portion of this project, I'm using the Allegro 4.2 multimedia library, which is compatible with DOS. This video player is therefore compatible with the Sound Blaster 16 sound card, an older and very common model. This card can be easily emulated using DOSBox, Nuked, or any other emulator dedicated to Sound Blaster cards.

The sound or music (AUDIO.WAV file) must be encoded for now as a WAV file for now, idealy compressed into: 11025Hz, in 8 bit unsigned mono with the SBPLAY.EXE tool from Creative (Abandonware) for speed up decoding the audio file when playing the video.

ABPLAY25 that contain SBPLAY.EXE (player and converter) link here:
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
/* 
 * Video is stored in a series of LZ4 compressed 256-color bitmaps 
 * We update the palette every frame
 */

/* LZ4 compressed frame */
struct frame_data {
  unsigned char palette_size;   // palette size minus one, since 256 is the limit
  unsigned char *palette;       // 18-bit color palette
  int size;
  unsigned char *data;
};

struct video_data {
  unsigned short frame_count;   // total frame count of entire movie
  unsigned char frame_rate;     // frames to display per second
  struct frame_data *frames;    // compressed frame data
} video_binary;

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
