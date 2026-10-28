@echo off

:: Crée le dossier PNG s'il n'existe pas
if not exist PNG mkdir PNG

:: Supprime les anciens fichiers PNG du dossier[cite: 1]
del /Q "PNG\*.PNG"

:: Redimensionne et cadre la vidéo initiale[cite: 1]
ffmpeg -i video.mp4 -vf "scale=300:140:force_original_aspect_ratio=decrease,pad=320:200:(ow-iw)/2:(oh-ih)/2:black" video_final.mp4

:: Génère la palette de couleurs[cite: 1]
ffmpeg -i video_final.mp4 -vf "crop=in_w:in_h-4:0:4,fps=15,scale=320:200:flags=lanczos,format=rgb24,palettegen" -y palette.png

:: Applique la palette et génère les images PNG (notez le double %% pour G_%%06d.PNG)[cite: 1]
ffmpeg -i video_final.mp4 -i palette.png -filter_complex "crop=in_w:in_h-4:0:4,fps=15,scale=320:200:flags=lanczos,format=rgb24[x];[x][1:v]paletteuse=dither=floyd_steinberg" -pix_fmt pal8 -start_number 0 -y "PNG\G_%%06d.PNG"
