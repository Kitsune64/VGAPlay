#!/bin/sh

# FramePack Suite - Linux pipeline
# PNGExtract -> mpg2wav -> FramePack -> Verificator

# Get the directory where this script is located
SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)

PNG_EXTRACT_BIN="$SCRIPT_DIR/PNGExtract"
MPG2WAV_BIN="$SCRIPT_DIR/mpg2wav"
FRAMEPACK_BIN="$SCRIPT_DIR/FramePack"
VERIFICATOR_BIN="$SCRIPT_DIR/Verificator"
PNG_DIR="$SCRIPT_DIR/PNG"

echo "========================================="
echo "        FramePack Suite 1.4 Pipeline     "
echo "========================================="

# Check that all executables exist and are executable
for executable in \
    "$PNG_EXTRACT_BIN" \
    "$MPG2WAV_BIN" \
    "$FRAMEPACK_BIN" \
    "$VERIFICATOR_BIN"
do
    if [ ! -x "$executable" ]; then
        echo "Error: Missing or non-executable file: $executable"
        echo "Did you run 'make'?"
        exit 1
    fi
done

# 1. Ask for the video file
printf "Enter the video file name with its extension (ex: video.mp4): "
read -r mpg_file

if [ -z "$mpg_file" ]; then
    echo "Error: Filename cannot be empty."
    exit 1
fi

if [ ! -f "$mpg_file" ]; then
    echo "Error: Video file '$mpg_file' was not found."
    exit 1
fi

# 2. Ask for output prefix
while true; do
    printf "Enter output prefix (without extension, ex: VIDEO): "
    read -r VIDEO_PREFIX

    if [ -z "$VIDEO_PREFIX" ]; then
        echo "Error: Prefix cannot be empty."
        continue
    fi

    case "$VIDEO_PREFIX" in
        *[!a-zA-Z0-9_-]*)
            echo "Invalid prefix. Use only letters, numbers, '_' or '-'."
            ;;
        *)
            break
            ;;
    esac
done

# 3. Ask for FPS
while true; do
    printf "Enter FPS (1 to 60): "
    read -r fps

    case "$fps" in
        ''|*[!0-9]*)
            echo "Invalid input. Please enter a number."
            ;;
        *)
            if [ "$fps" -ge 1 ] && [ "$fps" -le 60 ]; then
                break
            fi
            echo "Invalid FPS. Please enter a value between 1 and 60."
            ;;
    esac
done

# 4. Ask for maximum colors
while true; do
    printf "Enter max colors (1 to 256): "
    read -r max_colors

    case "$max_colors" in
        ''|*[!0-9]*)
            echo "Invalid input. Please enter a number."
            ;;
        *)
            if [ "$max_colors" -ge 1 ] && [ "$max_colors" -le 256 ]; then
                break
            fi
            echo "Invalid input. Please enter a number between 1 and 256."
            ;;
    esac
done

# 5. Ask for padding mode
while true; do
    printf "Enter padding mode (0 for off, 1 for on): "
    read -r padding_mode

    if [ "$padding_mode" = "0" ] || [ "$padding_mode" = "1" ]; then
        break
    fi

    echo "Invalid input. Please enter exactly 0 or 1."
done

# Warning before erasing previous PNG files
echo
echo "Warning: PNGExtract will erase existing PNG files in:"
echo "$PNG_DIR"
printf "Do you want to continue? (y/N): "
read -r response

case "$response" in
    [yY]*) ;;
    *)
        echo "Aborted."
        exit 1
        ;;
esac

# --- STEP 1: Run PNGExtract ---
echo "-----------------------------------------"
echo "Running PNGExtract..."
echo "-----------------------------------------"

"$PNG_EXTRACT_BIN" "$mpg_file" "$PNG_DIR" "$fps" "$max_colors" "$padding_mode"

if [ $? -ne 0 ]; then
    echo "Error: PNGExtract failed."
    exit 1
fi

# --- STEP 2: Run mpg2wav ---
echo "-----------------------------------------"
echo "Running mpg2wav..."
echo "Output file: ${VIDEO_PREFIX}.WAV"
echo "-----------------------------------------"

"$MPG2WAV_BIN" "$mpg_file" "$VIDEO_PREFIX"

if [ $? -ne 0 ]; then
    echo "Error: mpg2wav failed."
    exit 1
fi

# --- STEP 3: Run FramePack ---
if [ ! -d "$PNG_DIR" ]; then
    echo "Error: PNG directory was not found: $PNG_DIR"
    exit 1
fi

echo "-----------------------------------------"
echo "Running FramePack..."
echo "FramePack will automatically detect all PNG files."
echo "Maximum colors per image: $max_colors"
echo "Output file: ${VIDEO_PREFIX}.BIN"
echo "-----------------------------------------"

# FramePack syntax:
# FramePack <DIRECTORY> <PREFIX> <FPS> <MAX_COLORS>

"$FRAMEPACK_BIN" "$PNG_DIR" "$VIDEO_PREFIX" "$fps" "$max_colors"

if [ $? -ne 0 ]; then
    echo "Error: FramePack failed."
    exit 1
fi

# --- STEP 4: Run Verificator ---
echo "-----------------------------------------"
echo "Running Verificator on ${VIDEO_PREFIX}.BIN..."
echo "-----------------------------------------"

"$VERIFICATOR_BIN" "${VIDEO_PREFIX}.BIN"

if [ $? -ne 0 ]; then
    echo "Error: Verificator failed."
    exit 1
fi

echo "========================================="
echo "Pipeline completed successfully!"
echo "========================================="
