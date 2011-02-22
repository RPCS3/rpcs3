
/* Print out all the scancodes we have, just to verify them */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

int
main(int argc, char *argv[])
{
    SDL_Scancode scancode;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
        printf("Scancode #%d, \"%s\"\n", scancode,
               SDL_GetScancodeName(scancode));
    }
    SDL_Quit();
    return (0);
}
