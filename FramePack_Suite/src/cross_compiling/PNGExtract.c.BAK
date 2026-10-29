/*
  PL_MPEG - Extraction, Crop, FPS variable, Tramage Floyd-Steinberg et Padding (Hauteur fixe 160, ratio largeur, canvas 320x200)
  Licence : MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // Pour strcasecmp
#include <dirent.h>

#include "png.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
    #define strcasecmp _stricmp
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

// Résolution finale globale obligatoire
#define TARGET_WIDTH 320
#define TARGET_HEIGHT 200

// Hauteur fixe de la vidéo incrustée
#define VIDEO_TARGET_HEIGHT 160

// Nettoyage des anciens fichiers PNG dans le dossier cible
void clean_png_folder(const char *folder_path) {
    DIR *dir = opendir(folder_path);
    if (!dir) return;

    struct dirent *entry;
    char filepath[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        size_t len = strlen(entry->d_name);
        if (len > 4) {
            if (strcasecmp(entry->d_name + len - 4, ".png") == 0) {
                snprintf(filepath, sizeof(filepath), "%s/%s", folder_path, entry->d_name);
                remove(filepath);
            }
        }
    }
    closedir(dir);
}

// Mode 0 : Redimensionnement direct (plein écran 320x200)
void resize_image_direct(uint8_t *src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h) {
    for (int y = 0; y < dst_h; y++) {
        int src_y = (y * src_h) / dst_h;
        for (int x = 0; x < dst_w; x++) {
            int src_x = (x * src_w) / dst_w;
            
            int src_idx = (src_y * src_w + src_x) * 3;
            int dst_idx = (y * dst_w + x) * 3;
            
            dst[dst_idx]     = src[src_idx];     // R
            dst[dst_idx + 1] = src[src_idx + 1]; // G
            dst[dst_idx + 2] = src[src_idx + 2]; // B
        }
    }
}

// Mode 1 : Hauteur fixe à 160, largeur proportionnelle, centré sur le canvas 320x200
void resize_and_pad(uint8_t *src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h) {
    // 1. Remplissage complet du grand canvas 320x200 en noir (fond global)
    memset(dst, 0, dst_w * dst_h * 3);

    // 2. Calcul de l'échelle basée uniquement sur la hauteur (160 pixels) pour garder le ratio paysage
    float scale = (float)VIDEO_TARGET_HEIGHT / (float)src_h;

    int new_h = VIDEO_TARGET_HEIGHT;
    int new_w = (int)((float)src_w * scale + 0.5f);

    // Sécurisation si la largeur calculée dépasse le canvas 320
    if (new_w > dst_w) new_w = dst_w;

    // 3. Calcul des offsets pour centrer parfaitement la vidéo sur le canvas 320x200
    int offset_x = (dst_w - new_w) / 2;
    int offset_y = (dst_h - new_h) / 2; // (200 - 160) / 2 = 20 pixels de padding haut/bas

    for (int y = 0; y < new_h; y++) {
        int src_y = (y * src_h) / new_h;
        for (int x = 0; x < new_w; x++) {
            int src_x = (x * src_w) / new_w;

            int src_idx = (src_y * src_w + src_x) * 3;
            int dst_idx = ((y + offset_y) * dst_w + (x + offset_x)) * 3;

            dst[dst_idx]     = src[src_idx];     // R
            dst[dst_idx + 1] = src[src_idx + 1]; // G
            dst[dst_idx + 2] = src[src_idx + 2]; // B
        }
    }
}

// Quantification et tramage Floyd-Steinberg haute qualité (Palette 8-bits)
void quantize_floyd_steinberg(uint8_t *rgb_pixels, int width, int height, int max_colors, 
                              uint8_t *indexed_pixels, png_colorp palette, int *palette_size) {
    
    int r_levels = 8, g_levels = 8, b_levels = 4;
    if (max_colors <= 16) { r_levels = 2; g_levels = 2; b_levels = 2; }
    else if (max_colors <= 64) { r_levels = 4; g_levels = 4; b_levels = 4; }

    int p_idx = 0;
    for (int r = 0; r < r_levels; r++) {
        for (int g = 0; g < g_levels; g++) {
            for (int b = 0; b < b_levels; b++) {
                if (p_idx >= max_colors) break;
                palette[p_idx].red   = (r * 255) / (r_levels - 1);
                palette[p_idx].green = (g * 255) / (g_levels - 1);
                palette[p_idx].blue  = (b * 255) / (b_levels - 1);
                p_idx++;
            }
        }
    }
    *palette_size = p_idx;

    int *error_buf = (int *)malloc(width * height * 3 * sizeof(int));
    for (int i = 0; i < width * height * 3; i++) {
        error_buf[i] = rgb_pixels[i];
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            int closest_color_idx = 0;
            int min_distance = 9999999;

            int r_val = error_buf[idx];     if (r_val < 0) r_val = 0; if (r_val > 255) r_val = 255;
            int g_val = error_buf[idx + 1]; if (g_val < 0) g_val = 0; if (g_val > 255) g_val = 255;
            int b_val = error_buf[idx + 2]; if (b_val < 0) b_val = 0; if (b_val > 255) b_val = 255;

            for (int i = 0; i < *palette_size; i++) {
                int dr = r_val - palette[i].red;
                int dg = g_val - palette[i].green;
                int db = b_val - palette[i].blue;
                int distance = dr*dr + dg*dg + db*db;
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_color_idx = i;
                }
            }

            indexed_pixels[y * width + x] = closest_color_idx;

            int err_r = r_val - palette[closest_color_idx].red;
            int err_g = g_val - palette[closest_color_idx].green;
            int err_b = b_val - palette[closest_color_idx].blue;

            if (x + 1 < width) {
                error_buf[idx + 3]     += (err_r * 7) / 16;
                error_buf[idx + 4]     += (err_g * 7) / 16;
                error_buf[idx + 5]     += (err_b * 7) / 16;
            }
            if (y + 1 < height) {
                if (x - 1 >= 0) {
                    int n_idx = ((y + 1) * width + (x - 1)) * 3;
                    error_buf[n_idx]     += (err_r * 3) / 16;
                    error_buf[n_idx + 1] += (err_g * 3) / 16;
                    error_buf[n_idx + 2] += (err_b * 3) / 16;
                }
                int n_idx = ((y + 1) * width + x) * 3;
                error_buf[n_idx]     += (err_r * 5) / 16;
                error_buf[n_idx + 1] += (err_g * 5) / 16;
                error_buf[n_idx + 2] += (err_b * 5) / 16;
                if (x + 1 < width) {
                    int n_idx_p = ((y + 1) * width + (x + 1)) * 3;
                    error_buf[n_idx_p]     += (err_r * 1) / 16;
                    error_buf[n_idx_p + 1] += (err_g * 1) / 16;
                    error_buf[n_idx_p + 2] += (err_b * 1) / 16;
                }
            }
        }
    }
    free(error_buf);
}

// Écriture du fichier PNG indexé 8-bits
int write_png_indexed(const char *path, int width, int height, uint8_t *indexed_pixels, png_colorp palette, int palette_size) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { fclose(fp); return 0; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_write_struct(&png_ptr, NULL); fclose(fp); return 0; }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return 0;
    }

    png_init_io(png_ptr, fp);
    
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_set_PLTE(png_ptr, info_ptr, palette, palette_size);
    png_write_info(png_ptr, info_ptr);

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = indexed_pixels + y * width;
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);

    free(row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: PNGExtract <file.mpg> [fps] [max_colors] [padding_mode (0/1)]\n");
        printf("Exemple: PNGExtract sortie.mpg 15 256 1\n");
        return 1;
    }

    double target_fps = (argc >= 3) ? atof(argv[2]) : 15.0;
    int max_colors = (argc >= 4) ? atoi(argv[3]) : 256;
    int padding_mode = (argc >= 5) ? atoi(argv[4]) : 1; 

    if (target_fps <= 0) target_fps = 15.0;
    if (max_colors < 2) max_colors = 256;
    if (max_colors > 256) max_colors = 256;

    MKDIR("PNG");
    clean_png_folder("PNG");

    plm_t *plm = plm_create_with_filename(argv[1]);
    if (!plm) {
        printf("Impossible d'ouvrir le fichier %s\n", argv[1]);
        return 1;
    }

    plm_set_audio_enabled(plm, FALSE);
    
    int w = plm_get_width(plm);
    int h = plm_get_height(plm);
    double total_duration = plm_get_duration(plm);
    
    uint8_t *raw_pixels = (uint8_t *)malloc(w * h * 3);

    double next_sample_time = 0.0;
    int file_index = 0;
    plm_frame_t *frame = NULL;

    png_color palette[256];
    int palette_size = 0;

    // Le canvas final fait TOUJOURS exactement 320x200
    uint8_t *canvas_pixels = (uint8_t *)malloc(TARGET_WIDTH * TARGET_HEIGHT * 3);
    uint8_t *indexed_pixels = (uint8_t *)malloc(TARGET_WIDTH * TARGET_HEIGHT * 1);

    printf("Extraction en cours (FPS: %.1f, Couleurs: %d, Padding: %d)...\n", target_fps, max_colors, padding_mode);

    while ((frame = plm_decode_video(plm)) != NULL) {
        if (file_index > 0 && frame->time < next_sample_time) {
            continue;
        }
        next_sample_time = frame->time + (1.0 / target_fps);

        plm_frame_to_rgb(frame, raw_pixels, w * 3);

        // Crop de 4 pixels en haut
        int crop_top = 4;
        int new_w = w;
        int new_h = h - crop_top;
        
        uint8_t *cropped_pixels = (uint8_t *)malloc(new_w * new_h * 3);
        for (int y = 0; y < new_h; y++) {
            memcpy(cropped_pixels + y * new_w * 3, raw_pixels + (y + crop_top) * w * 3, new_w * 3);
        }

        // Redimensionnement selon le mode choisi
        if (padding_mode == 1) {
            resize_and_pad(cropped_pixels, new_w, new_h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
        } else {
            resize_image_direct(cropped_pixels, new_w, new_h, canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT);
        }

        // Quantification et tramage Floyd-Steinberg haute qualité
        quantize_floyd_steinberg(canvas_pixels, TARGET_WIDTH, TARGET_HEIGHT, max_colors, indexed_pixels, palette, &palette_size);

        char png_name[64];
        sprintf(png_name, "PNG/G_%06d.PNG", file_index);

        write_png_indexed(png_name, TARGET_WIDTH, TARGET_HEIGHT, indexed_pixels, palette, palette_size);

        // Calcul du pourcentage d'avancement propre sur une seule ligne
        if (total_duration > 0.0) {
            int percent = (int)((frame->time / total_duration) * 100.0);
            if (percent > 100) percent = 100;
            printf("\r[Extraction] Frame: %d | Progress: %3d%% (%.1fs / %.1fs)", file_index, percent, frame->time, total_duration);
        } else {
            printf("\r[Extraction] Frame: %d processed...", file_index);
        }
        fflush(stdout);

        free(cropped_pixels);
        file_index++;
    }
    
    printf("\nExtraction terminee avec succes ! Total: %d frames generees.\n", file_index);

    free(indexed_pixels);
    free(canvas_pixels);
    free(raw_pixels);
    plm_destroy(plm);
    return 0;
}
