#include "SDL.h"

static SDL_AudioSpec spec;
static Uint8 *sound = NULL;     /* Pointer to wave data */
static Uint32 soundlen = 0;     /* Length of wave data */

typedef struct
{
    SDL_AudioDeviceID dev;
    int soundpos;
    volatile int done;
} callback_data;

void SDLCALL
play_through_once(void *arg, Uint8 * stream, int len)
{
    callback_data *cbd = (callback_data *) arg;
    Uint8 *waveptr = sound + cbd->soundpos;
    int waveleft = soundlen - cbd->soundpos;
    int cpy = len;
    if (cpy > waveleft)
        cpy = waveleft;

    SDL_memcpy(stream, waveptr, cpy);
    len -= cpy;
    cbd->soundpos += cpy;
    if (len > 0) {
        stream += cpy;
        SDL_memset(stream, spec.silence, len);
        cbd->done++;
    }
}

static void
test_multi_audio(int devcount)
{
    callback_data cbd[64];
    int keep_going = 1;
    int i;

    if (devcount > 64) {
        fprintf(stderr, "Too many devices (%d), clamping to 64...\n",
                devcount);
        devcount = 64;
    }

    spec.callback = play_through_once;

    for (i = 0; i < devcount; i++) {
        const char *devname = SDL_GetAudioDeviceName(i, 0);
        printf("playing on device #%d: ('%s')...", i, devname);
        fflush(stdout);

        SDL_memset(&cbd[0], '\0', sizeof(callback_data));
        spec.userdata = &cbd[0];
        cbd[0].dev = SDL_OpenAudioDevice(devname, 0, &spec, NULL, 0);
        if (cbd[0].dev == 0) {
            printf("\nOpen device failed: %s\n", SDL_GetError());
        } else {
            SDL_PauseAudioDevice(cbd[0].dev, 0);
            while (!cbd[0].done)
                SDL_Delay(100);
            SDL_PauseAudioDevice(cbd[0].dev, 1);
            printf("done.\n");
            SDL_CloseAudioDevice(cbd[0].dev);
        }
    }

    SDL_memset(cbd, '\0', sizeof(cbd));

    printf("playing on all devices...\n");
    for (i = 0; i < devcount; i++) {
        const char *devname = SDL_GetAudioDeviceName(i, 0);
        spec.userdata = &cbd[i];
        cbd[i].dev = SDL_OpenAudioDevice(devname, 0, &spec, NULL, 0);
        if (cbd[i].dev == 0) {
            printf("Open device %d failed: %s\n", i, SDL_GetError());
        }
    }

    for (i = 0; i < devcount; i++) {
        if (cbd[i].dev) {
            SDL_PauseAudioDevice(cbd[i].dev, 0);
        }
    }

    while (keep_going) {
        keep_going = 0;
        for (i = 0; i < devcount; i++) {
            if ((cbd[i].dev) && (!cbd[i].done)) {
                keep_going = 1;
            }
        }
        SDL_Delay(100);
    }

    for (i = 0; i < devcount; i++) {
        if (cbd[i].dev) {
            SDL_PauseAudioDevice(cbd[i].dev, 1);
            SDL_CloseAudioDevice(cbd[i].dev);
        }
    }

    printf("All done!\n");
}


int
main(int argc, char **argv)
{
    int devcount = 0;

    /* Load the SDL library */
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    printf("Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

    devcount = SDL_GetNumAudioDevices(0);
    if (devcount < 1) {
        fprintf(stderr, "Don't see any specific audio devices!\n");
    } else {
        if (argv[1] == NULL) {
            argv[1] = "sample.wav";
        }

        /* Load the wave file into memory */
        if (SDL_LoadWAV(argv[1], &spec, &sound, &soundlen) == NULL) {
            fprintf(stderr, "Couldn't load %s: %s\n", argv[1],
                    SDL_GetError());
        } else {
            test_multi_audio(devcount);
            SDL_FreeWAV(sound);
        }
    }

    SDL_Quit();
    return 0;
}
