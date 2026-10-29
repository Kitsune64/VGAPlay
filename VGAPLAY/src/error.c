/*
 * error.c
 *
 *  Distributed under the MIT License. See README.TXT for details.
 */

#include "error.h"
#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>

void raise_error(const char *code){
    // Remet l'affichage d'Allegro en mode texte pour 
    // pouvoir afficher l'erreur proprement dans la console DOS
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    
    printf("FATAL ERROR: %s\n"
            "\n"
            "The program has terminated.\n", code);
    exit(-1);
}
