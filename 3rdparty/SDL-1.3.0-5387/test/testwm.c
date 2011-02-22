
/* Test out the window manager interaction functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

/* Is the cursor visible? */
static int visible = 1;

static Uint8 video_bpp;
static Uint32 video_flags;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

int
SetVideoMode(int w, int h)
{
    SDL_Surface *screen;
    int i;
    Uint8 *buffer;
    SDL_Color palette[256];

    screen = SDL_SetVideoMode(w, h, video_bpp, video_flags);
    if (screen == NULL) {
        fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
                w, h, video_bpp, SDL_GetError());
        return (-1);
    }
    printf("Running in %s mode\n", screen->flags & SDL_FULLSCREEN ?
           "fullscreen" : "windowed");

    /* Set the surface pixels and refresh! */
    for (i = 0; i < 256; ++i) {
        palette[i].r = 255 - i;
        palette[i].g = 255 - i;
        palette[i].b = 255 - i;
    }
    SDL_SetColors(screen, palette, 0, 256);
    if (SDL_LockSurface(screen) < 0) {
        fprintf(stderr, "Couldn't lock display surface: %s\n",
                SDL_GetError());
        return (-1);
    }
    buffer = (Uint8 *) screen->pixels;
    for (i = 0; i < screen->h; ++i) {
        memset(buffer, (i * 255) / screen->h,
               screen->w * screen->format->BytesPerPixel);
        buffer += screen->pitch;
    }
    SDL_UnlockSurface(screen);
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    return (0);
}

SDL_Surface *
LoadIconSurface(char *file, Uint8 ** maskp)
{
    SDL_Surface *icon;
    Uint8 *pixels;
    Uint8 *mask;
    int mlen, i, j;

    *maskp = NULL;

    /* Load the icon surface */
    icon = SDL_LoadBMP(file);
    if (icon == NULL) {
        fprintf(stderr, "Couldn't load %s: %s\n", file, SDL_GetError());
        return (NULL);
    }

    /* Check width and height 
       if ( (icon->w%8) != 0 ) {
       fprintf(stderr, "Icon width must be a multiple of 8!\n");
       SDL_FreeSurface(icon);
       return(NULL);
       }
     */


    if (icon->format->palette == NULL) {
        fprintf(stderr, "Icon must have a palette!\n");
        SDL_FreeSurface(icon);
        return (NULL);
    }

    /* Set the colorkey */
    SDL_SetColorKey(icon, SDL_SRCCOLORKEY, *((Uint8 *) icon->pixels));

    /* Create the mask */
    pixels = (Uint8 *) icon->pixels;
    printf("Transparent pixel: (%d,%d,%d)\n",
           icon->format->palette->colors[*pixels].r,
           icon->format->palette->colors[*pixels].g,
           icon->format->palette->colors[*pixels].b);
    mlen = (icon->w * icon->h + 7) / 8;
    mask = (Uint8 *) malloc(mlen);
    if (mask == NULL) {
        fprintf(stderr, "Out of memory!\n");
        SDL_FreeSurface(icon);
        return (NULL);
    }
    memset(mask, 0, mlen);
    for (i = 0; i < icon->h; i++)
        for (j = 0; j < icon->w; j++) {
            int pindex = i * icon->pitch + j;
            int mindex = i * icon->w + j;
            if (pixels[pindex] != *pixels)
                mask[mindex >> 3] |= 1 << (7 - (mindex & 7));
        }
    *maskp = mask;
    return (icon);
}

void
HotKey_ToggleFullScreen(void)
{
    SDL_Surface *screen;

    screen = SDL_GetVideoSurface();
    if (SDL_WM_ToggleFullScreen(screen)) {
        printf("Toggled fullscreen mode - now %s\n",
               (screen->flags & SDL_FULLSCREEN) ? "fullscreen" : "windowed");
    } else {
        printf("Unable to toggle fullscreen mode\n");
        video_flags ^= SDL_FULLSCREEN;
        SetVideoMode(screen->w, screen->h);
    }
}

void
HotKey_ToggleGrab(void)
{
    SDL_GrabMode mode;

    printf("Ctrl-G: toggling input grab!\n");
    mode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
    if (mode == SDL_GRAB_ON) {
        printf("Grab was on\n");
    } else {
        printf("Grab was off\n");
    }
    mode = SDL_WM_GrabInput(mode ? SDL_GRAB_OFF : SDL_GRAB_ON);
    if (mode == SDL_GRAB_ON) {
        printf("Grab is now on\n");
    } else {
        printf("Grab is now off\n");
    }
}

