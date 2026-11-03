#ifndef AUDIO_H
#define AUDIO_H

#define AUDIO_NONE 0
#define AUDIO_SB   1  /* Sound Blaster 16 natif (DMA 16 bits) */
#define AUDIO_AC97 2  /* AC'97 natif (Intel ICH, PCI bus mastering) */

/* Pilote detecte par audio_init() (ne change plus apres audio_stop) */
extern int audio_driver;

/* Interface du pilote audio unifie. Allegro n'est plus utilise. */

/* Detecte le materiel et l'initialise. Retourne AUDIO_xxx.
   Enregistre aussi audio_stop() via atexit(). */
int  audio_init(void);

/* Ouvre le WAV, prepare la lecture (materiel ou horloge de secours si muet).
   total_samples = nombre de frames audio (echantillons par canal).
   Retourne 0 uniquement si le WAV est illisible. */
int  audio_load_wav(const char *filename, unsigned int *total_samples);

void audio_play(void);

/* Position de lecture en frames audio, interpolee en continu.
   Retourne -1 quand la piste est terminee. */
int  audio_get_position(void);

/* A appeler le plus souvent possible (recharge les buffers). */
void audio_update(void);

/* Arrete la lecture et libere les ressources. Peut etre appelee plusieurs fois. */
void audio_stop(void);

#endif
