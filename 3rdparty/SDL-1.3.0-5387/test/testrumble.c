/*
Copyright (c) 2011, Edgar Simo Serra
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the Simple Directmedia Layer (SDL) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * includes
 */
#include <stdlib.h>
#include "SDL.h"
#include "SDL_haptic.h"

#include <stdio.h>              /* printf */
#include <string.h>             /* strstr */
#include <ctype.h>              /* isdigit */


static SDL_Haptic *haptic;


/**
 * @brief The entry point of this force feedback demo.
 * @param[in] argc Number of arguments.
 * @param[in] argv Array of argc arguments.
 */
int
main(int argc, char **argv)
{
    int i;
    char *name;
    int index;
    SDL_HapticEffect efx[5];
    int id[5];
    int nefx;
    unsigned int supported;

    name = NULL;
    index = -1;
    if (argc > 1) {
        name = argv[1];
        if ((strcmp(name, "--help") == 0) || (strcmp(name, "-h") == 0)) {
            printf("USAGE: %s [device]\n"
                   "If device is a two-digit number it'll use it as an index, otherwise\n"
                   "it'll use it as if it were part of the device's name.\n",
                   argv[0]);
            return 0;
        }

        i = strlen(name);
        if ((i < 3) && isdigit(name[0]) && ((i == 1) || isdigit(name[1]))) {
            index = atoi(name);
            name = NULL;
        }
    }

    /* Initialize the force feedbackness */
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK |
             SDL_INIT_HAPTIC);
    printf("%d Haptic devices detected.\n", SDL_NumHaptics());
    if (SDL_NumHaptics() > 0) {
        /* We'll just use index or the first force feedback device found */
        if (name == NULL) {
            i = (index != -1) ? index : 0;
        }
        /* Try to find matching device */
        else {
            for (i = 0; i < SDL_NumHaptics(); i++) {
                if (strstr(SDL_HapticName(i), name) != NULL)
                    break;
            }

            if (i >= SDL_NumHaptics()) {
                printf("Unable to find device matching '%s', aborting.\n",
                       name);
                return 1;
            }
        }

        haptic = SDL_HapticOpen(i);
        if (haptic == NULL) {
            printf("Unable to create the haptic device: %s\n",
                   SDL_GetError());
            return 1;
        }
        printf("Device: %s\n", SDL_HapticName(i));
    } else {
        printf("No Haptic devices found!\n");
        return 1;
    }

    /* We only want force feedback errors. */
    SDL_ClearError();

    if (SDL_HapticRumbleSupported(haptic) == SDL_FALSE) {
        printf("\nRumble not supported!\n");
        return 1;
    }
    if (SDL_HapticRumbleInit(haptic) != 0) {
        printf("\nFailed to initialize rumble: %s\n", SDL_GetError());
        return 1;
    }
    printf("Playing 2 second rumble at 0.5 magnitude.\n");
    if (SDL_HapticRumblePlay(haptic, 0.5, 5000) != 0) {
       printf("\nFailed to play rumble: %s\n", SDL_GetError() );
       return 1;
    }
    SDL_Delay(2000);
    printf("Stopping rumble.\n");
    SDL_HapticRumbleStop(haptic);
    SDL_Delay(2000);
    printf("Playing 2 second rumble at 0.3 magnitude.\n");
    if (SDL_HapticRumblePlay(haptic, 0.3, 5000) != 0) {
       printf("\nFailed to play rumble: %s\n", SDL_GetError() );
       return 1;
    }
    SDL_Delay(2000);

    /* Quit */
    if (haptic != NULL)
        SDL_HapticClose(haptic);
    SDL_Quit();

    return 0;
}