void
HotKey_Iconify(void)
{
    printf("Ctrl-Z: iconifying window!\n");
    SDL_WM_IconifyWindow();
}

void
HotKey_Quit(void)
{
    SDL_Event event;

    printf("Posting internal quit request\n");
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
}


static void
print_modifiers(void)
{
    int mod;
    printf(" modifiers:");
    mod = SDL_GetModState();
    if(!mod) {
        printf(" (none)");
        return;
    }
    if(mod & KMOD_LSHIFT)
        printf(" LSHIFT");
    if(mod & KMOD_RSHIFT)
        printf(" RSHIFT");
    if(mod & KMOD_LCTRL)
        printf(" LCTRL");
    if(mod & KMOD_RCTRL)
        printf(" RCTRL");
    if(mod & KMOD_LALT)
        printf(" LALT");
    if(mod & KMOD_RALT)
        printf(" RALT");
    if(mod & KMOD_LMETA)
        printf(" LMETA");
    if(mod & KMOD_RMETA)
        printf(" RMETA");
    if(mod & KMOD_NUM)
        printf(" NUM");
    if(mod & KMOD_CAPS)
        printf(" CAPS");
    if(mod & KMOD_MODE)
        printf(" MODE");
}

static void PrintKey(const SDL_Keysym *sym, int pressed)
{
    /* Print the keycode, name and state */
    if ( sym->sym ) {
        printf("Key %s:  %d-%s ", pressed ?  "pressed" : "released",
                    sym->sym, SDL_GetKeyName(sym->sym));
    } else {
        printf("Unknown Key (scancode = %d) %s ", sym->scancode,
                    pressed ?  "pressed" : "released");
    }

    /* Print the translated character, if one exists */
    if ( sym->unicode ) {
        /* Is it a control-character? */
        if ( sym->unicode < ' ' ) {
            printf(" (^%c)", sym->unicode+'@');
        } else {
#ifdef UNICODE
            printf(" (%c)", sym->unicode);
#else
            /* This is a Latin-1 program, so only show 8-bits */
            if ( !(sym->unicode & 0xFF00) )
                printf(" (%c)", sym->unicode);
            else
                printf(" (0x%X)", sym->unicode);
#endif
        }
    }
    print_modifiers();
    printf("\n");
}


static int (SDLCALL * old_filterfunc) (void *, SDL_Event *);
static void *old_filterdata;

int SDLCALL
FilterEvents(void *userdata, SDL_Event * event)
{
    static int reallyquit = 0;

    if (old_filterfunc) {
        old_filterfunc(old_filterdata, event);
    }

    switch (event->type) {

    case SDL_ACTIVEEVENT:
        /* See what happened */
        printf("App %s ", event->active.gain ? "gained" : "lost");
        if (event->active.state & SDL_APPACTIVE)
            printf("active ");
        if (event->active.state & SDL_APPINPUTFOCUS)
            printf("input ");
        if (event->active.state & SDL_APPMOUSEFOCUS)
            printf("mouse ");
        printf("focus\n");

        /* See if we are iconified or restored */
        if (event->active.state & SDL_APPACTIVE) {
            printf("App has been %s\n",
                   event->active.gain ? "restored" : "iconified");
        }
        return (0);

        /* We want to toggle visibility on buttonpress */
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        if (event->button.state == SDL_PRESSED) {
            visible = !visible;
            SDL_ShowCursor(visible);
        }
        printf("Mouse button %d has been %s at %d,%d\n",
               event->button.button,
               (event->button.state == SDL_PRESSED) ? "pressed" : "released",
               event->button.x, event->button.y);
        return (0);

        /* Show relative mouse motion */
    case SDL_MOUSEMOTION:
#if 0
        printf("Mouse motion: {%d,%d} (%d,%d)\n",
               event->motion.x, event->motion.y,
               event->motion.xrel, event->motion.yrel);
#endif
        return (0);

    case SDL_KEYDOWN:
        PrintKey(&event->key.keysym, 1);
        if (event->key.keysym.sym == SDLK_ESCAPE) {
            HotKey_Quit();
        }
        if ((event->key.keysym.sym == SDLK_g) &&
            (event->key.keysym.mod & KMOD_CTRL)) {
            HotKey_ToggleGrab();
        }
        if ((event->key.keysym.sym == SDLK_z) &&
            (event->key.keysym.mod & KMOD_CTRL)) {
            HotKey_Iconify();
        }
        if ((event->key.keysym.sym == SDLK_RETURN) &&
            (event->key.keysym.mod & (KMOD_ALT|KMOD_META))) {
            HotKey_ToggleFullScreen();
        }
        return (0);

	case SDL_KEYUP:
		PrintKey(&event->key.keysym, 0);
		return(0);

        /* Pass the video resize event through .. */
    case SDL_VIDEORESIZE:
        return (1);

        /* This is important!  Queue it if we want to quit. */
    case SDL_QUIT:
        if (!reallyquit) {
            reallyquit = 1;
            printf("Quit requested\n");
            return (0);
        }
        printf("Quit demanded\n");
        return (1);

        /* This will never happen because events queued directly
           to the event queue are not filtered.
         */
    case SDL_USEREVENT:
        return (1);

        /* Drop all other events */
    default:
        return (0);
    }
}

