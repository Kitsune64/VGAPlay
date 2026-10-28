#!/bin/bash

# Vérifie si un fichier a bien été fourni en argument
if [ -z "$1" ]; then
    echo "Erreur : Veuillez spécifier un fichier à convertir."
    echo "Exemple : ./convert_dos.sh mon_audio.mp3"
    exit 1
fi

INPUT_FILE="$1"
# Extrait le nom du fichier sans l'extension
FILENAME=$(basename -- "$INPUT_FILE")
BASENAME="${FILENAME%.*}"

echo "Convert $INPUT_FILE to WAV audio for DOS..."

# Exécution de FFmpeg
ffmpeg -i "$INPUT_FILE" -ar 11025 -ac 1 -c:a pcm_u8 -fflags +bitexact -map_metadata -1 "${BASENAME}_dos.wav"

echo "Terminé !"
