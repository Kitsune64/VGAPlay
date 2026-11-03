/*
 * error.c
 *
 *  Distributed under the MIT License. See README.TXT for details.
 *
 *  Version sans Allegro. Le DMA audio est coupe par le atexit(audio_stop)
 *  enregistre dans audio_init() : exit() suffit donc ici.
 */

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dpmi.h>

void raise_error(const char *code){
    /* Retour en mode texte via le BIOS pour afficher l'erreur proprement */
    __dpmi_regs r;
    memset(&r, 0, sizeof(r));
    r.x.ax = 0x0003;
    __dpmi_int(0x10, &r);

    printf("FATAL ERROR: %s\n"
            "\n"
            "The program has terminated.\n", code);
    exit(-1);
}
