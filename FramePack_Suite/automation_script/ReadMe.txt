========================================================================
                           FRAMEPACK SUITE
                          Automation Scripts
========================================================================

FramePack Suite is a collection of automation scripts designed to 
convert videos and audio files into formats compatible with VGAPLAY and 
MS-DOS environments.

------------------------------------------------------------------------
PREREQUISITES (FFMPEG)
------------------------------------------------------------------------
All scripts in this suite rely heavily on FFmpeg for media processing.
You must ensure FFmpeg is installed and available in your system PATH.

- Linux (Ubuntu/Debian): 
  sudo apt update && sudo apt install ffmpeg
- Windows: 
  Download the official FFmpeg builds and add its 'bin' folder to your 
  system environment variables (PATH).

------------------------------------------------------------------------
1. VIDEO CONVERSION (MP4 to PNG Sequence)
------------------------------------------------------------------------
Scripts: MP4toPNG.sh (Linux) / MP4toPNG.bat (Windows)

Description:
- Cleans up and ensures the 'PNG' directory exists.
- Rescales and pads the source video to a 320x200 resolution.
- Generates an optimized color palette using Floyd-Steinberg dithering 
  at 15 frames per second.
- Exports the resulting frames as indexed PNG sequence files 
  (e.g., G_000000.PNG).

Usage:
- Place your source video named 'video.mp4' in the script directory.
- Linux: ./MP4toPNG.sh
- Windows: Run MP4toPNG.bat

------------------------------------------------------------------------
2. AUDIO CONVERSION (To DOS-compatible WAV)
------------------------------------------------------------------------
Scripts: File2DOSWAV.sh (Linux) / File2DOSWAV.bat (Windows)

Description:
- Converts input audio files into an 8-bit unsigned mono WAV format 
  sampled at 11025 Hz (-ar 11025 -ac 1 -c:a pcm_u8), fully optimized for 
  DOS audio playback.

Usage:
- Linux: ./File2DOSWAV.sh <input_audio_file>
- Windows: Run File2DOSWAV.bat and enter the filename when prompted.

========================================================================
