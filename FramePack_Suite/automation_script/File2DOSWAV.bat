@echo off

set /p FICHIER=Enter the filename with extension (ex: music.mp3): 

if not exist "%FICHIER%" (
    echo.
    echo ERROR: The file "%FICHIER%" was not found in this folder.
    echo.
    pause
    exit
)

echo.
echo Converting %FICHIER% to DOS WAV format...
ffmpeg -i "%FICHIER%" -ar 11025 -ac 1 -c:a pcm_u8 -fflags +bitexact -map_metadata -1 "audio_dos.wav"

echo.
echo Conversion complete!
pause