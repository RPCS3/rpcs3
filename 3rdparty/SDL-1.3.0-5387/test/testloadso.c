
/* Test program to test dynamic loading with the loadso subsystem.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

typedef int (*fntype) (const char *);

int
main(int argc, char *argv[])
{
    int retval = 0;
    int hello = 0;
    const char *libname = NULL;
    const char *symname = NULL;
    void *lib = NULL;
    fntype fn = NULL;

    if (argc != 3) {
        const char *app = argv[0];
        fprintf(stderr, "USAGE: %s <library> <functionname>\n", app);
        fprintf(stderr, "       %s --hello <lib with puts()>\n", app);
        return 1;
    }

    /* Initialize SDL */
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 2;
    }

    if (strcmp(argv[1], "--hello") == 0) {
        hello = 1;
        libname = argv[2];
        symname = "puts";
    } else {
        libname = argv[1];
        symname = argv[2];
    }

    lib = SDL_LoadObject(libname);
    if (lib == NULL) {
        fprintf(stderr, "SDL_LoadObject('%s') failed: %s\n",
                libname, SDL_GetError());
        retval = 3;
    } else {
        fn = (fntype) SDL_LoadFunction(lib, symname);
        if (fn == NULL) {
            fprintf(stderr, "SDL_LoadFunction('%s') failed: %s\n",
                    symname, SDL_GetError());
            retval = 4;
        } else {
            printf("Found %s in %s at %p\n", symname, libname, fn);
            if (hello) {
                printf("Calling function...\n");
                fflush(stdout);
                fn("     HELLO, WORLD!\n");
                printf("...apparently, we survived.  :)\n");
                printf("Unloading library...\n");
                fflush(stdout);
            }
        }
        SDL_UnloadObject(lib);
    }
    SDL_Quit();
    return (0);
}
