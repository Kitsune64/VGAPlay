#!/bin/bash

# 1. Vérification de la présence de FFmpeg sur le système
if ! command -v ffmpeg &> /dev/null; then
    echo "❌ Erreur : FFmpeg n'est pas installé ou n'est pas dans le PATH."
    echo "Veuillez l'installer (ex: 'sudo apt install ffmpeg' sur Debian/Ubuntu) pour utiliser ce script."
    exit 1
fi

# 2. Demande interactive du fichier d'entrée
echo "=================================================="
# Traduction : "Veuillez entrer le nom ou le chemin du fichier vidéo :"
echo "Please enter the input video file name or path:"
echo "=================================================="
read -r input_file

# 3. Vérification de l'existence du fichier
if [ ! -f "$input_file" ]; then
    echo "❌ Erreur : Le fichier '$input_file' n'existe pas."
    exit 1
fi

# 4. Définition automatique du fichier de sortie (video.mpg)
output_file="video.mpg"

# 5. Exécution de la commande FFmpeg optimisée
echo "⏳ Processing conversion: '$input_file' ➡️ '$output_file'..."
ffmpeg -i "$input_file" -c:v mpeg1video -q:v 3 -pix_fmt yuv420p -color_range pc -c:a mp2 -ar 44100 -ac 1 "$output_file" -y

# 6. Message de succès
if [ $? -eq 0 ]; then
    echo "✅ Conversion completed successfully! Output saved as '$output_file'."
else
    echo "❌ An error occurred during conversion."
fi