int
main(int argc, char *argv[])
{
    SDL_Event event;
    const char *title;
    SDL_Surface *icon;
    Uint8 *icon_mask;
    int parsed;
    int w, h;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    /* Check command line arguments */
    w = 640;
    h = 480;
    video_bpp = 8;
    video_flags = SDL_SWSURFACE;
    parsed = 1;
    while (parsed) {
        if ((argc >= 2) && (strcmp(argv[1], "-fullscreen") == 0)) {
            video_flags |= SDL_FULLSCREEN;
            argc -= 1;
            argv += 1;
        } else if ((argc >= 2) && (strcmp(argv[1], "-resize") == 0)) {
            video_flags |= SDL_RESIZABLE;
            argc -= 1;
            argv += 1;
        } else if ((argc >= 2) && (strcmp(argv[1], "-noframe") == 0)) {
            video_flags |= SDL_NOFRAME;
            argc -= 1;
            argv += 1;
        } else if ((argc >= 3) && (strcmp(argv[1], "-width") == 0)) {
            w = atoi(argv[2]);
            argc -= 2;
            argv += 2;
        } else if ((argc >= 3) && (strcmp(argv[1], "-height") == 0)) {
            h = atoi(argv[2]);
            argc -= 2;
            argv += 2;
        } else if ((argc >= 3) && (strcmp(argv[1], "-bpp") == 0)) {
            video_bpp = atoi(argv[2]);
            argc -= 2;
            argv += 2;
        } else {
            parsed = 0;
        }
    }

    /* Set the icon -- this must be done before the first mode set */
    icon = LoadIconSurface("icon.bmp", &icon_mask);
    if (icon != NULL) {
        SDL_WM_SetIcon(icon, icon_mask);
    }
    if (icon_mask != NULL)
        free(icon_mask);

    /* Set the title bar */
    if (argv[1] == NULL)
        title = "Testing  1.. 2.. 3...";
    else
        title = argv[1];
    SDL_WM_SetCaption(title, "testwm");

    /* See if it's really set */
    SDL_WM_GetCaption(&title, NULL);
    if (title)
        printf("Title was set to: %s\n", title);
    else
        printf("No window title was set!\n");

    /* Initialize the display */
    if (SetVideoMode(w, h) < 0) {
        quit(1);
    }

    /* Set an event filter that discards everything but QUIT */
    SDL_GetEventFilter(&old_filterfunc, &old_filterdata);
    SDL_SetEventFilter(FilterEvents, NULL);

    /* Loop, waiting for QUIT */
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
        case SDL_VIDEORESIZE:
            printf("Got a resize event: %dx%d\n",
                   event.resize.w, event.resize.h);
            SetVideoMode(event.resize.w, event.resize.h);
            break;
        case SDL_USEREVENT:
            printf("Handling internal quit request\n");
            /* Fall through to the quit handler */
        case SDL_QUIT:
            printf("Bye bye..\n");
            quit(0);
        default:
            /* This should never happen */
            printf("Warning: Event %d wasn't filtered\n", event.type);
            break;
        }
    }
    printf("SDL_WaitEvent() error: %s\n", SDL_GetError());
    SDL_Quit();
    return (255);
}
