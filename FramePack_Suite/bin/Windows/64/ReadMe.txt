================================================================================
FRAMEPACK SUITE
================================================================================

OVERVIEW
--------
FramePack Suite is a collection of 4 core command-line tools designed for 
video processing, frame extraction, binary packaging, and validation. 

All programs in this suite are fully compatible with both Linux and Windows 
(32-bit and 64-bit environments) and are distributed under the open-source 
MIT License.

--------------------------------------------------------------------------------
PROGRAM COMPONENT DESCRIPTIONS
--------------------------------------------------------------------------------

1. FramePack (`FramePack.c` / `FramePack`)
   - Purpose: Scans a directory of ordered PNG image frames, performs color 
     quantization via Median Cut, applies an optimized DOS palette mapping, 
     compresses the indexed frames using LZ4 compression, and packs them into 
     a high-performance custom binary format (`.BIN`).
   - Usage:
     FramePack <DIRECTORY> <PREFIX> <EXTENSION> <FRAME_COUNT> <FPS> <MAX_COLORS>
   - Example:
     FramePack ./PNG video PNG 2853 15 256

2. mpg2wav (`mpg2wav.c` / `mpg2wav`)
   - Purpose: Extracts the audio track from an MPEG-PS (`.mpg`) file, 
     downsamples it to a target sample rate (11025 Hz optimized for Sound 
     Blaster/DOS hardware), converts stereo channels into mono, and exports 
     it as a standard uncompressed PCM `AUDIO.WAV` file.
   - Usage:
     mpg2wav <video.mpg>
   - Example:
     mpg2wav video.mpg

3. PNGExtract (`PNGExtract.c` / `PNGExtract`)
   - Purpose: Decodes a video file, extracts individual frames into a 
     designated `PNG` folder with variable framerate support, applies top cropping 
     and aspect ratio padding (320x200 target resolution), and executes 
     high-quality Floyd-Steinberg dithering with an 8-bit indexed palette.
   - Usage:
     PNGExtract <file.mpg> [fps] [max_colors] [padding_mode (0/1)]
   - Example:
     PNGExtract video.mpg 15 256 1

4. verificator (`verificator.c` / `verificator`)
   - Purpose: Validates the integrity of the generated `.BIN` package structures 
     created by FramePack. It checks file headers, reads the continuous index 
     table, decompresses the LZ4 stream, computes compression efficiency ratios, 
     and allows selective single-frame export to a PNG image file.
   - Usage:
     Mode 1 (Analysis) : verificator <video.bin>
     Mode 2 (Extract)  : verificator <video.bin> <frame_index> <output.png>
   - Example:
     verificator video.bin 150 extracted_frame.png

--------------------------------------------------------------------------------
LICENSE
--------------------------------------------------------------------------------
All programs within the FramePack Suite are released under the MIT License. 
You are free to use, modify, and distribute them as needed.
================================================================================
