
#include <SDL/SDL.h>
#if defined(NDS) || defined(__NDS__) || defined (__NDS)
#include <nds.h>
#include <fat.h>
#else
#define swiWaitForVBlank()
#define consoleDemoInit()
#define fatInitDefault()
#define RGB15(r,g,b) SDL_MapRGB(screen->format,((r)<<3),((g)<<3),((b)<<3))
#endif
    void
splash(SDL_Surface * screen, int s)
{
    SDL_Surface *logo;
    SDL_Rect area = { 0, 0, 256, 192 };

    logo = SDL_LoadBMP("sdl.bmp");
    if (!logo) {
        printf("Couldn't splash.\n");
        return;
    }
    /*logo->flags &= ~SDL_PREALLOC; */
    SDL_BlitSurface(logo, NULL, screen, &area);
    SDL_Flip(screen);
    while (s-- > 0) {
        int i = 60;
        while (--i)
            swiWaitForVBlank();
    }
}

int
main(void)
{
    SDL_Surface *screen;
    SDL_Joystick *stick;
    SDL_Event event;
    SDL_Rect rect = { 0, 0, 256, 192 };
    int i;

    consoleDemoInit();
    puts("Hello world!  Initializing FAT...");
    fatInitDefault();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        puts("# error initializing SDL");
        puts(SDL_GetError());
        return 1;
    }
    puts("* initialized SDL");
    screen = SDL_SetVideoMode(256, 192, 15, SDL_SWSURFACE);
    if (!screen) {
        puts("# error setting video mode");
        puts(SDL_GetError());
        return 2;
    }
    screen->flags &= ~SDL_PREALLOC;
    puts("* set video mode");
    stick = SDL_JoystickOpen(0);
    if (stick == NULL) {
        puts("# error opening joystick");
        puts(SDL_GetError());
//              return 3;
    }
    puts("* opened joystick");

    /*splash(screen, 3); */

    SDL_FillRect(screen, &rect, RGB15(0, 0, 31) | 0x8000);
    SDL_Flip(screen);

    while (1)
        while (SDL_PollEvent(&event))
            switch (event.type) {
            case SDL_JOYBUTTONDOWN:
                SDL_FillRect(screen, &rect, (u16) rand() | 0x8000);
                SDL_Flip(screen);
                if (rect.w > 8) {
                    rect.x += 4;
                    rect.y += 3;
                    rect.w -= 8;
                    rect.h -= 6;
                }
                printf("button %d pressed at %d ticks\n",
                       event.jbutton.button, SDL_GetTicks());
                break;
            case SDL_QUIT:
                SDL_Quit();
                return 0;
            default:
                break;
            }
    return 0;
}


