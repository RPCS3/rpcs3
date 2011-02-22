#include <stdio.h>
#include "SDL.h"

int
main(int argc, char **argv)
{
    SDL_AudioSpec spec;
    SDL_AudioCVT cvt;
    Uint32 len = 0;
    Uint8 *data = NULL;
    int cvtfreq = 0;
    int bitsize = 0;
    int blockalign = 0;
    int avgbytes = 0;
    SDL_RWops *io = NULL;

    if (argc != 4) {
        fprintf(stderr, "USAGE: %s in.wav out.wav newfreq\n", argv[0]);
        return 1;
    }

    cvtfreq = atoi(argv[3]);

    if (SDL_Init(SDL_INIT_AUDIO) == -1) {
        fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
        return 2;
    }

    if (SDL_LoadWAV(argv[1], &spec, &data, &len) == NULL) {
        fprintf(stderr, "failed to load %s: %s\n", argv[1], SDL_GetError());
        SDL_Quit();
        return 3;
    }

    if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
                          spec.format, spec.channels, cvtfreq) == -1) {
        fprintf(stderr, "failed to build CVT: %s\n", SDL_GetError());
        SDL_FreeWAV(data);
        SDL_Quit();
        return 4;
    }

    cvt.len = len;
    cvt.buf = (Uint8 *) malloc(len * cvt.len_mult);
    if (cvt.buf == NULL) {
        fprintf(stderr, "Out of memory.\n");
        SDL_FreeWAV(data);
        SDL_Quit();
        return 5;
    }
    memcpy(cvt.buf, data, len);

    if (SDL_ConvertAudio(&cvt) == -1) {
        fprintf(stderr, "Conversion failed: %s\n", SDL_GetError());
        free(cvt.buf);
        SDL_FreeWAV(data);
        SDL_Quit();
        return 6;
    }

    /* write out a WAV header... */
    io = SDL_RWFromFile(argv[2], "wb");
    if (io == NULL) {
        fprintf(stderr, "fopen('%s') failed: %s\n", argv[2], SDL_GetError());
        free(cvt.buf);
        SDL_FreeWAV(data);
        SDL_Quit();
        return 7;
    }

    bitsize = SDL_AUDIO_BITSIZE(spec.format);
    blockalign = (bitsize / 8) * spec.channels;
    avgbytes = cvtfreq * blockalign;

    SDL_WriteLE32(io, 0x46464952);      /* RIFF */
    SDL_WriteLE32(io, len * cvt.len_mult + 36);
    SDL_WriteLE32(io, 0x45564157);      /* WAVE */
    SDL_WriteLE32(io, 0x20746D66);      /* fmt */
    SDL_WriteLE32(io, 16);      /* chunk size */
    SDL_WriteLE16(io, 1);       /* uncompressed */
    SDL_WriteLE16(io, spec.channels);   /* channels */
    SDL_WriteLE32(io, cvtfreq); /* sample rate */
    SDL_WriteLE32(io, avgbytes);        /* average bytes per second */
    SDL_WriteLE16(io, blockalign);      /* block align */
    SDL_WriteLE16(io, bitsize); /* significant bits per sample */
    SDL_WriteLE32(io, 0x61746164);      /* data */
    SDL_WriteLE32(io, cvt.len_cvt);     /* size */
    SDL_RWwrite(io, cvt.buf, cvt.len_cvt, 1);

    if (SDL_RWclose(io) == -1) {
        fprintf(stderr, "fclose('%s') failed: %s\n", argv[2], SDL_GetError());
        free(cvt.buf);
        SDL_FreeWAV(data);
        SDL_Quit();
        return 8;
    }                           // if

    free(cvt.buf);
    SDL_FreeWAV(data);
    SDL_Quit();
    return 0;
}                               // main

// end of resample_test.c ...
