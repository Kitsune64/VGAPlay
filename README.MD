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

    Prepare Frames: Export your source video as a series of PNG images (strictly 320x200 resolution) named in ascending order (e.g., frame_000001.png, frame_000002.png).

    Compile the Generator: Compile main.c to generate the packaging utility.

    Run the Generator: Execute the program providing your directory, output name, frame count, FPS, and max colors limit:
