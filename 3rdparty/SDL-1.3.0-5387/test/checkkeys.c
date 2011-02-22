
/* Simple program:  Loop, watching keystrokes
   Note that you need to call SDL_PollEvent() or SDL_WaitEvent() to 
   pump the event loop and catch keystrokes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

static void
print_modifiers(void)
{
    int mod;
    printf(" modifiers:");
    mod = SDL_GetModState();
    if (!mod) {
        printf(" (none)");
        return;
    }
    if (mod & KMOD_LSHIFT)
        printf(" LSHIFT");
    if (mod & KMOD_RSHIFT)
        printf(" RSHIFT");
    if (mod & KMOD_LCTRL)
        printf(" LCTRL");
    if (mod & KMOD_RCTRL)
        printf(" RCTRL");
    if (mod & KMOD_LALT)
        printf(" LALT");
    if (mod & KMOD_RALT)
        printf(" RALT");
    if (mod & KMOD_LGUI)
        printf(" LGUI");
    if (mod & KMOD_RGUI)
        printf(" RGUI");
    if (mod & KMOD_NUM)
        printf(" NUM");
    if (mod & KMOD_CAPS)
        printf(" CAPS");
    if (mod & KMOD_MODE)
        printf(" MODE");
}

static void
PrintKey(SDL_Keysym * sym, SDL_bool pressed, SDL_bool repeat)
{
    /* Print the keycode, name and state */
    if (sym->sym) {
        printf("Key %s:  scancode %d = %s, keycode 0x%08X = %s ",
               pressed ? "pressed " : "released",
               sym->scancode,
               SDL_GetScancodeName(sym->scancode),
               sym->sym, SDL_GetKeyName(sym->sym));
    } else {
        printf("Unknown Key (scancode %d = %s) %s ",
               sym->scancode,
               SDL_GetScancodeName(sym->scancode),
               pressed ? "pressed" : "released");
    }

    /* Print the translated character, if one exists */
    if (sym->unicode) {
        /* Is it a control-character? */
        if (sym->unicode < ' ') {
            printf(" (^%c)", sym->unicode + '@');
        } else {
#ifdef UNICODE
            printf(" (%c)", sym->unicode);
#else
            /* This is a Latin-1 program, so only show 8-bits */
            if (!(sym->unicode & 0xFF00))
                printf(" (%c)", sym->unicode);
            else
                printf(" (0x%X)", sym->unicode);
#endif
        }
    }
    print_modifiers();
    if (repeat) {
        printf(" (repeat)");
    }
    printf("\n");
}

static void
PrintText(char *text)
{
    printf("Text: %s\n", text);
}

int
main(int argc, char *argv[])
{
    SDL_Event event;
    int done;
    Uint32 videoflags;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    videoflags = SDL_SWSURFACE;
    while (argc > 1) {
        --argc;
        if (argv[argc] && !strcmp(argv[argc], "-fullscreen")) {
            videoflags |= SDL_FULLSCREEN;
        } else {
            fprintf(stderr, "Usage: %s [-fullscreen]\n", argv[0]);
            quit(1);
        }
    }

    /* Set 640x480 video mode */
    if (SDL_SetVideoMode(640, 480, 0, videoflags) == NULL) {
        fprintf(stderr, "Couldn't set 640x480 video mode: %s\n",
                SDL_GetError());
        quit(2);
    }

    /* Watch keystrokes */
    done = 0;
    while (!done) {
        /* Check for events */
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            PrintKey(&event.key.keysym, event.key.state, event.key.repeat);
            break;
        case SDL_TEXTINPUT:
            PrintText(event.text.text);
            break;
        case SDL_MOUSEBUTTONDOWN:
            /* Any button press quits the app... */
        case SDL_QUIT:
            done = 1;
            break;
        default:
            break;
        }
    }

    SDL_Quit();
    return (0);
}
