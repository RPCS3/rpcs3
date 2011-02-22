
/* Program to load a wave file and loop playing it using SDL sound */

/* loopwaves.c is much more robust in handling WAVE files -- 
	This is only for simple WAVEs
*/
#include "SDL_config.h"

#include <stdio.h>
#include <stdlib.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "SDL.h"
#include "SDL_audio.h"

struct
{
    SDL_AudioSpec spec;
    Uint8 *sound;               /* Pointer to wave data */
    Uint32 soundlen;            /* Length of wave data */
    int soundpos;               /* Current play position */
} wave;


/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}


void SDLCALL
fillerup(void *unused, Uint8 * stream, int len)
{
    Uint8 *waveptr;
    int waveleft;

    /* Set up the pointers */
    waveptr = wave.sound + wave.soundpos;
    waveleft = wave.soundlen - wave.soundpos;

    /* Go! */
    while (waveleft <= len) {
        SDL_memcpy(stream, waveptr, waveleft);
        stream += waveleft;
        len -= waveleft;
        waveptr = wave.sound;
        waveleft = wave.soundlen;
        wave.soundpos = 0;
    }
    SDL_memcpy(stream, waveptr, len);
    wave.soundpos += len;
}

static int done = 0;
void
poked(int sig)
{
    done = 1;
}

int
main(int argc, char *argv[])
{
    /* Load the SDL library */
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    if (argv[1] == NULL) {
        argv[1] = "sample.wav";
    }
    /* Load the wave file into memory */
    if (SDL_LoadWAV(argv[1], &wave.spec, &wave.sound, &wave.soundlen) == NULL) {
        fprintf(stderr, "Couldn't load %s: %s\n", argv[1], SDL_GetError());
        quit(1);
    }

    wave.spec.callback = fillerup;
#if HAVE_SIGNAL_H
    /* Set the signals */
#ifdef SIGHUP
    signal(SIGHUP, poked);
#endif
    signal(SIGINT, poked);
#ifdef SIGQUIT
    signal(SIGQUIT, poked);
#endif
    signal(SIGTERM, poked);
#endif /* HAVE_SIGNAL_H */

    /* Initialize fillerup() variables */
    if (SDL_OpenAudio(&wave.spec, NULL) < 0) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        SDL_FreeWAV(wave.sound);
        quit(2);
    }

    printf("Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

    /* Let the audio run */
    SDL_PauseAudio(0);
    while (!done && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING))
        SDL_Delay(1000);

    /* Clean up on signal */
    SDL_CloseAudio();
    SDL_FreeWAV(wave.sound);
    SDL_Quit();
    return (0);
}
