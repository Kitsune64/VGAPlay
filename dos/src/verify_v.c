#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "decoder.h"
#include "fileio.h"
#include "error.h"
#include "lz4.h"

int main(int argc, char *argv[]) {
    // Verifier si le chemin de la video a bien ete fourni en argument
    if (argc < 2) {
        printf("Utilisation : VERIFY.EXE <chemin_vers_video>\n");
        printf("Exemple : VERIFY.EXE ..\\video.bin\n");
        return 1;
    }

    const char *video_path = argv[1];
    FILE *file = fopen(video_path, "rb");
    if (!file) {
        printf("Erreur : Impossible d'ouvrir le fichier %s\n", video_path);
        return 1;
    }

    // Ouverture du fichier de rapport en mode ecriture
    FILE *report = fopen("verify.log", "w");
    if (!report) {
        printf("Avertissement : Impossible de creer le fichier verify.log\n");
    }

    // 1. Lecture de l'en-tete global de la video
    struct video_header vh = decode_video_header(file);
    
    printf("=== RAPPORT DE VERIFICATION : %s ===\n", video_path);
    printf("Nombre de frames annonce : %d\n", vh.frame_count);
    printf("Framerate : %d fps\n\n", vh.frame_rate);

    if (report) {
        fprintf(report, "=== RAPPORT DE VERIFICATION : %s ===\n", video_path);
        fprintf(report, "Nombre de frames annonce : %d\n", vh.frame_count);
        fprintf(report, "Framerate : %d fps\n\n", vh.frame_rate);
    }

    decode_video_reset();

    // Initialisation du flux de decompression LZ4
    LZ4_streamDecode_t *lz4StreamDecode = LZ4_createStreamDecode();
    if (!lz4StreamDecode) {
        printf("Erreur : Impossible d'initialiser le flux LZ4.\n");
        if (report) {
            fprintf(report, "Erreur : Impossible d'initialiser le flux LZ4.\n");
            fclose(report);
        }
        fclose(file);
        return 1;
    }

    int errors_found = 0;
    int stride_anomalies = 0;

    // 2. Boucle d'analyse trame par trame
    for (int frame = 0; frame < vh.frame_count; frame++) {
        int cmpBytes = 0;
        byte read_pal_size = 0;

        // Lecture de la taille de la palette
        if (fread(&read_pal_size, 1, 1, file) != 1) {
            printf("[Frame %d] Erreur : Fin de fichier prematuree (palette size).\n", frame);
            if (report) fprintf(report, "[Frame %d] Erreur : Fin de fichier prematuree (palette size).\n", frame);
            errors_found++;
            break;
        }
        read_pal_size++;
        
        // Sauter les donnees de la palette dans le fichier
        if (fseek(file, 3 * read_pal_size, SEEK_CUR) != 0) {
            printf("[Frame %d] Erreur : Impossible de lire la palette.\n", frame);
            if (report) fprintf(report, "[Frame %d] Erreur : Impossible de lire la palette.\n", frame);
            errors_found++;
            break;
        }

        // Lecture de la taille du bloc compresse (cmpBytes)
        if (fread(&cmpBytes, sizeof(int), 1, file) != 1) {
            printf("[Frame %d] Erreur : Fin de fichier prematuree (cmpBytes).\n", frame);
            if (report) fprintf(report, "[Frame %d] Erreur : Fin de fichier prematuree (cmpBytes).\n", frame);
            errors_found++;
            break;
        }

        if (cmpBytes <= 0 || cmpBytes > BLOCK_BYTES) {
            printf("[Frame %d] Avertissement : cmpBytes suspect (%d)\n", frame, cmpBytes);
            if (report) fprintf(report, "[Frame %d] Avertissement : cmpBytes suspect (%d)\n", frame, cmpBytes);
        }

        // Lecture du bloc compresse brut
        byte *temp_cmpBuf = malloc(cmpBytes);
        if (!temp_cmpBuf) {
            printf("[Frame %d] Erreur : Allocation memoire impossible.\n", frame);
            if (report) fprintf(report, "[Frame %d] Erreur : Allocation memoire impossible.\n", frame);
            errors_found++;
            break;
        }

        if (fread(temp_cmpBuf, 1, cmpBytes, file) != (size_t)cmpBytes) {
            printf("[Frame %d] Erreur : Impossible de lire le bloc compresse de %d octets.\n", frame, cmpBytes);
            if (report) fprintf(report, "[Frame %d] Erreur : Impossible de lire le bloc compresse de %d octets.\n", frame, cmpBytes);
            free(temp_cmpBuf);
            errors_found++;
            break;
        }

        // Test de decompression LZ4
        char *temp_decBuf = malloc(BLOCK_BYTES);
        if (!temp_decBuf) {
            free(temp_cmpBuf);
            break;
        }

        const int decBytes = LZ4_decompress_safe_continue(
            lz4StreamDecode, (char*)temp_cmpBuf, temp_decBuf, cmpBytes, BLOCK_BYTES
        );

        // Seules les erreurs ou les valeurs differentes de 64000 s'affichent
        if (decBytes <= 0) {
            printf("[Frame %d] ERREUR DECOMPRESSION LZ4 (decBytes = %d)\n", frame, decBytes);
            if (report) fprintf(report, "[Frame %d] ERREUR DECOMPRESSION LZ4 (decBytes = %d)\n", frame, decBytes);
            errors_found++;
        } else if (decBytes != 64000) {
            printf("[Frame %d] Info : decBytes = %d (Presence de padding/stride detectee)\n", frame, decBytes);
            if (report) fprintf(report, "[Frame %d] Info : decBytes = %d (Presence de padding/stride detectee)\n", frame, decBytes);
            stride_anomalies++;
        }

        free(temp_cmpBuf);
        free(temp_decBuf);
    }

    // Liberation du flux LZ4
    free(lz4StreamDecode);
    fclose(file);
    
    printf("\n=== FIN DE L'ANALYSE ===\n");
    printf("Total erreurs de decompression : %d\n", errors_found);
    printf("Total frames avec stride/padding (!= 64000 octets) : %d\n", stride_anomalies);

    if (report) {
        fprintf(report, "\n=== FIN DE L'ANALYSE ===\n");
        fprintf(report, "Total erreurs de decompression : %d\n", errors_found);
        fprintf(report, "Total frames avec stride/padding (!= 64000 octets) : %d\n", stride_anomalies);
        fclose(report);
        printf("\nRapport sauvegarde dans : verify.log\n");
    }

    return 0;
}
