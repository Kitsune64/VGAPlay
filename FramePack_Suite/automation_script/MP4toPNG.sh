#!/bin/bash
rm ./PNG/*.PNG

ffmpeg -i video.mp4 -vf "scale=300:140:force_original_aspect_ratio=decrease,pad=320:200:(ow-iw)/2:(oh-ih)/2:black" video_final.mp4
ffmpeg -i video_final.mp4 -vf "crop=in_w:in_h-4:0:4,fps=15,scale=320:200:flags=lanczos,format=rgb24,palettegen" -y palette.png && \
ffmpeg -i video_final.mp4 -i palette.png -filter_complex "crop=in_w:in_h-4:0:4,fps=15,scale=320:200:flags=lanczos,format=rgb24[x];[x][1:v]paletteuse=dither=floyd_steinberg" -pix_fmt pal8 -start_number 0 -y ./PNG/G_%06d.PNG
