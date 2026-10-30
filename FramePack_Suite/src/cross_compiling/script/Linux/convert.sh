#!/bin/sh

# Get the directory where this script is located
SCRIPT_DIR=$(dirname "$0")
PNG_EXTRACT_BIN="$SCRIPT_DIR/PNGExtract"
MPG2WAV_BIN="$SCRIPT_DIR/mpg2wav"
FRAMEPACK_BIN="$SCRIPT_DIR/FramePack"
VERIFICATOR_BIN="$SCRIPT_DIR/Verificator"

echo "========================================="
echo "        FramePack Suite Pipeline         "
echo "========================================="

# Check if all executables exist in the same directory
if [ ! -f "$PNG_EXTRACT_BIN" ]; then
    echo "Error: The executable PNGExtract was not found in this directory. Did you run 'make'?"
    exit 1
fi
if [ ! -f "$MPG2WAV_BIN" ]; then
    echo "Error: The executable mpg2wav was not found in this directory. Did you run 'make'?"
    exit 1
fi
if [ ! -f "$FRAMEPACK_BIN" ]; then
    echo "Error: The executable FramePack was not found in this directory. Did you run 'make'?"
    exit 1
fi
if [ ! -f "$VERIFICATOR_BIN" ]; then
    echo "Error: The executable Verificator was not found in this directory. Did you run 'make'?"
    exit 1
fi

# 1. Ask for the MPG file
printf "Enter the video file name with its extension (ex: video.mp4): "
read -r mpg_file

if [ -z "$mpg_file" ]; then
    echo "Error: Filename cannot be empty."
    exit 1
fi

# Check if the file exists
if [ ! -f "$mpg_file" ]; then
    echo "Warning: The file '$mpg_file' does not exist."
    printf "Do you want to continue anyway? (y/N): "
    read -r response
    case "$response" in
        [yY]*) ;;
        *) echo "Aborted."; exit 1 ;;
    esac
fi

# 2. Ask for FPS
printf "Enter FPS (e.g., 15, 24, 30): "
read -r fps

if [ -z "$fps" ]; then
    echo "Error: FPS cannot be empty."
    exit 1
fi

# 3. Ask for Max Colors (with validation: max 256)
while true; do
    printf "Enter max colors (1 to 256): "
    read -r max_colors
    if [ "$max_colors" -eq "$max_colors" ] 2>/dev/null && [ "$max_colors" -ge 1 ] && [ "$max_colors" -le 256 ]; then
        break
    fi
    echo "Invalid input. Please enter a number between 1 and 256."
done

# 4. Ask for Padding Mode (0 or 1)
while true; do
    printf "Enter padding mode (0 for off, 1 for on): "
    read -r padding_mode
    if [ "$padding_mode" = "0" ] || [ "$padding_mode" = "1" ]; then
        break
    fi
    echo "Invalid input. Please enter exactly 0 or 1."
done

# Warning before erasing previous files
echo "Warning: PNGExtract will erase your current PNG files in the PNG directory."
printf "Do you want to continue anyway? (y/N): "
read -r response
case "$response" in
    [yY]*) ;;
    *) echo "Aborted."; exit 1 ;;
esac

# --- STEP 1: Run PNGExtract ---
echo "-----------------------------------------"
echo "Running PNGExtract..."
echo "-----------------------------------------"
"$PNG_EXTRACT_BIN" "$mpg_file" "PNG" "$fps" "$max_colors" "$padding_mode"

# --- STEP 2: Run mpg2wav ---
echo "-----------------------------------------"
echo "Running mpg2wav (Generating AUDIO.WAV)..."
echo "-----------------------------------------"
"$MPG2WAV_BIN" "$mpg_file"

# --- STEP 3: Auto-count PNGs and run FramePack ---
PNG_DIR="$SCRIPT_DIR/PNG"

if [ ! -d "$PNG_DIR" ]; then
    echo "Error: The PNG directory '$PNG_DIR' was not found after extraction."
    exit 1
fi

# Count PNG files in the directory (handling uppercase .PNG extension)
frame_count=$(ls -1 "$PNG_DIR"/*.PNG 2>/dev/null | wc -l)

if [ "$frame_count" -eq 0 ]; then
    echo "Error: No PNG files found in $PNG_DIR."
    exit 1
fi

echo "-----------------------------------------"
echo "Detected $frame_count frames in $PNG_DIR"
echo "Running FramePack..."
echo "-----------------------------------------"
"$FRAMEPACK_BIN" "$PNG_DIR" "VIDEO" "PNG" "$frame_count" "$fps" "$max_colors"

# --- STEP 4: Run Verificator ---
echo "-----------------------------------------"
echo "Running Verificator on VIDEO.BIN..."
echo "-----------------------------------------"
"$VERIFICATOR_BIN" "VIDEO.BIN"

echo "========================================="
echo "Pipeline completed successfully from start to end!"
echo "========================================="
