
/* Simple program:  Create a blank window, wait for keypress, quit.

   Please see the SDL documentation for details on using the SDL API:
   /Developer/Documentation/SDL/docs.html
*/
   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "SDL.h"

extern void Atlantis_Init ();
extern void Atlantis_Reshape (int w, int h);
extern void Atlantis_Animate ();
extern void Atlantis_Display ();

static SDL_Surface *gScreen;

static void initAttributes ()
{
    // Setup attributes we want for the OpenGL context
    
    int value;
    
    // Don't set color bit sizes (SDL_GL_RED_SIZE, etc)
    //    Mac OS X will always use 8-8-8-8 ARGB for 32-bit screens and
    //    5-5-5 RGB for 16-bit screens
    
    // Request a 16-bit depth buffer (without this, there is no depth buffer)
    value = 16;
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, value);
    
    
    // Request double-buffered OpenGL
    //     The fact that windows are double-buffered on Mac OS X has no effect
    //     on OpenGL double buffering.
    value = 1;
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, value);
}

static void printAttributes ()
{
    // Print out attributes of the context we created
    int nAttr;
    int i;
    
    int  attr[] = { SDL_GL_RED_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_GREEN_SIZE,
                    SDL_GL_ALPHA_SIZE, SDL_GL_BUFFER_SIZE, SDL_GL_DEPTH_SIZE };
                    
    char *desc[] = { "Red size: %d bits\n", "Blue size: %d bits\n", "Green size: %d bits\n",
                     "Alpha size: %d bits\n", "Color buffer size: %d bits\n", 
                     "Depth bufer size: %d bits\n" };

    nAttr = sizeof(attr) / sizeof(int);
    
    for (i = 0; i < nAttr; i++) {
    
        int value;
        SDL_GL_GetAttribute (attr[i], &value);
        printf (desc[i], value);
    } 
}

static void createSurface (int fullscreen)
{
    Uint32 flags = 0;
    
    flags = SDL_OPENGL;
    if (fullscreen)
        flags |= SDL_FULLSCREEN;
    
    // Create window
    gScreen = SDL_SetVideoMode (640, 480, 0, flags);
    if (gScreen == NULL) {
        
        fprintf (stderr, "Couldn't set 640x480 OpenGL video mode: %s\n",
                 SDL_GetError());
        SDL_Quit();
        exit(2);
    }
}

static void initGL ()
{
    Atlantis_Init ();
    Atlantis_Reshape (gScreen->w, gScreen->h);
}

static void drawGL ()
{
    Atlantis_Animate ();
    Atlantis_Display ();
}

static void mainLoop ()
{
    SDL_Event event;
    int done = 0;
    int fps = 24;
    int delay = 1000/fps;
    int thenTicks = -1;
    int nowTicks;
    
    while ( !done ) {

        /* Check for events */
        while ( SDL_PollEvent (&event) ) {
            switch (event.type) {

                case SDL_MOUSEMOTION:
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    break;
                case SDL_KEYDOWN:
                    /* Any keypress quits the app... */
                case SDL_QUIT:
                    done = 1;
                    break;
                default:
                    break;
            }
        }
    
        // Draw at 24 hz
        //     This approach is not normally recommended - it is better to
        //     use time-based animation and run as fast as possible
        drawGL ();
        SDL_GL_SwapBuffers ();

        // Time how long each draw-swap-delay cycle takes
        // and adjust delay to get closer to target framerate
        if (thenTicks > 0) {
            nowTicks = SDL_GetTicks ();
            delay += (1000/fps - (nowTicks-thenTicks));
            thenTicks = nowTicks;
            if (delay < 0)
                delay = 1000/fps;
        }
        else {
            thenTicks = SDL_GetTicks ();
        }

        SDL_Delay (delay);
    }
}

int main(int argc, char *argv[])
{
    // Init SDL video subsystem
    if ( SDL_Init (SDL_INIT_VIDEO) < 0 ) {
        
        fprintf(stderr, "Couldn't initialize SDL: %s\n",
            SDL_GetError());
        exit(1);
    }

    // Set GL context attributes
    initAttributes ();
    
    // Create GL context
    createSurface (0);
    
    // Get GL context attributes
    printAttributes ();
    
    // Init GL state
    initGL ();
    
    // Draw, get events...
    mainLoop ();
    
    // Cleanup
    SDL_Quit();

    return 0;
}
