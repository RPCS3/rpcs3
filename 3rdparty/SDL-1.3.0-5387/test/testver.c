
/* Test program to compare the compile-time version of SDL with the linked
   version of SDL
*/

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_revision.h"

int
main(int argc, char *argv[])
{
    SDL_version compiled;
    SDL_version linked;

#if SDL_VERSION_ATLEAST(1, 3, 0)
    printf("Compiled with SDL 1.3 or newer\n");
#else
    printf("Compiled with SDL older than 1.3\n");
#endif
    SDL_VERSION(&compiled);
    printf("Compiled version: %d.%d.%d.%d (%s)\n",
           compiled.major, compiled.minor, compiled.patch,
           SDL_REVISION_NUMBER, SDL_REVISION);
    SDL_GetVersion(&linked);
    printf("Linked version: %d.%d.%d.%d (%s)\n",
           linked.major, linked.minor, linked.patch,
           SDL_GetRevisionNumber(), SDL_GetRevision());
    SDL_Quit();
    return (0);
}
